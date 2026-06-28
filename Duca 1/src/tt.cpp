#include "tt.h"
#include "search.h"
#include <iostream>

TTEntry* transposition_table = nullptr;
int tt_num_entries = 0;

void init_tt(int megabytes) {
    int bytes = megabytes * 1024 * 1024;
    tt_num_entries = bytes / sizeof(TTEntry);
    
    if (transposition_table != nullptr) {
        delete[] transposition_table;
    }
    
    transposition_table = new TTEntry[tt_num_entries];
    clear_tt();
    std::cout << "Transposition Table Inizializzata: " << megabytes 
              << " MB (" << tt_num_entries << " entries)\n";
}

void clear_tt() {
    for (int i = 0; i < tt_num_entries; i++) {
        transposition_table[i].hash_key = 0;
        transposition_table[i].depth = 0;
        transposition_table[i].score = 0;
        transposition_table[i].flag = 0;
        transposition_table[i].best_move = 0;
    }
}

void free_tt() {
    if (transposition_table != nullptr) {
        delete[] transposition_table;
        transposition_table = nullptr;
    }
}

int probe_tt(uint64_t hash_key, int depth, int alpha, int beta, Move& tt_move, int ply) {
    TTEntry* entry = &transposition_table[hash_key % tt_num_entries];

    if (entry->hash_key == hash_key) {
        tt_move = entry->best_move;

        if (entry->depth >= depth) {
            int score = entry->score;
            
            if (score > MATE_VALUE - 1000) score -= ply;
            if (score < -MATE_VALUE + 1000) score += ply;

            if (entry->flag == HASH_EXACT) return score;
            if (entry->flag == HASH_ALPHA && score <= alpha) return alpha;
            if (entry->flag == HASH_BETA && score >= beta) return beta;
        }
    }
    return UNKNOWN_SCORE;
}

void store_tt(uint64_t hash_key, int depth, int score, int flag, Move best_move, int ply) {
    TTEntry* entry = &transposition_table[hash_key % tt_num_entries];

    if (score > MATE_VALUE - 1000) score += ply;
    if (score < -MATE_VALUE + 1000) score -= ply;

    entry->hash_key = hash_key;
    entry->depth = depth;
    entry->score = score;
    entry->flag = flag;
    entry->best_move = best_move;
}