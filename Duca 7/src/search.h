#pragma once
#include "board.h"
#include "move.h"
#include <chrono>
#include <atomic>

#define INF_VAL 50000
#define MATE_VALUE 49000

struct ThreadData {
    int id = 0;
    Move killer_moves[2][64] = {};
    int history_moves[2][6][64] = {};
    Move counter_move_table[64] = {};
    uint64_t search_history[128] = {};
    long long nodes_searched = 0;
    long long quiescence_nodes = 0;
    Move root_best_move = 0;
    int root_score = 0;
};

extern std::atomic<bool> search_aborted;
extern uint64_t game_history[2048];
extern int game_history_ply;

extern std::chrono::time_point<std::chrono::high_resolution_clock> search_start_time;
extern long long search_time_limit;

void search_position(Board& board, int max_depth, int num_search_threads);
bool is_repetition(uint64_t hash, int ply);