#pragma once
#include "board.h"
#include "move.h"

// Helper per trovare quale pezzo è su una casa data, ritorna -1 se vuota
inline int get_piece_on_square(const Board& board, int square) {
    return board.pieces[square];
}

// ritorna false se la mossa lascia il re in presa
bool make_move(Board& board, Move move, UndoRecord& undo);

// Ripristina istantaneamente la scacchiera allo stato pre-mossa
void unmake_move(Board& board, Move move, const UndoRecord& undo);