#ifndef BATTLESHIPS
#define BATTLESHIPS

#include <stdbool.h>
#include "tinygl.h"

#define LEFT_BOUND 6
#define RIGHT_BOUND 0
#define TOP_BOUND 0
#define BOTTOM_BOUND 4
#define MAX_SHIP_OFFSETS 5
#define MAX_SHIPS_ON_BOARD 10
#define LOOP_RATE 300
#define DEFAULT_POS_X 2
#define DEFAULT_POS_Y 3
#define TICK_SPEED 2


enum game_state{WAIT_FOR_CONNECT, PLACE_SHIPS, GAME_RUN, GAME_OVER};

typedef struct Ship_t {
    tinygl_point_t pos;
    tinygl_point_t offsets[MAX_SHIP_OFFSETS];
    int nOffsets;
    bool isActive;
    bool isAlive;
    bool isPlaced;
} Ship;


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

tinygl_point_t vectorAdd(tinygl_point_t, tinygl_point_t);
tinygl_point_t getGridPosition(Ship*, int); 
bool isEqual(tinygl_point_t, tinygl_point_t);
bool isPointVacant(tinygl_point_t, Ship*, int);
bool isWithinGrid(tinygl_point_t point);
void rotateShip(Ship*);
void moveShip(Ship*, tinygl_point_t);
void drawShip(Ship*);
bool placeShip(Ship*, Ship*, int);
void drawGameBoard(Ship*, int);


#endif
