#pragma once
#include "board.h"
#include <cstdint>

uint64_t perft_driver(Board& board, int depth);
void perft_test(Board& board, int depth);