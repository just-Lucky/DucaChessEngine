#pragma once
#include <cstdint>

extern uint64_t rook_masks[64];
extern uint64_t bishop_masks[64];
extern uint64_t rook_magics[64];
extern uint64_t bishop_magics[64];
extern int rook_shifts[64];
extern int bishop_shifts[64];

extern uint64_t rook_attacks_table[64][4096];
extern uint64_t bishop_attacks_table[64][512];