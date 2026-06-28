#pragma once
#include "types.h"
#include "board.h"

const uint64_t not_A_file  = 0xFEFEFEFEFEFEFEFEULL; // Tutte le colonne tranne la A
const uint64_t not_H_file  = 0x7F7F7F7F7F7F7F7FULL; // Tutte le colonne tranne la H
const uint64_t not_AB_file = 0xFCFCFCFCFCFCFCFCULL; // Tutte tranne A e B
const uint64_t not_GH_file = 0x3F3F3F3F3F3F3F3FULL; // Tutte tranne G e H


// Conterranno le mosse pre-calcolate per ogni singola casa (da 0 a 63)
extern uint64_t knight_attacks[64];
extern uint64_t king_attacks[64];
extern uint64_t pawn_attacks[2][64]; // [0] = Bianco, [1] = Nero

uint64_t get_rook_attacks(int square, uint64_t occupancy);
uint64_t get_bishop_attacks(int square, uint64_t occupancy);
uint64_t get_queen_attacks(int square, uint64_t occupancy);


void init_leaping_attacks();
void init_sliders_attacks();
bool is_square_attacked(int square, int attacker_side, const Board& board);