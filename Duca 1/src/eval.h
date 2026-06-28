#pragma once
#include "board.h"

enum PieceValues {
    VAL_PAWN   = 100,
    VAL_KNIGHT = 300,
    VAL_BISHOP = 320,
    VAL_ROOK   = 500,
    VAL_QUEEN  = 900,
    VAL_KING   = 10000
};
extern const int piece_values_array[6];
extern const int* pst_tables[6];
int evaluate(const Board& board);