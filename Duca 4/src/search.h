#pragma once
#include "board.h"
#include "move.h"
#include <chrono>
#include <atomic>

#define INF_VAL 50000
#define MATE_VALUE 49000

extern std::atomic<bool> search_aborted;
extern uint64_t game_history[2048];
extern int game_history_ply;

extern std::chrono::time_point<std::chrono::high_resolution_clock> search_start_time;
extern long long search_time_limit;

void search_position(Board& board, int max_depth);
bool is_repetition(uint64_t hash, int ply);