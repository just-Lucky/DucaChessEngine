#pragma once
#include "board.h"
#include "types.h"
#include "move.h"

#define UNKNOWN_SCORE 100000

enum TTFlag {
    HASH_EXACT = 0, // Punteggio esatto (nessun taglio)
    HASH_ALPHA = 1, // Upper bound (la mossa era troppo scarsa, score <= alpha)
    HASH_BETA  = 2  // Lower bound (la mossa era troppo forte, score >= beta)
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