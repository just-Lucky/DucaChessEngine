#include <iostream>
#include "attacks.h"
#include "magic.h"

uint64_t knight_attacks[64];
uint64_t king_attacks[64];
uint64_t pawn_attacks[2][64];

uint64_t mask_knight_attacks(int square) {
    uint64_t attacks = 0ULL;
    uint64_t bitboard = 1ULL << square;

    // Upward movements (Left: << bit)
    if ((bitboard << 17) & not_A_file)  attacks |= (bitboard << 17); // 2 Up, 1 Right
    if ((bitboard << 15) & not_H_file)  attacks |= (bitboard << 15); // 2 Up, 1 Left
    if ((bitboard << 10) & not_AB_file) attacks |= (bitboard << 10); // 1 Up, 2 Right
    if ((bitboard <<  6) & not_GH_file) attacks |= (bitboard <<  6); // 1 Up, 2 Left

    // Downward movements (Right: >> bit)
    if ((bitboard >> 15) & not_A_file)  attacks |= (bitboard >> 15); // 2 Down, 1 Right
    if ((bitboard >> 17) & not_H_file)  attacks |= (bitboard >> 17); // 2 Down, 1 Left
    if ((bitboard >>  6) & not_AB_file) attacks |= (bitboard >>  6); // 1 Down, 2 Right
    if ((bitboard >> 10) & not_GH_file) attacks |= (bitboard >> 10); // 1 Down, 2 Left

    return attacks;
}

uint64_t mask_king_attacks(int square) {
    uint64_t attacks = 0ULL;
    uint64_t bitboard = 1ULL << square;

    // Movements
    if (bitboard >> 8) attacks |= (bitboard >> 8); // Down
    if ((bitboard >> 9) & not_H_file) attacks |= (bitboard >> 9); // Down-Left
    if ((bitboard >> 7) & not_A_file) attacks |= (bitboard >> 7); // Down-Right
    if ((bitboard >> 1) & not_H_file) attacks |= (bitboard >> 1); // Left

    if (bitboard << 8) attacks |= (bitboard << 8); // Up
    if ((bitboard << 9) & not_A_file) attacks |= (bitboard << 9); // Up-Right
    if ((bitboard << 7) & not_H_file) attacks |= (bitboard << 7); // Up-Left
    if ((bitboard << 1) & not_A_file) attacks |= (bitboard << 1); // Right

    return attacks;
}

uint64_t mask_pawn_attacks(int color, int square) {
    uint64_t attacks = 0ULL;
    uint64_t bitboard = 1ULL << square;

    if (color == WHITE) {
        // Up-Left attack
        if ((bitboard << 7) & not_H_file) attacks |= (bitboard << 7);
        // Up-Right attack
        if ((bitboard << 9) & not_A_file) attacks |= (bitboard << 9);
    }
    else {
        // Down-Left attack
        if ((bitboard >> 9) & not_H_file) attacks |= (bitboard >> 9);
        // Down-Right attack
        if ((bitboard >> 7) & not_A_file) attacks |= (bitboard >> 7);
    }

    return attacks;
}

uint64_t get_rook_attacks(int square, uint64_t occupancy) {
    occupancy &= rook_masks[square];
    occupancy *= rook_magics[square];
    occupancy >>= rook_shifts[square];
    return rook_attacks_table[square][occupancy];
}

uint64_t get_bishop_attacks(int square, uint64_t occupancy) {
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magics[square];
    occupancy >>= bishop_shifts[square];
    return bishop_attacks_table[square][occupancy];
}

uint64_t get_queen_attacks(int square, uint64_t occupancy) {
    return get_rook_attacks(square, occupancy) | get_bishop_attacks(square, occupancy);
}


void init_leaping_attacks() {
    for (int square = 0; square < 64; ++square) {
        knight_attacks[square] = mask_knight_attacks(square);
        king_attacks[square] = mask_king_attacks(square);
        
        pawn_attacks[WHITE][square] = mask_pawn_attacks(WHITE, square);
        pawn_attacks[BLACK][square] = mask_pawn_attacks(BLACK, square);
    }
}
    uint64_t prng_magic() {
        static uint64_t seed = 0x98f1071ab85db23fULL;
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        return seed * 0x2545F4914F6CDD1DULL;
    }

    uint64_t random_uint64_fewbits() {
        return prng_magic() & prng_magic() & prng_magic();
    }

    int count_bits(uint64_t b) {
        int count = 0;
        while (b) { count++; b &= b - 1; }
        return count;
    }

    uint64_t generate_rook_mask(int square) {
        uint64_t mask = 0ULL;
        int tr = square / 8, tf = square % 8;
        for (int r = tr + 1; r <= 6; r++) mask |= (1ULL << (r * 8 + tf));
        for (int r = tr - 1; r >= 1; r--) mask |= (1ULL << (r * 8 + tf));
        for (int f = tf + 1; f <= 6; f++) mask |= (1ULL << (tr * 8 + f));
        for (int f = tf - 1; f >= 1; f--) mask |= (1ULL << (tr * 8 + f));
        return mask;
    }

    uint64_t generate_bishop_mask(int square) {
        uint64_t mask = 0ULL;
        int tr = square / 8, tf = square % 8;
        for (int r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) mask |= (1ULL << (r * 8 + f));
        for (int r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) mask |= (1ULL << (r * 8 + f));
        for (int r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) mask |= (1ULL << (r * 8 + f));
        for (int r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) mask |= (1ULL << (r * 8 + f));
        return mask;
    }

    uint64_t generate_rook_attacks_on_the_fly(int square, uint64_t occupancy) {
        uint64_t attacks = 0ULL;
        int tr = square / 8, tf = square % 8;
        for (int r = tr + 1; r <= 7; r++) { attacks |= (1ULL << (r * 8 + tf)); if (occupancy & (1ULL << (r * 8 + tf))) break; }
        for (int r = tr - 1; r >= 0; r--) { attacks |= (1ULL << (r * 8 + tf)); if (occupancy & (1ULL << (r * 8 + tf))) break; }
        for (int f = tf + 1; f <= 7; f++) { attacks |= (1ULL << (tr * 8 + f)); if (occupancy & (1ULL << (tr * 8 + f))) break; }
        for (int f = tf - 1; f >= 0; f--) { attacks |= (1ULL << (tr * 8 + f)); if (occupancy & (1ULL << (tr * 8 + f))) break; }
        return attacks;
    }

    uint64_t generate_bishop_attacks_on_the_fly(int square, uint64_t occupancy) {
        uint64_t attacks = 0ULL;
        int tr = square / 8, tf = square % 8;
        for (int r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) { attacks |= (1ULL << (r * 8 + f)); if (occupancy & (1ULL << (r * 8 + f))) break; }
        for (int r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) { attacks |= (1ULL << (r * 8 + f)); if (occupancy & (1ULL << (r * 8 + f))) break; }
        for (int r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) { attacks |= (1ULL << (r * 8 + f)); if (occupancy & (1ULL << (r * 8 + f))) break; }
        for (int r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) { attacks |= (1ULL << (r * 8 + f)); if (occupancy & (1ULL << (r * 8 + f))) break; }
        return attacks;
    }

    uint64_t set_occupancy(int index, int bits_in_mask, uint64_t attack_mask) {
        uint64_t occupancy = 0ULL;
        for (int i = 0; i < bits_in_mask; i++) {
            uint64_t bit = attack_mask & -attack_mask;
            attack_mask ^= bit;
            if (index & (1 << i)) occupancy |= bit;
        }
        return occupancy;
    }

    uint64_t find_magic(int square, int relevant_bits, bool is_rook) {
        uint64_t mask = is_rook ? generate_rook_mask(square) : generate_bishop_mask(square);
        int permutations = 1 << relevant_bits;
        
        uint64_t occupancies[4096];
        uint64_t attacks[4096];
        uint64_t used_attacks[4096];
        int used_epoch[4096] = {0};
        
        for (int i = 0; i < permutations; i++) {
            occupancies[i] = set_occupancy(i, relevant_bits, mask);
            attacks[i] = is_rook ? generate_rook_attacks_on_the_fly(square, occupancies[i])
                                 : generate_bishop_attacks_on_the_fly(square, occupancies[i]);
        }

        int current_epoch = 0;
        for (int k = 0; k < 100000000; k++) {
            uint64_t magic_candidate = random_uint64_fewbits();
            current_epoch++;
            bool success = true;
            
            for (int i = 0; i < permutations; i++) {
                int magic_index = (occupancies[i] * magic_candidate) >> (64 - relevant_bits);
                
                if (used_epoch[magic_index] < current_epoch) {
                    used_epoch[magic_index] = current_epoch;
                    used_attacks[magic_index] = attacks[i];
                } else if (used_attacks[magic_index] != attacks[i]) {
                    success = false;
                    break;
                }
            }
            if (success) return magic_candidate;
        }
        return 0ULL;
    }
void init_sliders_attacks() {
    for (int square = 0; square < 64; square++) {

        rook_masks[square] = generate_rook_mask(square);
        int r_bits = count_bits(rook_masks[square]);
        rook_shifts[square] = 64 - r_bits;
        rook_magics[square] = find_magic(square, r_bits, true);
        
        int r_permutations = 1 << r_bits;
        for (int i = 0; i < r_permutations; i++) {
            uint64_t occ = set_occupancy(i, r_bits, rook_masks[square]);
            uint64_t magic_index = (occ * rook_magics[square]) >> rook_shifts[square];
            rook_attacks_table[square][magic_index] = generate_rook_attacks_on_the_fly(square, occ);
        }
        
        bishop_masks[square] = generate_bishop_mask(square);
        int b_bits = count_bits(bishop_masks[square]);
        bishop_shifts[square] = 64 - b_bits;
        bishop_magics[square] = find_magic(square, b_bits, false);
        
        int b_permutations = 1 << b_bits;
        for (int i = 0; i < b_permutations; i++) {
            uint64_t occ = set_occupancy(i, b_bits, bishop_masks[square]);
            uint64_t magic_index = (occ * bishop_magics[square]) >> bishop_shifts[square];
            bishop_attacks_table[square][magic_index] = generate_bishop_attacks_on_the_fly(square, occ);
        }
    }
}

bool is_square_attacked(int square, int attacker_side, const Board& board) {
    // Pawn attacks
    int defender_side = attacker_side ^ 1;
    int attacker_pawn = (attacker_side == WHITE) ? W_PAWN : B_PAWN;
    if (pawn_attacks[defender_side][square] & board.bitboards[attacker_pawn]) return true;

    // Knight attacks
    int attacker_knight = (attacker_side == WHITE) ? W_KNIGHT : B_KNIGHT;
    if (knight_attacks[square] & board.bitboards[attacker_knight]) return true;

    // King attacks
    int attacker_king = (attacker_side == WHITE) ? W_KING : B_KING;
    if (king_attacks[square] & board.bitboards[attacker_king]) return true;

    // Diagonal attacks
    int attacker_bishop = (attacker_side == WHITE) ? W_BISHOP : B_BISHOP;
    int attacker_queen  = (attacker_side == WHITE) ? W_QUEEN : B_QUEEN;
    uint64_t diagonal_attackers = board.bitboards[attacker_bishop] | board.bitboards[attacker_queen];
    
    if (get_bishop_attacks(square, board.occupancies[BOTH]) & diagonal_attackers) return true;

    // Straight attacks
    int attacker_rook = (attacker_side == WHITE) ? W_ROOK : B_ROOK;
    uint64_t straight_attackers = board.bitboards[attacker_rook] | board.bitboards[attacker_queen];
    
    if (get_rook_attacks(square, board.occupancies[BOTH]) & straight_attackers) return true;

    return false;
}