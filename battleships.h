/** @file   battleships.c
    @author S. Li (gli65), S.A. Heslip (she119)
    @date   10 Oct 2020
*/

#ifndef BATTLESHIPS
#define BATTLESHIPS

#include <stdbool.h>
#include "tinygl.h"

#define LEFT_BOUND 6
#define RIGHT_BOUND 0
#define TOP_BOUND 0
#define BOTTOM_BOUND 4
#define MESSAGE_RATE 30
#define IR_RATE 100
#define LOOP_RATE 600
#define MAX_SHIPS 5
#define MAX_SHIP_OFFSETS 4
#define DEFAULT_POS_X 2
#define DEFAULT_POS_Y 3


/* Enumerator for the control of the current state of the
 * game */
typedef enum state {
    START_SCREEN,       // Display game title
    WAIT_FOR_CONNECT,   // Connect with other board
    PLACE_SHIPS,        // Place ships on board
    MY_TURN,            // Run this boards turn to fire
    OPPONENT_TURN,      // Wait for other boards turn to fire
    GAME_OVER           // Game win/loss
    } state_t;


/* Defines the Ship structure with ship parameters.
 * pos: Current grid position of the ships centre.
 * offsets: List of ship vertices relative to the ship's pos
 * hitStatus: List of ship offset' status
 * nOffsets: Number of ship vertices
 * isActive: Is this ship being placed?
 * isAlive: Does this ship still have alive offsets?
 * isPlaced: Has this ship been successfully placed?
 *
 * e.g. Consider this three length 'I' ship on the game board:
 *
 *                         centre (pos)
 *                           v
 *         Ship: [-1, 0]  [0, 0]  [1, 0]
 *                  ^        ^       ^
 *           offset 0        1       2
 *
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


typedef struct Targetter_t {
    tinygl_point_t pos;
} Targetter;

Ship L_ship = {
    .pos={1,1},
    .offsets={{1,1}, {1,0}, {0,0}},
    .hitStatus = {false, false, false},
    .nOffsets = 3,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

Ship I_ship = {
    .pos={1,1},
    .offsets={{-1,0}, {0,0}, {1,0}},
    .hitStatus = {false, false, false},
    .nOffsets = 3,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

Ship i_ship = {
    .pos={1,1},
    .offsets={{-1,0}, {0,0}},
    .hitStatus = {false, false},
    .nOffsets = 2,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

Ship o_ship = {
    .pos={1,1},
    .offsets={{0,0}},
    .hitStatus = {false},
    .nOffsets = 1,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

/* Game variables */
static state_t gameState;


/* Returns the vector addition of points a and b */
tinygl_point_t vectorAdd(tinygl_point_t, tinygl_point_t);

/* Returns the grid position of a Ship's offset given an offsetIndex */
tinygl_point_t getGridPosition(Ship*, int);

/* Checks equality of two tingygl_points A and B */
bool isEqual(tinygl_point_t, tinygl_point_t);

/* Checks if point on the grid is vacant */
bool isPointVacant(tinygl_point_t, Ship*, int);

/* Checks whether a point is within the game grid or out of bounds */
bool isWithinGrid(tinygl_point_t point);

/* Rotates a Ship 90 degrees and shifts if Ship is then outside the grid*/
void rotateShip(Ship*);

/* Moves a Ship across the grid in a given direction if possible */
void moveShip(Ship*, tinygl_point_t);

/* Draws a Ship to the LED matrix */
void drawShip(Ship*);

/* Attempts to place a Ship at it's current position, returns
 * whether placement was successful or not. */
bool placeShip(Ship*, Ship*, int);

/* Performs check on navswitch for the moving and rotating of the
 * current ship for the PLACE_SHIPS mode */
void checkNavswitchMoveShip(Ship*);

/* Draws placed and active ships to the LED matrix */
void drawBoard(Ship*, int);

/* Resets a ship to default parameters for game reset */
void resetShip(Ship*);

/* Resets all ships to default parameters for game reset */
void resetBoard(Ship*, int);

/* Moves the firing targetter around the grid */
void moveTargetter(Targetter*, tinygl_point_t);

/* Encodes vector grid point into a char */
char encodePointToChar(tinygl_point_t);

/* Decodes char into grid point */
tinygl_point_t decodeCharToPoint(char);


/* Changes game state to new state */
void changeState(state_t);


/* Checks whether point is currently on a ship */
bool checkShipHit(tinygl_point_t);

/* taskGameRun
 * Handles the game logic task */
static void taskGameRun (void);

/* taskDisplay
 * Handles the graphics logic */
static void taskDisplay (void);
#endif
