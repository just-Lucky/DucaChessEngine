#pragma once
#include "board.h"
#include <cstdint>

extern uint64_t zobrist_pieces[12][64];
extern uint64_t zobrist_enpassant[8];
extern uint64_t zobrist_castling[16];
extern uint64_t zobrist_side;

void init_zobrist();
uint64_t generate_hash(const Board& board);