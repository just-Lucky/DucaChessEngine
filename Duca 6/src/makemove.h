#pragma once
#include "board.h"
#include "move.h"

// returns false if the move is illegal
bool make_move(Board& board, Move move, UndoRecord& undo);

// Resets the board to the pre-move state
void unmake_move(Board& board, Move move, const UndoRecord& undo);