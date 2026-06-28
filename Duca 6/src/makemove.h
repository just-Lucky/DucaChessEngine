#pragma once
#include "board.h"
#include "move.h"

// Helper that returns piece on a given square. returns -1 if empty
inline int get_piece_on_square(const Board& board, int square) {
    return board.pieces[square];
}

// returns false if the move is illegal
bool make_move(Board& board, Move move, UndoRecord& undo);

// Resets the board to the pre-move state
void unmake_move(Board& board, Move move, const UndoRecord& undo);