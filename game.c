/** @file   game.c
    @authors S. Heslip (she119), S. Li (gli65)
    @date   11 Oct 2020
*/

#include <stdlib.h>
#include <stdio.h>
#include "system.h"
#include "button.h"
#include "navswitch.h"
#include "led.h"
#include "tinygl.h"
#include "pacer.h"
#include "ir_uart.h"
#include "../fonts/font5x7_1.h"
#include "tetris.h"

#define LOOP_RATE 300
#define GAME_RATE 1
#define MESSAGE_RATE 15


/* Defines the game states */
typedef enum game_state_t
{
    WAIT_FOR_CONNECT,
    GAME_RUN,
    GAME_WIN,
    GAME_LOSS,
    RESET
} game_state;


/* Defines the IR character messages */
enum {
    SEND_LINE = 'A',
    SEND_WIN = 'W',
    SEND_CONNECT_REQUEST = 'L'
};


/* Checks the left button for a press and rotates tetronimo  */
void checkButton(Tetronimo* activeTetronimo, uint8_t* bitmap)
{
    button_update();
    if (button_push_event_p(BUTTON1) && activeTetronimo) {
        rotateTetronimo(activeTetronimo, bitmap, 0);
    }
}

/* Checks the navswitch and performs associated action */
void checkNavswitch(Tetronimo* activeTetronimo, uint8_t* bitmap)
{
    navswitch_update();
    if (navswitch_push_event_p (NAVSWITCH_PUSH) && activeTetronimo) {
        rotateTetronimo(activeTetronimo, bitmap, 1);
    }
    if (navswitch_push_event_p (NAVSWITCH_EAST) && activeTetronimo) {
        shiftTetronimo(activeTetronimo, bitmap, tinygl_point(1,0));
    }
    if (navswitch_push_event_p (NAVSWITCH_WEST) && activeTetronimo) {
        shiftTetronimo(activeTetronimo, bitmap, tinygl_point(-1,0));
    }
    if (navswitch_push_event_p (NAVSWITCH_SOUTH) && activeTetronimo) {
        slamTetronimo(activeTetronimo, bitmap);
    }
}

/* Sends nLines to the other board to make it harder for them */
void sendLines(int nLines)
{
    for (int line = 0; line < nLines; line++) {
        ir_uart_putc(SEND_LINE);
    }

}

/* Sends a connection request to the other board over IR */
void sendConnect(void)
{
    for (int i = 0; i < 3; i++) {
        ir_uart_putc(SEND_CONNECT_REQUEST);
    }
}

/* Receives a connection request from the other board over IR */
bool recieveConnect(void)
{
    char recieved = 0;
    bool isConnected = false;
    if (ir_uart_read_ready_p ()) {
        recieved = ir_uart_getc ();
        if (recieved == SEND_CONNECT_REQUEST) {
            isConnected = true;
        }
    }
    return isConnected;
}

/* Main game loop */
int main(void)
{
    system_init();
    led_init();
    ir_uart_init();
    button_init ();
    navswitch_init ();
    pacer_init (LOOP_RATE);

    tinygl_font_set(&font5x7_1);
    tinygl_init(LOOP_RATE);
    tinygl_text_speed_set(MESSAGE_RATE);
    tinygl_text_mode_set(TINYGL_TEXT_MODE_SCROLL);
    tinygl_text_speed_set (20);

    // Fill all tetronimos with their associated values
    Tetronimo allTetronimos[] = {
        {{0,0}, 2, {{0,1}, {0,0}}},                 // Small I piece
        {{0,0}, 4, {{-1,0}, {-1,1}, {0,0}, {0,1}}}, // Square piece
        {{0,0}, 4, {{-1,0}, { 0,1}, {0,0}, {1,0}}}, // T piece
        {{0,0}, 3, {{ 0,0}, {1,0}, {0,1}}},         // Small L piece
        {{0,0}, 3, {{ -1,0}, {0,0}, {1,0}}},        //  I piece
        {{0,0}, 4, {{-1,0}, { 0,0}, {0,1}, {1,1}}}, // S piece
        {{0,0}, 4, {{-1,1}, { 0,1}, {0,0}, {1,0}}}, // Z piece
    };

    uint8_t bitmap[7] = {0};

    Tetronimo* activeTetronimo;
    getNewTetronimo(&activeTetronimo, allTetronimos);

    game_state gameState = WAIT_FOR_CONNECT;
    int isDisplayingMessage = false;
    int nLinesCleared = 0;

    int ledTick = 0;
    int ledState = 0;
    int ledRate = 4;
    int gameTick = 0;
    int playerScore = 0;
    while (1) {
        pacer_wait();
        switch (gameState) {
            case WAIT_FOR_CONNECT :
                // Establish IR connection
                if (!isDisplayingMessage) {
                    isDisplayingMessage = 1;
                    tinygl_text(" -Push nav button to start- ");
                }
                navswitch_update();
                if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
                    sendConnect();
                    gameState = GAME_RUN;
                }
                if (recieveConnect()) {
                    gameState = GAME_RUN;
                }
                break;

            case GAME_RUN :
                // Run game logic
                ledState = 4;

                if (activeTetronimo == NULL) {
                    getNewTetronimo(&activeTetronimo, allTetronimos);
                }

                if (ir_uart_read_ready_p ()) {
                    // Check for win or line sent
                    char recieved = ir_uart_getc ();
                    if (recieved == SEND_WIN) {
                        gameState = GAME_WIN;
                    } else if (recieved == SEND_LINE) {
                        addLine(bitmap);
                    }
                }

                checkNavswitch(activeTetronimo, bitmap);
                checkButton(activeTetronimo, bitmap);

                gameTick++;
                if (gameTick > LOOP_RATE / GAME_RATE) {
                    gameTick = 0;

                    if (hasTetronimoLanded(activeTetronimo, bitmap)) {
                        if (checkGameOver(activeTetronimo)) {
                            gameState = GAME_LOSS;
                        }
                        saveTetronimoToBitmap(activeTetronimo, bitmap);
                        activeTetronimo = NULL;
                    } else {
                        shiftTetronimo(activeTetronimo, bitmap, tinygl_point(0,1)); //Fall
                    }
                }
                tinygl_clear();
                drawBitmap(bitmap);
                nLinesCleared = clearFullLines(bitmap);
                sendLines(nLinesCleared);
                if (activeTetronimo) {
                    drawTetronimo(activeTetronimo);
                }
                break;


            case GAME_LOSS :
                // This player has lost the game
                ledRate = 2;
                tinygl_clear();
                char buff1[22];
                sprintf(buff1, " -You lose. Score:%d- ", playerScore);
                tinygl_text(buff1);
                ir_uart_putc(SEND_WIN);
                gameState = RESET;
                break;


            case RESET :
                // Resets the game
                reset(&activeTetronimo, bitmap);
                gameState = WAIT_FOR_CONNECT;
                break;


            case GAME_WIN :
                // This player has won the game
                ledRate = 6;
                tinygl_clear();
                playerScore++;
                char buff2[22];
                sprintf(buff2, " -You win! Score:%d- ", playerScore);
                tinygl_text(buff2);
                gameState = RESET;
                break;
        }

        tinygl_update();

        ledTick++;
        if (ledTick > LOOP_RATE / ledRate) {
            ledTick = 0;
            ledState = !ledState;
        }
        led_set(LED1, ledState);
    }
}
