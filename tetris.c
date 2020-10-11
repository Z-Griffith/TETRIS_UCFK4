/** @file   tetris.c
    @authors S. Heslip (she119), S. Li (gli65)
    @date   11 Oct 2020
*/
#include "tinygl.h"
#include "tetris.h"


/* Returns the absolute position of a point on a Tetronimo */
tinygl_point_t absolute_pos(Tetronimo* tetr, int offset_index)
{
    return vector_add_points(tetr->pos, tetr->offsets[offset_index]);
}

/* Returns the vector addition of two points */
tinygl_point_t vector_add_points(tinygl_point_t a, tinygl_point_t b)
{
    tinygl_point_t newPoint = {a.x + b.x, a.y + b.y};
    return newPoint;
}

/* Draws a Tetronimo to the LED matrix */
void draw_tetronimo(Tetronimo* tetr)
{
    for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
        tinygl_draw_point(absolute_pos(tetr, offset_idx), 1);
    }
}

/* Moves a Tetronimo by shift vector if possible */
void shift_tetronimo(Tetronimo* tetr, uint8_t* bitmap, tinygl_point_t shift)
{
    // Check validity of this shift
    int shift_is_valid = 1;
    for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
        tinygl_point_t abs_pos = absolute_pos(tetr, offset_idx);
        tinygl_point_t point_after_move = vector_add_points(abs_pos, shift);
        if (is_point_occupied(bitmap, point_after_move) || !is_point_within_grid(point_after_move)) {
            // Shift is in valid, terminate the shift
            shift_is_valid = 0;
            break;
        }
    }
            
    // Shift Tetronimo
    if (shift_is_valid) {
        tetr->pos = vector_add_points(tetr->pos, shift);
    }
}

void slam_tetronimo(Tetronimo* tetr, uint8_t* bitmap)
{
    while(has_tetronimo_landed(tetr, bitmap) == 0) {
        shift_tetronimo(tetr, bitmap, tinygl_point(0, 1));
    }
}


int checkGameOver(Tetronimo* tetr)
{
    int is_game_over = 0;
    for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
        if (absolute_pos(tetr, offset_idx).y <= (WALL_TOP+1)) {
            is_game_over = 1;
            break;
        }
    }
    return is_game_over;
}


/* Rotates a Tetronimo +-90 degrees */
void rotate_tetronimo(Tetronimo* tetr, uint8_t* bitmap, int isClockwise)
{
    tinygl_point_t new_offset;
    int is_valid_rotation = 1;
    Tetronimo temp;
    temp.pos = tetr->pos;
    temp.id = tetr->id;
    // Rotate each offset using 2D rotation matrix
    for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
        if (isClockwise) {
            new_offset = tinygl_point(-(tetr->offsets[offset_idx]).y, (tetr->offsets[offset_idx]).x);
        } else {
            new_offset = tinygl_point((tetr->offsets[offset_idx]).y, -(tetr->offsets[offset_idx]).x);
        }
        temp.offsets[offset_idx] = new_offset;
        if (is_point_occupied(bitmap, absolute_pos(&temp, offset_idx))) {
            is_valid_rotation = 0;
            break;
        }
    }
    
    if (is_valid_rotation) {
        for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
            
            tetr->offsets[offset_idx] = temp.offsets[offset_idx];
        }
    
        // Check new positions and shift if needed
        for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
            // If one of the offsets is in left wall, shift tetronimo right
            if(absolute_pos(tetr, offset_idx).x <= WALL_LEFT) {
                shift_tetronimo(tetr, bitmap, tinygl_point(1,0));
                break;
            }
            // ^^ right wall, shift left
            if(absolute_pos(tetr, offset_idx).x >= WALL_RIGHT) {
                shift_tetronimo(tetr, bitmap, tinygl_point(-1,0));
                break;
            }
        }
    }
}

/* Checks if the given coordinates are currently occupied */
int is_point_occupied(uint8_t* bitmap, tinygl_point_t point) 
{
    if ((bitmap[point.y] >> point.x) & 1) {
        return 1;
    } else {
        return 0;
    }
}

int is_point_within_grid(tinygl_point_t point) 
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
    for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
        tinygl_point_t abs_pos = absolute_pos(tetr, offset_idx);
        tinygl_point_t point_below = vector_add_points(abs_pos, tinygl_point(0, 1));
        if (is_point_occupied(bitmap, point_below) || point_below.y >= WALL_FLOOR) {
            return 1;
        } 
    }
    return 0;
}

void shift_bitmap(uint8_t* bitmap, int cleared_row_idx)
{
    for (int row_idx = cleared_row_idx; row_idx > 0; row_idx--) {
        *(bitmap + row_idx) = bitmap[row_idx-1];
    }
}


/* Clears completed lines from the game
 * Returns number of lines cleared */
int clear_full_lines(uint8_t* bitmap)
{
    int nLinesCleared = 0;
    for (int row_idx = 0; row_idx < WALL_FLOOR; row_idx++) {
        if (bitmap[row_idx] == 0x1F) {
            nLinesCleared++;
            *(bitmap + row_idx) = 0x00;
            shift_bitmap(bitmap, row_idx);
        }
    }
    return nLinesCleared;
}


/* Draws the bitmap of rows to the LED matrix */
void drawBitmap(uint8_t* bitmap)
{
    for (int row_idx = 0; row_idx < WALL_FLOOR; row_idx++) {
        uint8_t current_row = bitmap[row_idx]; 
        
        for (int col_idx = 0; col_idx < 5; col_idx++) {
            if ((current_row >> col_idx) & 1) {
                tinygl_draw_point(tinygl_point(col_idx, row_idx), 1);
            } else {
                tinygl_draw_point(tinygl_point(col_idx, row_idx), 0);
            }
        }
    }
}

/* Converts the points of the Tetronimo to the bitmap encoding*/ 
void save_tetronimo_to_bitmap(Tetronimo* tetr, uint8_t* bitmap)
{
    for (int row_idx = 0; row_idx < WALL_FLOOR; row_idx++) { 
    
        for (int col_idx = 0; col_idx < WALL_RIGHT; col_idx++) {
            
            for (int offset_idx = 0; offset_idx < tetr->n_offsets; offset_idx++) {
                tinygl_point_t current_point = absolute_pos(tetr, offset_idx);
                
                if (current_point.y == row_idx && current_point.x == col_idx) {
                    *(bitmap + row_idx) = bitmap[row_idx] | (1 << col_idx);
                }
            }
        }
    }
}

/* Retrieves a fresh random Tetronimo from the Tetronimo pieces array */
void getNewTetronimo(Tetronimo** tetr, Tetronimo* pieces) 
{
    int rand_int = rand() % N_PIECES;
    (pieces + rand_int)->pos = tinygl_point(SPAWN_POINT);
    *tetr = &pieces[rand_int];
}

void reset(Tetronimo** tetr, uint8_t* bitmap) {
    *tetr = NULL;
    for (int row_idx = 0; row_idx < 7; row_idx++) {
        bitmap[row_idx] = 0x00;
    }
}

void addLine(uint8_t* bitmap)
{
    for (int row_idx = 0; row_idx < 6; row_idx++) {
        *(bitmap + row_idx) = bitmap[row_idx+1];
    }
    
    *(bitmap + 6) = 0x15;
}

/* Defines the encoded bitmap */

//* Defines each Tetronimo piece and each offset */
//static Tetronimo pieces[] = {
    //{0, {0,0}, {{-1,1}, {-1,0}, {0,0}, {1,0}}}, // J piece
    //{1, {0,0}, {{-1,0}, {-1,1}, {0,0}, {0,1}}}, // Square piece
    //{2, {0,0}, {{-1,0}, { 0,1}, {0,0}, {1,0}}}, // T piece
    //{3, {0,0}, {{-1,0}, { 0,0}, {1,0}, {1,1}}}, // L piece
    //{4, {0,0}, {{-1,0}, { 0,0}, {1,0}, {2,0}}}, // I piece
    //{5, {0,0}, {{-1,0}, { 0,0}, {0,1}, {1,1}}}, // S piece
    //{6, {0,0}, {{-1,1}, { 0,1}, {0,0}, {1,0}}}, // Z piece
//};

/* Defines each Tetronimo piece and each offset */

