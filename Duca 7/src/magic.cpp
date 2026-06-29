#include "magic.h"

uint64_t rook_masks[64];
uint64_t bishop_masks[64];
uint64_t rook_magics[64];
uint64_t bishop_magics[64];
int rook_shifts[64];
int bishop_shifts[64];

uint64_t rook_attacks_table[64][4096];
uint64_t bishop_attacks_table[64][512];