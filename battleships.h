/** @file   battleships.h
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   11 Oct 2020
*/

#ifndef BATTLESHIPS_H
#define BATTLESHIPS_H

#include <stdbool.h>
#include "tinygl.h"

#define LEFT_BOUND 6
#define RIGHT_BOUND 0
#define TOP_BOUND 0
#define BOTTOM_BOUND 4
#define MESSAGE_RATE 30
#define IR_RATE 10
#define LOOP_RATE 300
#define MAX_SHIPS 5
#define MAX_SHIP_OFFSETS 4
#define DEFAULT_POS_X 2
#define DEFAULT_POS_Y 3

/** 
 * @brief Enumerator of states for the control of the current state of the game
 */
typedef enum state {
    START_SCREEN,           // Display game title
    WAIT_FOR_CONNECT,       // Connect with other board
    PLACE_SHIPS,            // Place ships on board
    MY_TURN,                // Run this boards turn to fire
    WAIT_FOR_HIT_RECIEVE,   // Wait until missile results recieved
    OPPONENT_TURN,          // Wait for other boards turn to fire
    WAIT_FOR_HIT_SEND,       // Wait until missile results sent
    GAME_OVER               // Game win/loss
    } state_t;


/** 
 * @brief Declares the Ship structure with ship parameters for keeping
 * track of ship location, hit status, placement status.
 * 
 *      e.g. Consider this 'I' ship on the game board:
 *
 *                         centre (pos)
 *                           v
 *         Ship: [-1, 0]  [0, 0]  [1, 0]
 *                  ^        ^       ^
 *           offset 0        1       2
 *
 * @param pos: Current grid position of the ships centre.
 * @param offsets: List of ship vertices relative to the ship's pos
 * @param hitStatus: List of ship offset' status
 * @param nOffsets: Number of ship vertices
 * @param isActive: Is this ship being placed?
 * @param isAlive: Does this ship still have alive offsets?
 * @param isPlaced: Has this ship been successfully placed?

 */
typedef struct Ship_t {
    tinygl_point_t pos;
    tinygl_point_t offsets[MAX_SHIP_OFFSETS];
    bool hitStatus[MAX_SHIP_OFFSETS];
    int nOffsets;
    bool isActive;
    bool isAlive;
    bool isPlaced;
} Ship;


/** 
 * @brief Declares the Targetter struct, for targetting next missle shot.
 */
typedef struct Targetter_t {
    tinygl_point_t pos;
} Targetter;



/** 
 * @brief 'L' type Ship with three offsets:
 * Ship Diagram:
 *                     [][]
 *                       []
 */
Ship L_ship = {
    .pos={1,1},
    .offsets={{1,1}, {1,0}, {0,0}},
    .hitStatus = {false, false, false},
    .nOffsets = 3,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

/** 
 * @brief 'I' type Ship with three offsets:
 * Ship Diagram:
 *                     [][][]
 */
Ship I_ship = {
    .pos={1,1},
    .offsets={{-1,0}, {0,0}, {1,0}},
    .hitStatus = {false, false, false},
    .nOffsets = 3,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

/** 
 * @brief 'i' type Ship with two offsets:
 * Ship Diagram:
 *                     [][]
 */
Ship i_ship = {
    .pos={1,1},
    .offsets={{-1,0}, {0,0}},
    .hitStatus = {false, false},
    .nOffsets = 2,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

/** 
 * @brief Smallest ship; the 'o' type ship with one offset:
 * Ship Diagram:
 *                       []
 */
Ship o_ship = {
    .pos={1,1},
    .offsets={{0,0}},
    .hitStatus = {false},
    .nOffsets = 1,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

/** 
 * @brief Tracks which state the game is in e.g.
 *       START_SCREEN: Display 'Battleships' and wait for keypress.
 */
static state_t gameState;


/** 
 * @brief Returns the vector addition of points a and b.
 * @param First point vector, a.
 * @param Second point vector, b.
 * @return Vector sum of points a and b as a vector.
 */
tinygl_point_t vectorAdd(tinygl_point_t, tinygl_point_t);


/** 
 * @brief Returns the grid position of a Ship's offset given an offsetIndex
 * @param Ship pointer.
 * @param offsetIndex.
 * @return Grid position as a vector.
 */
tinygl_point_t getGridPosition(Ship*, int);


/** 
 * @brief Checks equality between two vectors (tinygl_point_t).
 * @param First point vector, a.
 * @param Second point vector, b.
 * @return True if a and b vectors are equal.
 */
bool isEqual(tinygl_point_t, tinygl_point_t);


/** 
 * @brief Checks if a grid point does not contain a placed ship.
 * @param Grid point as a vector.
 * @param Ships array.
 * @param number of ships in ships array.
 * @return True if point is free of ships.
 */
bool isPointVacant(tinygl_point_t, Ship*, int);


/** 
 * @brief Checks if a point is within the game grid.
 * @param Grid point as a vector.
 * @return True if point is within the game grid.
 */
bool isWithinGrid(tinygl_point_t);


/** 
 * @brief Rotates a Ship 90 degrees and shifts if then outside the grid
 * @param Ship pointer.
 */
void rotateShip(Ship*);


/** 
 * @brief Moves a Ship across the grid in a given direction if possible.
 * @param Ship pointer.
 * @param Direction to move ship as a vector.
 */
void moveShip(Ship*, tinygl_point_t);


/** 
 * @brief Draws a ship to the LED matrix using TinyGL.
 * @param Ship pointer.
 */
void drawShip(Ship*);


/** 
 * @brief Attempts to place a Ship at it's current position.
 * @param Ship pointer.
 * @param Ships array.
 * @param Number of ships in ships array.
 * @return Returns True if placement was successful.
 */
bool placeShip(Ship*, Ship*, int);


/** 
 * @brief Performs check on navswitch for moving and rotating a Ship.
 * @param Ship pointer.
 */
void checkNavswitchMoveShip(Ship*);


/** 
 * @brief Performs check on navswitch for moving Targetter.
 * @param Targetter pointer.
 */
void checkNavswitchMoveTargetter(Targetter*);


/** 
 * @brief Draws placed and active ships to the LED matrix.
 * @param Ships array.
 * @param Number of ships in ships array.
 */
void drawBoard(Ship*, int);


/** 
 * @brief Resets a ship to default parameters for game reset.
 * @param Ships pointer.
 */
void resetShip(Ship*);


/** 
 * @brief Resets all ships to default parameters for game reset.
 * @param Ships array.
 * @param Number of ships in ships array.
 */
void resetBoard(Ship*, int);


/** 
 * @brief Moves the firing targetter around the grid.
 * @param Targetter pointer.
 * @param Direction as a vector.
 */
void moveTargetter(Targetter*, tinygl_point_t);


/** 
 * @brief Encodes vector grid point into a char for IR sending.
 * @param Grid point as a vector.
 * @return Encoded char.
 */
char encodePointToChar(tinygl_point_t);


/** 
 * @brief Decodes vector grid point into a char for IR recieving.
 * @param Encoded char.
 * @return Grid point as a vector.
 */
tinygl_point_t decodeCharToPoint(char);


/** 
 * @brief Changes game state to new state and perform trans-state actions.
 * @param New game state.
 */
void changeState(state_t);


/** 
 * @brief Checks whether point is currently on a ship.
 * @param Grid point as a vector.
 * @return True if a ship is located at this grid point.
 */
bool checkShipHit(tinygl_point_t);


/** 
 * @brief Handles the game logic, sending/recieving IR, and state changes.
 */
//void taskGameRun (void);


/** 
 * @brief Handles the graphics logic, message timeouts, etc.
 */
//void taskDisplay (void);


/** 
 * @brief Checks whether this board's player has lost the game.
 * @return True if this board has all ships sunk.
 */
bool checkGameLoss (void);


/** 
 * @brief Checks whether a Ship has sunk or not (All offsets hit).
 * @param Ship pointer.
 * @return True if all offsets are hit; ship has sank.
 */
bool isShipSunk(Ship*);
#endif
