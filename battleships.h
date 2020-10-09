#ifndef BATTLESHIPS
#define BATTLESHIPS

#include <stdbool.h>
#include "tinygl.h"

#define LEFT_BOUND 6
#define RIGHT_BOUND 0
#define TOP_BOUND 0
#define BOTTOM_BOUND 4
#define MAX_SHIP_OFFSETS 5
#define MESSAGE_RATE 20
#define LOOP_RATE 600
#define DEFAULT_POS_X 2
#define DEFAULT_POS_Y 3
#define TICK_SPEED 2


enum game_state{WAIT_FOR_CONNECT_MESSAGE, WAIT_FOR_CONNECT,
    PLACE_SHIPS_MESSAGE, PLACE_SHIPS_CONFIRM, PLACE_SHIPS,
    MY_TURN_MESSAGE, MY_TURN_CONFIRM, MY_TURN,
    OPPONENT_TURN_MESSAGE, OPPONENT_TURN_CONFIRM, OPPONENT_TURN, 
    GAME_OVER_MESSAGE, GAME_OVER};


/* Defines the Ship structure with ship parameters.
 * pos: Current grid position of the ships centre.
 * offsets: List of ship vertices relative to the ship's pos
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
    int nOffsets;
    bool isActive;
    bool isAlive;
    bool isPlaced;
} Ship;


typedef struct Targetter_t {
    tinygl_point_t pos;
    bool hasFired;
} Targetter;

Ship L_ship = {
    .pos={1,1},
    .offsets={{1,1}, {1,0}, {0,0}},
    .nOffsets = 3,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

Ship I_ship = {
    .pos={1,1},
    .offsets={{-1,0}, {0,0}, {1,0}},
    .nOffsets = 3,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

Ship i_ship = {
    .pos={1,1},
    .offsets={{-1,0}, {0,0}},
    .nOffsets = 2,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

Ship o_ship = {
    .pos={1,1},
    .offsets={{0,0}},
    .nOffsets = 1,
    .isActive = false,
    .isAlive = true,
    .isPlaced = false
};

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

void moveTargetter(Targetter*, tinygl_point_t);

/* Encodes vector grid point into a char */
char encodePointToChar(tinygl_point_t);

/* Decodes char into grid point */
tinygl_point_t decodeCharToPoint(char c);

/* Sends the current targetter position as a char to the other board */
void launchMissile(Targetter*);
#endif
