/** @file   tetris.h
    @authors S. Heslip (she119), S. Li (gli65)
    @date   11 Oct 2020
*/

#ifndef TETRIS_H
#define TETRIS_H

#include <stdlib.h>
#include "tinygl.h"

#define WALL_LEFT -1
#define WALL_RIGHT 5
#define WALL_FLOOR 7
#define WALL_TOP -1
#define SPAWN_POINT 2, 0
#define MAX_OFFSETS 4
#define N_TETRONIMOS 7

/* Declares the Tetronimo struct to hold active tetronimo */
typedef struct Tetronimo_t {
    tinygl_point_t pos;
    int nOffsets;
    tinygl_point_t offsets[MAX_OFFSETS];
} Tetronimo;

/* Returns the absolute position of a point on a Tetronimo */
tinygl_point_t getGridPos(Tetronimo*, int);


/* Returns the vector addition of two points */
tinygl_point_t vectorAddPoints(tinygl_point_t, tinygl_point_t);


/* Draws a Tetronimo to the LED matrix */
void drawTetronimo(Tetronimo*);


/* Moves a Tetronimo by shift vector if possible */
void shiftTetronimo(Tetronimo*, uint8_t*, tinygl_point_t);


/* Rotates a Tetronimo +-90 degrees determined by isClockwise paramter */
void rotateTetronimo(Tetronimo*, uint8_t*, int);


/* Pushes a tetronimo to the bottom of the grid in one frame */
void slamTetronimo(Tetronimo*, uint8_t*);


/* Checks the board for game over condition (tetronimo height > grid height) */
int checkGameOver(Tetronimo*);


/* Checks if the given coordinates are currently occupied */
int isPointOccupied(uint8_t*, tinygl_point_t);


/* Returns true if point vector is within grid bounds */
int isPointWithinGrid(tinygl_point_t);


/* Draws the bitmap patterns of the landed tetronimos to the screen using tinygl */
void drawBitmap(uint8_t*);


/* Shifts the rows of the bitmap downwards when a line is cleared */
void shiftBitmap(uint8_t*, int);


/* Checks whether the Tetronimo has impacted the ground or the pile */
int hasTetronimoLanded(Tetronimo*, uint8_t*);


/* Converts the points of the Tetronimo to the bitmap encoding*/
void saveTetronimoToBitmap(Tetronimo*, uint8_t*);


/* Clears completed lines from the game, returning number of lines cleared */
int clearFullLines(uint8_t*);


/* Resets the game */
void reset(Tetronimo**, uint8_t*);


/* Retrieves a fresh random Tetronimo from the Tetronimo allTetronimos array */
void getNewTetronimo(Tetronimo**, Tetronimo*);


/* Adds a broken line to the bottom of the bitmap */
void addLine(uint8_t*);

#endif
