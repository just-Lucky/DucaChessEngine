#pragma once
#include "board.h"
#include "move.h"

// Helper per trovare quale pezzo è su una casa data, ritorna -1 se vuota
inline int get_piece_on_square(const Board& board, int square) {
    uint64_t bit = 1ULL << square;
    if (!(board.occupancies[BOTH] & bit)) return -1;
    for (int p = W_PAWN; p <= B_KING; ++p) {
        if (board.bitboards[p] & bit) return p;
    }
    return -1;
}

// ritorna false se la mossa lascia il re in presa
bool make_move(Board& board, Move move, UndoRecord& undo);

// Ripristina istantaneamente la scacchiera allo stato pre-mossa
void unmake_move(Board& board, Move move, const UndoRecord& undo);