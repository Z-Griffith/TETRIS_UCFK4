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
#include "battleships.h"

/* Game state variables */
static state_t gameState;
static bool playerOne = false;
static int stateTick = 0;
static Targetter* targetter;
static Ship* currentShip;
static int nShips;
static bool placementSuccess;
static int nShipsPlaced;
static bool connectionSuccess;
static Ship ships[MAX_SHIPS];


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


/* Game Round Term Show on display round one two three*/
void roundTerm(int termNumber)
{
    //round term number start with one end with 3.
    tinygl_text_speed_set (MESSAGE_RATE);
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
        if (1) { // there have some porblems I am not sure about that the button is right.
            display = 0;
        }
    }

    free(message_text);
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
    
    char sendChar = encodePointToChar(targetter->pos); 
    
    /* Send targetting coordinates to other board for fire */
    ir_uart_putc(sendChar);
}


bool ir_connect(void)
{
    bool success = false;
    if (button_push_event_p(BUTTON1)) {
        // Set player 1
        ir_uart_putc(CONNECTION_CONFIRM);
        playerOne = true;
        success = true;
    } else if (ir_uart_read_ready_p ()) {
        char recieved = ir_uart_getc();
        // Set player 2
        if (recieved == CONNECTION_CONFIRM) {
            playerOne = false;
            success = true;
        }
    }
    return success;
}

/* Checks whether the current targetter pos is currently on a ship */
void checkHit(void)
{
    for (int i = 0; i < nShips; i++) {
        for (int j = 0; j < ships[i].nOffsets; j++) {
            if (isEqual(targetter->pos, getGridPosition(ships + i, j))) {
                ships[i].hitStatus[j] = true;
            }
        }
    }
}


/* Initialises the UCFK4 system and modules */
void initialiseSystem(void)
{
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
    changeState(START_SCREEN);
}
    
/* Changes game state to new state */
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
            if (playerOne) {
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

static void taskGameRun (void)
{
    switch (gameState) {
        case START_SCREEN:
            if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
                changeState(WAIT_FOR_CONNECT);
            }
            break;
            
            
        case WAIT_FOR_CONNECT :
            /* TODO: IR comms connect */
            
            if (!connectionSuccess) {
                connectionSuccess = ir_connect();
            }
            
            if (connectionSuccess) {
                changeState(PLACE_SHIPS);
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
            
            checkNavswitchMoveShip(currentShip);

            /* If all ships placed, start game */
            if (nShipsPlaced == nShips) {
                if (playerOne == true) {
                    changeState(MY_TURN);
                } else if (playerOne == false) {
                    changeState(OPPONENT_TURN);
                }
            }
            break;

        case MY_TURN :
            checkNavswitchMoveTargetter(targetter);
            
            if (targetter->hasFired) {
                resetTargetter(targetter);
                changeState(OPPONENT_TURN);
            }
            
            break;
        
        case OPPONENT_TURN :
            
            /* TODO: Implement wait for oppenent turn
             * and recieve data */
             
            if (ir_uart_read_ready_p ()) {
                char recieved = ir_uart_getc ();
                /* TODO: Implement message validity check? */
                 
                 /* TODO: Check ship hit */
                 
                 tinygl_point_t impactPoint = decodeCharToPoint(recieved);
                 /* TEST */
                 targetter->pos = impactPoint;
                 //state = MY_TURN;
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

static void taskDisplay(void)
{
    switch (gameState) {
        case START_SCREEN :
            break;
        
        case WAIT_FOR_CONNECT :
            break;
            
        case PLACE_SHIPS :
            if (stateTick > 5000) {
                stateTick = 6000;
                tinygl_clear();
                drawBoard(ships, nShips);
            }
            break;
            
        case MY_TURN :
            if (stateTick > 3000) {
                stateTick = 5000;
                tinygl_clear();
                drawTargetter(targetter);
            }
            break;
        
        case OPPONENT_TURN :
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




int main(void)
{
    /* Initialize system */
    initialiseSystem();
    

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

    currentShip = &ships[0];
    currentShip->isActive = 1;
    
    nShipsPlaced = 0;
    placementSuccess = false;
    connectionSuccess = false;
    
    int ledState = 0;
    int tick = 0;
    int tickSpeed = 2;
    
    

    while(1) {
        pacer_wait();
        navswitch_update();
        button_update();
        
        taskGameRun();
        taskDisplay();
        
        stateTick++;
        tick++;
        if (tick > LOOP_RATE / tickSpeed) {
            tick = 0;
            ledState = !ledState;
        }

        led_set(LED1, ledState);
    }

}

