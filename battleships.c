/** @file   battleships.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   11 Oct 2020
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

/* -------- Static Variables --------- */ // TODO: Get rid of some of these globals
/**
 * @brief Tracks game state.
 */
static state_t gameState;

/**
 * @brief Tracks if this board is player one or not.
 */
static bool isPlayerOne;

/**
 * @brief Tracks number of ticks passed since entering a new game state.
 */
static int stateTick = 0;


/**
 * @brief Firing targetter struct
 */
static Targetter* targetter;


/**
 * @brief Tracks current ship being placed.
 */
static Ship* currentShip;


/**
 * @brief Total number of ships in the ships array.
 */
static int nShips;


/**
 * @brief Tracks if the last Ship placement was successful or not.
 */
static bool placementSuccess;


/**
 * @brief Tracks number of Ships successfully placed.
 */
static int nShipsPlaced;


/**
 * @brief Is user input on or not
 */
static bool userInputEnabled = true;

/**
 * @brief Is user input on or not
 */
static bool displayMessageEnabled = true;


/**
 * @brief Global ships array, contains all ships placed or not.
 */
static Ship ships[MAX_SHIPS];

/**
 * @brief Controls the state of the LED.
 */
static bool ledState = false;
/* -------- END STATIC VARIABLES --------- */


/**
 * @brief Returns the vector addition of points a and b.
 * @param First point vector, a.
 * @param Second point vector, b.
 * @return Vector sum of points a and b as a vector.
 */
tinygl_point_t vectorAdd(tinygl_point_t a, tinygl_point_t b)
{
    return tinygl_point(a.x + b.x, a.y + b.y);
}


/**
 * @brief Returns the grid position of a Ship's offset given an offsetIndex
 * @param Ship pointer.
 * @param offsetIndex.
 * @return Grid position as a vector.
 */
tinygl_point_t getGridPosition(Ship* ship, int offsetIndex)
{
    return vectorAdd(ship->pos, ship->offsets[offsetIndex]);
}


/**
 * @brief Checks equality between two vectors (tinygl_point_t).
 * @param First point vector, a.
 * @param Second point vector, b.
 * @return True if a and b vectors are equal.
 */
bool isEqual(tinygl_point_t pointA, tinygl_point_t pointB)
{
    return (pointA.x == pointB.x && pointA.y == pointB.y);
}


/**
 * @brief Checks if a grid point does not contain a placed ship.
 * @param Grid point as a vector.
 * @param Ships array.
 * @param number of ships in ships array.
 * @return True if point is free of ships.
 */
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


/**
 * @brief Checks if a point is within the game grid.
 * @param Grid point as a vector.
 * @return True if point is within the game grid.
 */
bool isWithinGrid(tinygl_point_t point)
{
    bool isWithinXLimits = (point.x >= TOP_BOUND && point.x <= BOTTOM_BOUND);
    bool isWithinYLimits = (point.y >= RIGHT_BOUND && point.y <= LEFT_BOUND);

    return (isWithinXLimits && isWithinYLimits);
}


/**
 * @brief Rotates a Ship 90 degrees and shifts if then outside the grid
 * @param Ship pointer.
 */
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


/**
 * @brief Moves a Ship across the grid in a given direction if possible.
 * @param Ship pointer.
 * @param Direction to move ship as a vector.
 */
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


/**
 * @brief Draws a ship to the LED matrix using TinyGL.
 * @param Ship pointer.
 */
void drawShip(Ship* ship)
{
    for (int i = 0; i < ship->nOffsets; i++) {
        // Only draw offset if not hit
        if (ship->hitStatus[i] == false) {
            tinygl_draw_point(vectorAdd(ship->pos, ship->offsets[i]), 1);
        }
    }
}


/**
 * @brief Attempts to place a Ship at it's current position.
 * @param Ship pointer.
 * @param Ships array.
 * @param Number of ships in ships array.
 * @return Returns True if placement was successful.
 */
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


/**
 * @brief Performs check on navswitch for moving and rotating a Ship.
 * @param Ship pointer.
 */
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

/**
 * @brief Performs check on navswitch for moving Targetter.
 * @param Targetter pointer.
 */
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
}


/**
 * @brief Draws placed and active ships to the LED matrix.
 * @param Ships array.
 * @param Number of ships in ships array.
 */
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


/**
 * @brief Resets a ship to default parameters for game reset.
 * @param Ships pointer.
 */
void resetShip(Ship* ship)
{
    ship->pos.x = DEFAULT_POS_X;
    ship->pos.y = DEFAULT_POS_Y;
    ship->isPlaced = false;
    ship->isAlive = false;
    ship->isActive = false;
}


/**
 * @brief Resets all ships to default parameters for game reset.
 * @param Ships array.
 * @param Number of ships in ships array.
 */
void resetBoard(Ship* ships, int nShips)
{
    for (int i = 0; i < nShips; i++) {
        resetShip(ships + i);
    }
}


/**
 * @brief Moves the firing targetter around the grid.
 * @param Targetter pointer.
 * @param Direction as a vector.
 */
void moveTargetter(Targetter* targetter, tinygl_point_t direction)
{
    tinygl_point_t newPosition = vectorAdd(targetter->pos, direction);
    if (isWithinGrid(newPosition)) {
        targetter->pos = newPosition;
    }
}


/**
 * @brief Draws the firing targetter to the LED matrix using TinyGL
 * @param Targetter pointer.
 */
void drawTargetter(Targetter* targetter)
{
    /* TODO: Put a toggle so the targetter can flash with tick
     * update maybe? */
    tinygl_draw_point(targetter->pos, 1);
}


/**
 * @brief Resets the firing targetter to its default position.
 * @param Targetter pointer.
 */
void resetTargetter(Targetter* targetter)
{
    targetter->pos = tinygl_point(DEFAULT_POS_X, DEFAULT_POS_Y);
}


/**
 * @brief Encodes vector grid point into a char for IR sending.
 * @param Grid point as a vector.
 * @return Encoded char.
 */
char encodePointToChar(tinygl_point_t point)
{
    return ((BOTTOM_BOUND + 1) * point.y) + point.x;
}


/**
 * @brief Decodes vector grid point into a char for IR recieving.
 * @param Encoded char.
 * @return Grid point as a vector.
 */
tinygl_point_t decodeCharToPoint(char c)
{
    int x = c % (BOTTOM_BOUND+1);
    int y = (c-x)/(BOTTOM_BOUND+1);
    return tinygl_point(x, y);
}


/**
 * @brief Checks whether point is currently on a ship.
 * @param Grid point as a vector.
 * @return True if a ship is located at this grid point.
 */
bool checkShipHit(tinygl_point_t impactPoint)
{
    bool hit = false;
    for (int i = 0; i < nShips; i++) {
        for (int j = 0; j < ships[i].nOffsets; j++) {
            if (isEqual(impactPoint, getGridPosition(ships + i, j))) {
                ships[i].hitStatus[j] = true;
                hit = true;
            }
        }
    }
    return hit;
}



/**
 * @brief Changes game state to new state and perform trans-state actions.
 * @param New game state.
 */
void changeState(state_t newState)
{
    displayMessageEnabled = true;

    switch(newState) {
    case START_SCREEN :
        tinygl_clear();
        tinygl_text(" BATTLESHIPS ");
        break;

    case WAIT_FOR_CONNECT :
        tinygl_clear();
        tinygl_text(" Push to connect ");
        break;

    case PLACE_SHIPS :
        userInputEnabled = false;
        displayMessageEnabled = true;
        tinygl_clear();
        tinygl_text(" Place ships ");
        break;

    case MY_TURN :
        userInputEnabled = false;
        displayMessageEnabled = true;
        resetTargetter(targetter);
        tinygl_clear();
        tinygl_text(" Your turn ");
        break;

    case WAIT_FOR_HIT_RECIEVE :
        tinygl_text(" waiting for hit.. ");
        break;

    case OPPONENT_TURN :
        userInputEnabled = false;
        displayMessageEnabled = true;
        tinygl_clear();
        tinygl_text(" Opponents turn ");
        break;

    case WAIT_FOR_HIT_SEND :
        userInputEnabled = false;
        tinygl_clear();
        tinygl_text(" waiting for confirm.. ");
        break;

    case GAME_OVER :
        tinygl_clear();
        tinygl_text(" Game Over!");
        break;
    }
    gameState = newState;
    stateTick = 0;
}


/**
 * @brief Handles the game logic, sending/recieving IR, and state changes.
 */
static void taskGameRun (void)
{
    char newMessage = '\0'; // TODO: ???!


    switch (gameState) {
    case START_SCREEN:
        if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
            changeState(WAIT_FOR_CONNECT);
        }
        break;


    case WAIT_FOR_CONNECT :
        if (button_push_event_p(BUTTON1)) {
            irRequestPlayerOne();

        } else if (irWasSentMessageReceived(REQUEST_PLAYER_ONE)) {
            // Make us player 1
            isPlayerOne = true;
            changeState(PLACE_SHIPS);

        } else if (irGetLastMessageRecieved() == REQUEST_PLAYER_ONE) {
            // Make us player 2
            irMarkMessageAsRead();
            isPlayerOne = false;
            changeState(PLACE_SHIPS);
        }


        break;


    case PLACE_SHIPS :
        if (userInputEnabled) {
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


            checkNavswitchMoveShip(currentShip); // TODO: I don't like this much...
        }

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
        if (userInputEnabled) {
            checkNavswitchMoveTargetter(targetter);
            if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
                // Fire missile at current targetter pos
                char encodedCoordinates = encodePointToChar(targetter->pos);
                irSendMissile(encodedCoordinates);
                changeState(WAIT_FOR_HIT_RECIEVE);
            }
        }
        break;


    case WAIT_FOR_HIT_RECIEVE :
        newMessage = irGetLastMessageRecieved();
        if (newMessage == SEND_HIT) {  // TODO: Display "hit"
            irMarkMessageAsRead();
            changeState(OPPONENT_TURN);
        } else if (newMessage == SEND_MISS) { // TODO: Display "miss"
            irMarkMessageAsRead();
            changeState(OPPONENT_TURN);
        } else if (stateTick > 10000) {
            // Exceeded wait time, count as miss
            changeState(OPPONENT_TURN);
        }

        break;

    case OPPONENT_TURN :
        /* TODO: Display confirmation of hit or miss */
        /* TODO: Check number of ships surviving, etc */

        newMessage = irGetLastMessageRecieved();
        if (newMessage < NO_MESSAGE) {  // Must be coordinates
            tinygl_point_t impactPoint = decodeCharToPoint(newMessage);
            irMarkMessageAsRead();
            if (checkShipHit(impactPoint)) {
                pacer_wait(); // TODO: Shouldn't be here
                irSendHit();
                changeState(WAIT_FOR_HIT_SEND);
                // TODO: Display "hit"
            } else {
                // It's a miss
                irSendMiss();
                changeState(WAIT_FOR_HIT_SEND);
                // TODO: Display "miss"
            }

        }
        break;

    case WAIT_FOR_HIT_SEND :
        // if checkWin is zero The statement change to game over
        if (checkGameLoss()) {
            changeState(GAME_OVER);
            // TODO: Send game over flag.
        }

        if (irWasSentMessageReceived(SEND_HIT) || irWasSentMessageReceived(SEND_MISS)) {
            changeState(MY_TURN);
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


/**
 * @brief Handles the graphics logic, message timeouts, etc.
 */
static void taskDisplay(void)
{
    if ((stateTick > DISPLAY_TEXT_WAIT || NAVSWITCH_PRESS) && displayMessageEnabled) {
        userInputEnabled = true;
        displayMessageEnabled = false;
    }

    switch (gameState) {
    case START_SCREEN :
        break;

    case WAIT_FOR_CONNECT :
        break;

    case PLACE_SHIPS :
        // Display message for n ticks, then display board
        if (!displayMessageEnabled) {
            tinygl_clear();
            drawBoard(ships, nShips);
        }
        break;

    case MY_TURN :
        // Display message for n ticks, then display target cursor
        if (!displayMessageEnabled) {
            tinygl_clear();
            drawTargetter(targetter);
        }
        break;


    case WAIT_FOR_HIT_RECIEVE :
        break;

    case OPPONENT_TURN :
        // Display message for n ticks, then display board
        if (!displayMessageEnabled) {
            tinygl_clear();
            drawBoard(ships, nShips);
        }
        break;

    case WAIT_FOR_HIT_SEND :
        break;


    case GAME_OVER :
        break;
    }

    tinygl_update();

}


/**
 * @brief Checks whether a Ship has sunk or not (All offsets hit).
 * @param Ship pointer.
 * @return True if all offsets are hit; ship has sank.
 */
bool isShipSunk(Ship* ship)
{
    bool hasShipSank = true;
    for (int i = 0; i < ship->nOffsets; i++) {
        if (ship->hitStatus[i] == false) {
            hasShipSank = false;
            break;
        }
    }
    return hasShipSank;
}

/**
 * @brief Checks whether this board's player has lost the game.
 * @return True if this board has all ships sunk.
 */
bool checkGameLoss (void)
{
    bool hasPlayerLost = true;
    for (int i = 0; i < nShips; i++) {
        if (isShipSunk(&ships[i]) == false) {
            // A ship hasn't sank yet, game on.
            hasPlayerLost = false;
            break;
        }
    }
    return hasPlayerLost;
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


/**
 * @brief Mainloop. Initialise all utilities and drivers and loop.
 */
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

    irInit(LOOP_RATE, IR_RATE);


    changeState(START_SCREEN);

    /* Configure ships to use */
    ships[0] = L_ship;
    //ships[1] = I_ship;
    /*
        ships[2] = i_ship;
        ships[3] = i_ship;
        ships[4] = o_ship;
    */
    nShips = 1;

    /* Build firing targetter for MY_TURN mode */
    Targetter firing_targetter = {
        .pos={DEFAULT_POS_X, DEFAULT_POS_Y}
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
    int gameTick = 0;

    while(1) {
        pacer_wait();
        navswitch_update();
        button_update();

        taskGameRun(); // Run through game logic
        taskDisplay(); // Run through display logic
        irTask();

        stateTick++; // Increment stateTick for inter state timing


        // Flash LED
        gameTick++;
        if (gameTick > LOOP_RATE / 2) {
            gameTick = 0;
            ledState = !ledState;
        }
        led_set(LED1, ledState);
    }

}

