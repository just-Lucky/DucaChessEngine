#pragma once
#include "board.h"
#include "move.h"

struct MoveList {
    Move moves[256];
    int count = 0;

    inline void add_move(Move move) {
        moves[count] = move;
        count++;
    }
};

void generate_pseudo_legal_moves(const Board& board, MoveList& move_list);