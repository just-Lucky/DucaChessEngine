#pragma once
#include "board.h"
#include "types.h"
#include "move.h"

#define UNKNOWN_SCORE 100000

enum TTFlag {
    HASH_EXACT = 0,
    HASH_ALPHA = 1,
    HASH_BETA  = 2 
};

struct TTEntry {
    uint64_t hash_key;
    int depth;
    int score;
    int flag;
    Move best_move;
};

extern TTEntry* transposition_table;
extern int tt_num_entries;

void init_tt(int megabytes);
void clear_tt();
void free_tt();

int probe_tt(uint64_t hash_key, int depth, int alpha, int beta, Move& tt_move, int ply);
void store_tt(uint64_t hash_key, int depth, int score, int flag, Move best_move, int ply);