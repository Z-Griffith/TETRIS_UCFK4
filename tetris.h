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
#define N_PIECES 7

typedef struct Tetronimo_t {
    int id;
    tinygl_point_t pos;
    int n_offsets;
    tinygl_point_t offsets[MAX_OFFSETS];
} Tetronimo;


tinygl_point_t absolute_pos(Tetronimo*, int);
tinygl_point_t vector_add_points(tinygl_point_t, tinygl_point_t);

void draw_tetronimo(Tetronimo*);
void shiftTetronimo(Tetronimo*, uint8_t*, tinygl_point_t);
void rotateTetronimo(Tetronimo*, uint8_t*, int);
void slam_tetronimo(Tetronimo*, uint8_t*);
void drawBitmap(uint8_t*);
void save_tetronimo_to_bitmap(Tetronimo*, uint8_t*);
void shift_bitmap(uint8_t*, int);
int clear_full_lines(uint8_t*);
void reset(Tetronimo**, uint8_t*);
void getNewTetronimo(Tetronimo**, Tetronimo*);
void addLine(uint8_t*);

int isPointOccupied(uint8_t*, tinygl_point_t);
int isPointWithinGrid(tinygl_point_t);
int has_tetronimo_landed(Tetronimo*, uint8_t*);
int checkGameOver(Tetronimo*);
#endif
