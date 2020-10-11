#include <stdlib.h>
#include "system.h"
#include "button.h"
#include "navswitch.h"
#include "led.h"
#include "tinygl.h"
#include "pacer.h"
#include "ir_uart.h"
#include "tetris.h"
#include "../fonts/font5x7_1.h"

#define LOOP_RATE 300
#define TICK_RATE 1
#define MESSAGE_RATE 5

enum {WAIT_FOR_CONNECT, GAME_RUN, GAME_WIN, GAME_OVER, WAIT_FOR_INPUT};

void check_button(Tetronimo* activeTetronimo, uint8_t* bitmap)
{
    button_update();
    
    if (button_push_event_p(BUTTON1) && activeTetronimo) {
        rotate_tetronimo(activeTetronimo, bitmap, 0);
    }
}



void check_navswitch(Tetronimo* activeTetronimo, uint8_t* bitmap)
{
    navswitch_update();
    
    if (navswitch_push_event_p (NAVSWITCH_PUSH) && activeTetronimo) {
        rotate_tetronimo(activeTetronimo, bitmap, 1);
    }
    if (navswitch_push_event_p (NAVSWITCH_EAST) && activeTetronimo) {
        shift_tetronimo(activeTetronimo, bitmap, tinygl_point(1,0));
    }
    if (navswitch_push_event_p (NAVSWITCH_WEST) && activeTetronimo) {
        shift_tetronimo(activeTetronimo, bitmap, tinygl_point(-1,0));
    }
    if (navswitch_push_event_p (NAVSWITCH_SOUTH) && activeTetronimo) {
        slam_tetronimo(activeTetronimo, bitmap);
    }
}

void send_lines(int n_lines) 
{
    for (int line = 0; line < n_lines; line++) {
        ir_uart_putc('L');
    }
    
}

void send_connect(void) 
{
    for (int i = 0; i < 3; i++) {
        ir_uart_putc('C');
    }
}

bool recieveConnect(void)
{
    char recieved = 0;
    bool isConnected = false;
    if (ir_uart_read_ready_p ()) {
        recieved = ir_uart_getc ();
        if (recieved == 'C') {
            isConnected = true;
        }
    }
    return isConnected;
}


int main(void)
{
    system_init();
    led_init();
    ir_uart_init();
    button_init ();
    navswitch_init ();
    
    int led_state = 1;
    led_set(LED1, led_state);
    
    pacer_init (LOOP_RATE);
    
    tinygl_font_set(&font5x7_1);
    tinygl_init(LOOP_RATE);
    
    Tetronimo pieces[] = {
        {0, {0,0}, 3, {{0,1}, {0,0}, {1,0}}}, // Small J piece
        {1, {0,0}, 4, {{-1,0}, {-1,1}, {0,0}, {0,1}}}, // Square piece
        {2, {0,0}, 4, {{-1,0}, { 0,1}, {0,0}, {1,0}}}, // T piece
        {3, {0,0}, 3, {{ 0,0}, {1,0}, {0,1}}}, // Small L piece
        {4, {0,0}, 3, {{ -1,0}, {0,0}, {1,0}}}, // Small I piece
        {5, {0,0}, 4, {{-1,0}, { 0,0}, {0,1}, {1,1}}}, // S piece
        {6, {0,0}, 4, {{-1,1}, { 0,1}, {0,0}, {1,0}}}, // Z piece
    };
    
    uint8_t bitmap[7] = {0};
    
    Tetronimo* activeTetronimo;
    get_new_tetronimo(&activeTetronimo, pieces);
    
    int state = WAIT_FOR_CONNECT;
    int nLinesCleared = 0;
    
    int tick = 0;
    while (1) {
        pacer_wait();
        if (state == WAIT_FOR_CONNECT) {
            navswitch_update();
            
            if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
                send_connect();
                state = GAME_RUN;
            }
            
            if (recieveConnect()) {
                state = GAME_RUN;
            }
        } else if (state == GAME_RUN) {
            if (activeTetronimo == NULL) {
                get_new_tetronimo(&activeTetronimo, pieces);
            }
            
            if (ir_uart_read_ready_p ()) {
                char recieved = ir_uart_getc ();
                if (recieved == 'W') {
                    state = GAME_WIN;
                }
                if (recieved == 'L') {
                    add_line(bitmap);
                }
            }
            
            check_navswitch(activeTetronimo, bitmap);
            check_button(activeTetronimo, bitmap);
                
            tick = tick + 1;
            if (tick > LOOP_RATE / TICK_RATE) {
                tick = 0;
                led_state = !led_state;
                led_set(LED1, led_state);
                
                if (has_tetronimo_landed(activeTetronimo, bitmap)) {
                    if (check_game_over(activeTetronimo)) {
                        state = GAME_OVER;
                    }
                    save_tetronimo_to_bitmap(activeTetronimo, bitmap);
                    activeTetronimo = NULL;
                } else {
                    shift_tetronimo(activeTetronimo, bitmap, tinygl_point(0,1)); //Fall
                }
            }
        
            tinygl_clear();
            drawBitmap(bitmap);
            nLinesCleared = clear_full_lines(bitmap);
            send_lines(nLinesCleared);
            if (activeTetronimo) {
                draw_tetronimo(activeTetronimo);
            }
        } else if (state == GAME_OVER) {
            tinygl_clear();
            tinygl_text_speed_set(MESSAGE_RATE);
    
            tinygl_text_mode_set(TINYGL_TEXT_MODE_SCROLL);
            tinygl_text_speed_set (20);
            tinygl_text("GAME OVER!");
            
            ir_uart_putc('W');
            
            state = WAIT_FOR_INPUT;
            
        } else if (state == WAIT_FOR_INPUT) {
            button_update();
            if (button_push_event_p(BUTTON1)) {
                reset(&activeTetronimo, bitmap);
                state = WAIT_FOR_CONNECT;
            }
        } else if (state == GAME_WIN) {
            tinygl_clear();
            tinygl_text_speed_set(MESSAGE_RATE);
    
            tinygl_text_mode_set(TINYGL_TEXT_MODE_SCROLL);
            tinygl_text_speed_set (20);
            tinygl_text("YOU WIN!");
            state = WAIT_FOR_INPUT;
        }
            
        tinygl_update();
    }
}
