/** @file   battleships.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   30 Sep 2020
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "system.h"
#include "button.h"
#include "navswitch.h"
#include "led.h"
#include "tinygl.h"
#include "pacer.h"
#include "ir_uart.h"
#include "timer.h"
#include "../fonts/font5x7_1.h"
#include "ir_comms.h"
#include "battleships.h"

/* Globals */
static state_t gameState;       // Variable to track state of game
static bool isPlayerOne;        // Is this board player 1?
static int stateTick = 0;       // Tracks length of time elapsed during a state
static Targetter* targetter;    // Firing targetter cursor
static Ship* currentShip;       // Current Ship for ship placing
static int nShips;              // Total number of ships in Ship array
static bool placementSuccess;   // Result of last ship placement TODO: Don't think this should be here...
static int nShipsPlaced;        // Total number of Ships successfully placed
static Ship ships[MAX_SHIPS];   // Ship array of all ships


// DEBUG
static int ledState = 0;

/* Returns the vector addition of points a and b */
tinygl_point_t vectorAdd(tinygl_point_t a, tinygl_point_t b)
{
    return tinygl_point(a.x + b.x, a.y + b.y);
}

/* Returns the grid position of a Ship's offset given an offsetIndex */
tinygl_point_t getGridPosition(Ship* ship, int offsetIndex)
{
    return vectorAdd(ship->pos, ship->offsets[offsetIndex]);
}

/* Checks equality of two tingygl_points A and B */
bool isEqual(tinygl_point_t pointA, tinygl_point_t pointB) {
    return (pointA.x == pointB.x && pointA.y == pointB.y);
}

/* Checks if point on the grid is vacant */
bool isPointVacant(tinygl_point_t point, Ship* ships, int nShips)
{
    bool pointIsVacant = true;
    tinygl_point_t gridPosition;
    Ship* currentShip;
    bool isShipActive;
    bool isShipPlaced;
    for (int i = 0; i < nShips; i++) {
        currentShip = &ships[i];
        for (int j = 0; j < currentShip->nOffsets; j++) {
            gridPosition = getGridPosition(currentShip, j);
            isShipActive = currentShip->isActive;
            isShipPlaced = currentShip->isPlaced;
            if (isEqual(point, gridPosition) && !isShipActive && isShipPlaced) {
                pointIsVacant = false;
                break;
            }
        }
    }
    return pointIsVacant;
}

/* Checks whether a point is within the game grid or out of bounds */
bool isWithinGrid(tinygl_point_t point)
{
    bool isWithinXLimits = (point.x >= TOP_BOUND && point.x <= BOTTOM_BOUND);
    bool isWithinYLimits = (point.y >= RIGHT_BOUND && point.y <= LEFT_BOUND);

    return (isWithinXLimits && isWithinYLimits);
}

/* Rotates a Ship 90 degrees and shifts if Ship is then outside the grid*/
void rotateShip(Ship* ship)
{
    tinygl_point_t newOffset;
    tinygl_point_t offsetGridPosition;

    // Rotate each offset using 2D rotation matrix
    for (int i = 0; i < ship->nOffsets; i++) {
        newOffset = tinygl_point(-(ship->offsets[i]).y, (ship->offsets[i]).x);

        ship->offsets[i] = newOffset;

        offsetGridPosition = getGridPosition(ship, i);

        // Shift if Ship is out of bounds now
        if (offsetGridPosition.x < TOP_BOUND) {
            moveShip(ship, tinygl_point(1,0));
        } else if (offsetGridPosition.x > BOTTOM_BOUND) {
            moveShip(ship, tinygl_point(-1,0));
        }

        if (offsetGridPosition.y < RIGHT_BOUND) {
            moveShip(ship, tinygl_point(0,1));
        } else if (offsetGridPosition.y > LEFT_BOUND) {
            moveShip(ship, tinygl_point(0,-1));
        }
    }
}

/* Moves a Ship across the grid in a given direction if possible */
void moveShip(Ship* ship, tinygl_point_t moveDirection)
{
    bool isValidMove = true;
    tinygl_point_t gridPosition;
    tinygl_point_t newGridPosition;

    for (int i = 0; i < ship->nOffsets; i++) {
        gridPosition = getGridPosition(ship, i);
        newGridPosition = vectorAdd(gridPosition, moveDirection);
        if(!isWithinGrid(newGridPosition)) {
            isValidMove = false;
            break;
        }
    }
    if (isValidMove) {
        ship->pos = vectorAdd(ship->pos, moveDirection);
    }
}

/* Draws a Ship to the LED matrix */
void drawShip(Ship* ship)
{
    for (int i = 0; i < ship->nOffsets; i++) {
        // Only draw offset if not hit
        if (ship->hitStatus[i] == false) {
            tinygl_draw_point(vectorAdd(ship->pos, ship->offsets[i]), 1);
        }
    }
}

/* Attempts to place a Ship at it's current position, returns
 * whether placement was successful or not. */
bool placeShip(Ship* ship, Ship* ships, int nShips)
{
    bool isValidPlacement = true;
    tinygl_point_t gridPosition;

    for (int i = 0; i < ship->nOffsets; i++) {
        gridPosition = getGridPosition(ship, i);

        if(!isPointVacant(gridPosition, ships, nShips)) {
            isValidPlacement = false;
            break;
        }
    }

    if (isValidPlacement) {
        ship->isActive = 0;
        ship->isPlaced = 1;
    }
    return isValidPlacement;
}


/* Performs check on navswitch for the moving and rotating of the
 * current ship for the PLACE_SHIPS mode */
void checkNavswitchMoveShip(Ship* currentShip)
{
    if (navswitch_push_event_p(NAVSWITCH_WEST)) {
        moveShip(currentShip, tinygl_point(-1,0));
    }
    if (navswitch_push_event_p(NAVSWITCH_EAST)) {
        moveShip(currentShip, tinygl_point(1, 0));
    }
    if (navswitch_push_event_p(NAVSWITCH_NORTH)) {
        moveShip(currentShip, tinygl_point(0, -1));
    }
    if (navswitch_push_event_p(NAVSWITCH_SOUTH)) {
        moveShip(currentShip, tinygl_point(0,1));
    }
    if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
        rotateShip(currentShip);
    }
}

/* Performs check on navswitch for the moving and of the
 * firing targetter for the MY_TURN mode */
void checkNavswitchMoveTargetter(Targetter* targetter)
{
    if (navswitch_push_event_p(NAVSWITCH_WEST)) {
        moveTargetter(targetter, tinygl_point(-1,0));
    }
    if (navswitch_push_event_p(NAVSWITCH_EAST)) {
        moveTargetter(targetter, tinygl_point(1, 0));
    }
    if (navswitch_push_event_p(NAVSWITCH_NORTH)) {
        moveTargetter(targetter, tinygl_point(0, -1));
    }
    if (navswitch_push_event_p(NAVSWITCH_SOUTH)) {
        moveTargetter(targetter, tinygl_point(0,1));
    }
    if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
        launchMissile(targetter);
    }
}

/* Draws placed and active ships to the LED matrix */
void drawBoard(Ship* ships, int nShips)
{
    Ship currentShip;
    for (int i = 0; i < nShips; i++) {
        currentShip = ships[i];
        if (currentShip.isPlaced || currentShip.isActive) {
            drawShip(&currentShip);
        }
    }
}

/* Resets a ship to default parameters for game reset */
void resetShip(Ship* ship) {
    ship->pos.x = DEFAULT_POS_X;
    ship->pos.y = DEFAULT_POS_Y;
    ship->isPlaced = false;
    ship->isAlive = false;
    ship->isActive = false;
}


/* Resets all ships to default parameters for game reset */
void resetBoard(Ship* ships, int nShips)
{
    for (int i = 0; i < nShips; i++) {
        resetShip(ships + i);
    }
}

/* Moves the firing targetter around the grid */
void moveTargetter(Targetter* targetter, tinygl_point_t direction)
{
    tinygl_point_t newPosition = vectorAdd(targetter->pos, direction);
    if (isWithinGrid(newPosition)) {
        targetter->pos = newPosition;
    }
}

/*  Draws the firing targetter to the screen */
void drawTargetter(Targetter* targetter)
{
    /* TODO: Put a toggle so the targetter can flash with tick
     * update maybe? */
     tinygl_draw_point(targetter->pos, 1);
}

/* Resets the targetter to defaults */
void resetTargetter(Targetter* targetter)
{
    targetter->pos = tinygl_point(DEFAULT_POS_X, DEFAULT_POS_Y);
    targetter->hasFired = false;
    targetter->hasLocalTarget = false;
}

/* Encodes vector grid point into a char */
char encodePointToChar(tinygl_point_t point)
{
    return ((BOTTOM_BOUND + 1) * point.y) + point.x;
}

/* Decodes char into grid point */
tinygl_point_t decodeCharToPoint(char c)
{
    int x = c % (BOTTOM_BOUND+1);
    int y = (c-x)/(BOTTOM_BOUND+1);
    return tinygl_point(x, y);
}

/* Sends the current targetter position as a char to the other board */
void launchMissile(Targetter* targetter)
{
    targetter->hasFired = true;
    char encodedCoordinates = encodePointToChar(targetter->pos);
    /* Send targetting coordinates to other board for fire */
    irQueueAdd(SENDING_COORDINATES, encodedCoordinates);
}


/* Checks whether the current targetter pos is currently on a ship */
bool checkHit(void)
{
    bool hit = false;
    for (int i = 0; i < nShips; i++) {
        for (int j = 0; j < ships[i].nOffsets; j++) {
            if (isEqual(targetter->pos, getGridPosition(ships + i, j))) {
                ships[i].hitStatus[j] = true;
                hit = true;
            }
        }
    }
    return hit;
}


/* Handles game state change events
 * Displays messages on statechange*/
void changeState(state_t newState)
{
    switch(newState) {
        case START_SCREEN :
            tinygl_clear();
            tinygl_text(" BATTLESHIPS: Push to start");
            break;

        case WAIT_FOR_CONNECT :
            tinygl_clear();
            tinygl_text(" Player 1 push");
            break;

        case PLACE_SHIPS :
            tinygl_clear();
            if (isPlayerOne) {
                tinygl_text("P-1 Place ships");
            } else {
                tinygl_text("P-2 Place ships");
            }
            break;

        case MY_TURN :
            tinygl_clear();
            tinygl_text("Your turn!");
            break;

        case OPPONENT_TURN :
            tinygl_clear();
            tinygl_text("Opponents turn!");
            break;

        case GAME_OVER :
            tinygl_clear();
            tinygl_text(" Game Over!");
            break;
    }
    gameState = newState;
    stateTick = 0;
}


/* taskGameRun
 * Handles the game logic task */
static void taskGameRun (void)
{
    switch (gameState) {
        case START_SCREEN:
            if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
                changeState(WAIT_FOR_CONNECT);
            }
            break;


        case WAIT_FOR_CONNECT :
            if (button_push_event_p(BUTTON1)) {
                irQueueAdd(REQUEST_PLAYER_ONE); //
                irQueueAdd(WAIT_FOR_CONFIRM);
            }
            break;


        case PLACE_SHIPS :
            /* TODO: Refactor into function */
            if (button_push_event_p(BUTTON1)) {
                // Place a ship
                placementSuccess = placeShip(currentShip, ships, nShips);
                if (placementSuccess) {
                    nShipsPlaced++;
                    currentShip++;
                    currentShip->isActive = 1;
                }
            }

            checkNavswitchMoveShip(currentShip); // I don't like this much...

            // If all ships placed, start game
            if (nShipsPlaced == nShips) {
                if (isPlayerOne == true) {
                    changeState(MY_TURN);
                } else if (isPlayerOne == false) {
                    changeState(OPPONENT_TURN);
                }
            }
            break;

        case MY_TURN :
            /* TODO: Display confirmation of hit or miss */
            checkNavswitchMoveTargetter(targetter);
            if (targetter->hasFired) {
                resetTargetter(targetter);
                changeState(OPPONENT_TURN);
            }
            break;

        case OPPONENT_TURN :
            /* TODO: Display confirmation of hit or miss */
            /* TODO: Check number of ships surviving, etc */

            if (targetter->hasLocalTarget) {
                if (checkHit() == true) {
                    // A ship has been hit
                    irQueueAdd(SEND_HIT, 0);

                    // TODO: Display "hit"
                } ;
                resetTargetter(targetter);
                changeState(MY_TURN);
            }


            // if checkWin is zero The statement change to game over
            if (checkWin() == false) {
                changeState(GAME_OVER);
            }


            break;


        case GAME_OVER :
            /* TODO: Scoring system? */
            resetBoard(ships, nShips);
            currentShip = &ships[0];
            currentShip->isActive = 1;
            nShipsPlaced = 0;
            changeState(PLACE_SHIPS);
            break;
        }
}

/* taskDisplay
 * Handles the graphics logic */
static void taskDisplay(void)
{
    switch (gameState) {
        case START_SCREEN :
            break;

        case WAIT_FOR_CONNECT :
            break;

        case PLACE_SHIPS :
            // Display message for n ticks, then display board
            if (stateTick > 5000 || navswitch_push_event_p(NAVSWITCH_PUSH)) {
                stateTick = 6000;
                tinygl_clear();
                drawBoard(ships, nShips);
            }
            break;

        case MY_TURN :
            // Display message for n ticks, then display target cursor
            if (stateTick > 3000) {
                stateTick = 5000;
                tinygl_clear();
                drawTargetter(targetter);
            }
            break;

        case OPPONENT_TURN :
            // Display message for n ticks, then display board
            if (stateTick > 3000) {
                stateTick = 5000;
                tinygl_clear();
                drawBoard(ships, nShips);
            }
            break;


        case GAME_OVER :
            break;
        }

    tinygl_update();

}

/* taskIRSendRecieve
 * Handles the sending and recieving of IR information */
void taskIRSendRecieve (void)
{
    bool isSending = false;
    char str[15];
    char data;
    if (irTick > LOOP_RATE / IR_RATE) {
        ledState = !ledState;
        irTick = 0;
        isSending = true; // We're sending this tick
    }
    if (isSending) {
        irSend();
    } else { // We're recieving
        if (ir_uart_read_ready_p ()) {
            data = ir_uart_getc();
            switch (irLastMessageRecieved) {
                case NO_MESSAGE :
                    irLastMessageRecieved = data;
                    break;

                case REQUEST_PLAYER_ONE :
                    isPlayerOne = false;


                case WAITING_FOR_RESPONSE :
                    tinygl_clear();
                    tinygl_text(str);
                    irLastMessageRecieved = NO_MESSAGE;
                    break;

                case READY_TO_SEND :
                    tinygl_clear();
                    sprintf(str, "1 %c", data);
                    tinygl_text(str);
                    irLastMessageRecieved = NO_MESSAGE;
                    break;

                case CONFIRM :
                    tinygl_clear();
                    sprintf(str, "2 %c", data);
                    tinygl_text(str);
                    irLastMessageRecieved = NO_MESSAGE;
                    break;

                case SENDING_STATE :
                    //irQueueAdd(lastMessageSent, lastData);
                    break;

                case SENDING_COORDINATES :
                    break;

                case SEND_HIT :
                    break;

                case SEND_MISS :
                    break;

            }

        }
    }
}

/* Check ship sink or not */
bool checkShipSink(ship* checkShip)
{
    bool checkSink = true;
    for (int i = 0; i < checkShip.nOffsets; i++) {
        if (ships.hitStatus[i] = true) {
            checkSink = ture;
        } else {
            checkSink = false;
            break;
        }
    }
    return checkSink;
}

/* Check Who is the winner */
bool checkWin (void) {
    int playerShipNum = 0;
    for (int i = 0; i < nShips; i++) {
        if (!checkShipSink(ships[i])) {
            playerShipNum++;
        }
    }
    return playerShipNum > 0;

}

/* Show winner*/
void showWinner(int player)
{
    tinygl_clear();
    char* message = "";
    if (player == 1) {
        message = " Winner is player 1";

    } else {
        message = " Winner is player 2";
    }
    tinygl_text(message);
}


int main(void)
{
    /* Initialize system */
    system_init();
    navswitch_init();
    button_init();
    led_init();
    tinygl_init(LOOP_RATE);
    tinygl_font_set (&font5x7_1);
    tinygl_text_speed_set (MESSAGE_RATE);
    tinygl_text_mode_set (TINYGL_TEXT_MODE_SCROLL);
    pacer_init(LOOP_RATE);
    ir_uart_init();
    for (int i = 0; i < IR_SEND_QUEUE_MAX; i++) {
        irSendQueue[i] = NO_MESSAGE;
    }
    changeState(START_SCREEN);

    /* Configure ships to use */
    ships[0] = L_ship;
    ships[1] = I_ship;
    ships[2] = i_ship;
    ships[3] = i_ship;
    ships[4] = o_ship;
    nShips = 5;

    /* Build firing targetter for MY_TURN mode */
    Targetter firing_targetter = {
        .pos={DEFAULT_POS_X, DEFAULT_POS_Y},
        .hasFired = false
    };
    targetter = &firing_targetter;

    // Set current ship to new ship
    currentShip = &ships[0];
    currentShip->isActive = 1;

    // TODO: I don't think these should be globals...?
    nShipsPlaced = 0;
    placementSuccess = false;

    ledState = 0;
    stateTick = 0;
    irTick = 0;

    while(1) {
        pacer_wait();
        navswitch_update();
        button_update();

        taskGameRun(); // Run through game logic
        taskDisplay(); // Run through display logic
        taskIRSendRecieve(); // Send and recieve data

        stateTick++; // Increment stateTick for inter state timing

        irTick++; // Increment irTick for ir send/recieve timing

        // Flash LED
        if (irTick > LOOP_RATE / IR_RATE) {
            //tick = 0;
            ledState = !ledState;
        }
        led_set(LED1, ledState);
    }

}

