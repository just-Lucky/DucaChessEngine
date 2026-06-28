#include "tt.h"
#include "search.h"
#include <iostream>
#include <new>

TTEntry* transposition_table = nullptr;
int tt_num_entries = 0;

bool init_tt(int megabytes) {
    switch(megabytes){
        case 256: break;
        case 512: break;
        case 1024: break;
        case 2048: break;
        default: {
            std::cout << "Transposition Table:\n\tAvailable Dimensions: 256MB, 512MB, 1024MB, 2048MB.\n\tSelected Dimension: " << megabytes << "MB.\n";
            return false;
        }
    }
    size_t bytes = (size_t)megabytes * 1024 * 1024;
    size_t max_entries = bytes / sizeof(TTEntry);
    
    size_t tt_entries = 1;
    while (tt_entries * 2 <= max_entries) {
        tt_entries *= 2;
    }
    
    if (transposition_table != nullptr) {
        delete[] transposition_table;
        transposition_table = nullptr;
    }
    tt_num_entries = static_cast<int>(tt_entries);
    transposition_table = new(std::nothrow) TTEntry[tt_num_entries];
    if(transposition_table == nullptr){
        std::cout << "Transposition Table: Insufficient Memory.\n";
        return false;
    }
    clear_tt();
    std::cout << "Transposition Table Initialized: " << (tt_num_entries * sizeof(TTEntry)) / (1024 * 1024)
              << " MB (" << tt_num_entries << " entries)\n";
    return true;
}

void clear_tt() {
    for (int i = 0; i < tt_num_entries; i++) {
        transposition_table[i].hash_key = 0;
        transposition_table[i].depth = 0;
        transposition_table[i].score = 0;
        transposition_table[i].age_and_flag = 0;
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
    TTEntry* entry = &transposition_table[hash_key & (tt_num_entries - 1)];
    uint8_t entry_flag = entry->age_and_flag & 0x3;
    if (entry->hash_key == hash_key) {
        tt_move = entry->best_move;
        entry->age_and_flag = (tt_global_age << 2) | entry_flag;
        if (entry->depth >= depth) {
            int score = entry->score;
            
            if (score > MATE_VALUE - 1000) score -= ply;
            if (score < -MATE_VALUE + 1000) score += ply;

            if (entry_flag == HASH_EXACT) return score;
            if (entry_flag == HASH_ALPHA && score <= alpha) return alpha;
            if (entry_flag == HASH_BETA && score >= beta) return beta;
        }
    }
    return UNKNOWN_SCORE;
}

void store_tt(uint64_t hash_key, int depth, int score, int flag, Move best_move, int ply) {
    TTEntry* entry = &transposition_table[hash_key & (tt_num_entries - 1)];
    bool replace = false;
    if ((entry->hash_key == hash_key && (depth >= entry->depth || flag == HASH_EXACT && (entry->age_and_flag & 0x3) != HASH_EXACT)) || (entry->age_and_flag >> 2 != tt_global_age || depth >= entry->depth)){
        replace = true;
    }
    if(!replace) return;
    if (score > MATE_VALUE - 1000) score += ply;
    if (score < -MATE_VALUE + 1000) score -= ply;

    entry->hash_key = hash_key;
    entry->depth = depth;
    entry->score = score;
    entry->best_move = best_move;
    entry->age_and_flag = (tt_global_age << 2) | (flag & 0x3);
}