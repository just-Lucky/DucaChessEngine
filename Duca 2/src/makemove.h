#pragma once
#include "board.h"
#include "move.h"

// Helper that returns the piece on a given square. Returns -1 if empty
inline int get_piece_on_square(const Board& board, int square) {
    uint64_t bit = 1ULL << square;
    if (!(board.occupancies[BOTH] & bit)) return -1;
    for (int p = W_PAWN; p <= B_KING; ++p) {
        if (board.bitboards[p] & bit) return p;
    }
    return -1;
}

// returns false if the move is illegal
bool make_move(Board& board, Move move, UndoRecord& undo);

// Resets the board to the pre-move state
void unmake_move(Board& board, Move move, const UndoRecord& undo);