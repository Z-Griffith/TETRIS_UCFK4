/** @file   tetris.c
    @authors S. Heslip (she119), S. Li (gli65)
    @date   11 Oct 2020
*/
#include "tinygl.h"
#include "tetris.h"


/* Returns the absolute position of a point on a Tetronimo */
tinygl_point_t getGridPos(Tetronimo* tetr, int offset_index)
{
    return vectorAddPoints(tetr->pos, tetr->offsets[offset_index]);
}

/* Returns the vector addition of two points */
tinygl_point_t vectorAddPoints(tinygl_point_t a, tinygl_point_t b)
{
    tinygl_point_t newPoint = {a.x + b.x, a.y + b.y};
    return newPoint;
}

/* Draws a Tetronimo to the LED matrix */
void draw_tetronimo(Tetronimo* tetr)
{
    for (int i = 0; i < tetr->nOffsets; i++) {
        tinygl_draw_point(getGridPos(tetr, i), 1);
    }
}

/* Moves a Tetronimo by shift vector if possible */
void shiftTetronimo(Tetronimo* tetr, uint8_t* bitmap, tinygl_point_t shift)
{
    // Check validity of this shift
    int shift_is_valid = 1;
    for (int i = 0; i < tetr->nOffsets; i++) {
        tinygl_point_t abs_pos = getGridPos(tetr, i);
        tinygl_point_t point_after_move = vectorAddPoints(abs_pos, shift);
        if (isPointOccupied(bitmap, point_after_move) || !isPointWithinGrid(point_after_move)) {
            // Shift is in valid, terminate the shift
            shift_is_valid = 0;
            break;
        }
    }
            
    // Shift Tetronimo
    if (shift_is_valid) {
        tetr->pos = vectorAddPoints(tetr->pos, shift);
    }
}

void slamTetronimo(Tetronimo* tetr, uint8_t* bitmap)
{
    while(has_tetronimo_landed(tetr, bitmap) == 0) {
        shiftTetronimo(tetr, bitmap, tinygl_point(0, 1));
    }
}


int checkGameOver(Tetronimo* tetr)
{
    int isGameOver = 0;
    for (int i = 0; i < tetr->nOffsets; i++) {
        if (getGridPos(tetr, i).y <= (WALL_TOP+1)) {
            isGameOver = 1;
            break;
        }
    }
    return isGameOver;
}


/* Rotates a Tetronimo +-90 degrees */
void rotateTetronimo(Tetronimo* tetr, uint8_t* bitmap, int isClockwise)
{
    tinygl_point_t newOffset;
    int isValidRotation = 1;
    Tetronimo temp;
    temp.pos = tetr->pos;
    temp.id = tetr->id;
    // Rotate each offset using 2D rotation matrix
    for (int i = 0; i < tetr->nOffsets; i++) {
        if (isClockwise) {
            newOffset = tinygl_point(-(tetr->offsets[i]).y, (tetr->offsets[i]).x);
        } else {
            newOffset = tinygl_point((tetr->offsets[i]).y, -(tetr->offsets[i]).x);
        }
        temp.offsets[i] = newOffset;
        if (isPointOccupied(bitmap, getGridPos(&temp, i))) {
            isValidRotation = 0;
            break;
        }
    }
    
    if (isValidRotation) {
        for (int i = 0; i < tetr->nOffsets; i++) {
            
            tetr->offsets[i] = temp.offsets[i];
        }
    
        // Check new positions and shift if needed
        for (int i = 0; i < tetr->nOffsets; i++) {
            // If one of the offsets is in left wall, shift tetronimo right
            if(getGridPos(tetr, i).x <= WALL_LEFT) {
                shiftTetronimo(tetr, bitmap, tinygl_point(1,0));
                break;
            }
            // ^^ right wall, shift left
            if(getGridPos(tetr, i).x >= WALL_RIGHT) {
                shiftTetronimo(tetr, bitmap, tinygl_point(-1,0));
                break;
            }
        }
    }
}

/* Checks if the given coordinates are currently occupied */
int isPointOccupied(uint8_t* bitmap, tinygl_point_t point) 
{
    if ((bitmap[point.y] >> point.x) & 1) {
        return 1;
    } else {
        return 0;
    }
}

int isPointWithinGrid(tinygl_point_t point) 
{
    if (point.x > WALL_LEFT && point.x < WALL_RIGHT && point.y < WALL_FLOOR) {
        return 1;
    } else {
        return 0;
    } 
}

/* Checks whether the Tetronimo has impacted the ground or the pile */
int has_tetronimo_landed(Tetronimo* tetr, uint8_t* bitmap)
{
    for (int i = 0; i < tetr->nOffsets; i++) {
        tinygl_point_t abs_pos = getGridPos(tetr, i);
        tinygl_point_t point_below = vectorAddPoints(abs_pos, tinygl_point(0, 1));
        if (isPointOccupied(bitmap, point_below) || point_below.y >= WALL_FLOOR) {
            return 1;
        } 
    }
    return 0;
}

void shift_bitmap(uint8_t* bitmap, int clearedRowIdx)
{
    for (int iRow = clearedRowIdx; iRow > 0; iRow--) {
        *(bitmap + iRow) = bitmap[iRow-1];
    }
}


/* Clears completed lines from the game
 * Returns number of lines cleared */
int clear_full_lines(uint8_t* bitmap)
{
    int nLinesCleared = 0;
    for (int iRow = 0; iRow < WALL_FLOOR; iRow++) {
        if (bitmap[iRow] == 0x1F) {
            nLinesCleared++;
            *(bitmap + iRow) = 0x00;
            shift_bitmap(bitmap, iRow);
        }
    }
    return nLinesCleared;
}


/* Draws the bitmap of rows to the LED matrix */
void drawBitmap(uint8_t* bitmap)
{
    for (int iRow = 0; iRow < WALL_FLOOR; iRow++) {
        uint8_t current_row = bitmap[iRow]; 
        
        for (int iCol = 0; iCol < 5; iCol++) {
            if ((current_row >> iCol) & 1) {
                tinygl_draw_point(tinygl_point(iCol, iRow), 1);
            } else {
                tinygl_draw_point(tinygl_point(iCol, iRow), 0);
            }
        }
    }
}

/* Converts the points of the Tetronimo to the bitmap encoding*/ 
void save_tetronimo_to_bitmap(Tetronimo* tetr, uint8_t* bitmap)
{
    for (int iRow = 0; iRow < WALL_FLOOR; iRow++) { 
    
        for (int iCol = 0; iCol < WALL_RIGHT; iCol++) {
            
            for (int i = 0; i < tetr->nOffsets; i++) {
                tinygl_point_t currentPoint = getGridPos(tetr, i);
                
                if (currentPoint.y == iRow && currentPoint.x == iCol) {
                    *(bitmap + iRow) = bitmap[iRow] | (1 << iCol);
                }
            }
        }
    }
}

/* Retrieves a fresh random Tetronimo from the Tetronimo pieces array */
void getNewTetronimo(Tetronimo** tetr, Tetronimo* pieces) 
{
    int randInt = rand() % N_PIECES;
    (pieces + randInt)->pos = tinygl_point(SPAWN_POINT);
    *tetr = &pieces[randInt];
}

/* Resets the game */
void reset(Tetronimo** tetr, uint8_t* bitmap) {
    *tetr = NULL;
    for (int iRow = 0; iRow < 7; iRow++) {
        bitmap[iRow] = 0x00;
    }
}

/* Adds a broken line to the bottom of the bitmap */
void addLine(uint8_t* bitmap)
{
    for (int iRow = 0; iRow < 6; iRow++) {
        *(bitmap + iRow) = bitmap[iRow+1];
    }
    
    *(bitmap + 6) = 0x15;
}


