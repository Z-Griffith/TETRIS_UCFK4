/** @file   battleships.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   30 Sep 2020
*/

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "system.h"
#include "button.h"
#include "navswitch.h"
#include "led.h"
#include "tinygl.h"
#include "pacer.h"
#include "ir_uart.h"
#include "../fonts/font5x7_1.h"
#include "battleships.h"

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
bool isEqual(tinygl_point_t pointA, tinygl_point_t pointB)
{
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

/* Rotates a Ship 90 degrees */
void rotateShip(Ship* ship)
{
    tinygl_point_t newOffset;
    tinygl_point_t offsetGridPosition;

    // Rotate each offset using 2D rotation matrix
    for (int i = 0; i < ship->nOffsets; i++) {
        newOffset = tinygl_point(-(ship->offsets[i]).y, (ship->offsets[i]).x);

        ship->offsets[i] = newOffset;

        offsetGridPosition = getGridPosition(ship, i);

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
        tinygl_draw_point(vectorAdd(ship->pos, ship->offsets[i]), 1);
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

void resetShip(Ship* ship)
{
    ship->pos.x = DEFAULT_POS_X;
    ship->pos.y = DEFAULT_POS_Y;
    ship->isPlaced = false;
    ship->isAlive = false;
    ship->isActive = false;
}

/* Game Round Term Show on display round one two three*/
void roundTerm(int termNumber)
{
    //round term number start with one end with 3.
    /* TODO: Initialise tinygl. */
    tinygl_init (500); //PACER_RATE
    tinygl_font_set (&font5x7_1);
    tinygl_text_speed_set (25); //MESSAGE_RATE
    tinygl_text_mode_set (TINYGL_TEXT_MODE_SCROLL);

    /* TODO: Set the message using tinygl_text().  */
    char* part_of_text = "Round ";
    char t_number = (char) (termNumber + '0'); // int to char
    char* message_text = malloc(sizeof(char)*7);//9 char we need now.
    strncpy(message_text, part_of_text, 6);
    message_text[6] = t_number;
    message_text[7] = 0;

    tinygl_text(message_text);
    pacer_init (500);
    button_init ();
    int display = 1;
    while(display)
    {
        pacer_wait();
        tinygl_update();
        if (button_pressed_p ()) { // there have some porblems I am not sure about that the button is right.
            display = 0;
        }
    }

    free(message_text);
    return 0;
}

int main(void)
{
    system_init();
    navswitch_init();
    button_init();
    led_init();
    tinygl_init(LOOP_RATE);
    pacer_init(LOOP_RATE);

    enum game_state state = PLACE_SHIPS;


    Ship ships[] = {
        L_ship,
        I_ship,
        i_ship,
        i_ship,
        o_ship,
        o_ship
    };

    Ship* currentShip = &ships[0];
    currentShip->isActive = 1;
    int nShips = 6;
    int nShipsPlaced = 0;
    bool placementSuccess = false;
    int ledState = 0;
    int tick = 0;
    int termNumber = 1;



    while(1) {
        pacer_wait();
        navswitch_update();
        button_update();
        switch (state) {
        case WAIT_FOR_CONNECT :
            /* TODO: IR comms connect */
            continue;
            break;

        case PLACE_SHIPS :
            /* TODO: Refactor into function */
            tinygl_clear();
            drawBoard(ships, nShips);
            if (button_push_event_p(BUTTON1)) {
                // Place a ship
                placementSuccess = placeShip(currentShip, ships, nShips);
                if (placementSuccess) {
                    nShipsPlaced++;
                    currentShip++;
                    currentShip->isActive = 1;
                }
            }
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


            if (nShipsPlaced == nShips) {
                state = GAME_RUN;
            }
            break;

        case GAME_RUN :
            /* TODO: */
            drawBoard(ships, nShips);
            break;

        case GAME_OVER:
            continue;
            break;
        }

        tick++;
        if (tick > LOOP_RATE / TICK_SPEED) {
            tick = 0;
            ledState = !ledState;
        }

        led_set(LED1, ledState);
        tinygl_update();
    }

}

