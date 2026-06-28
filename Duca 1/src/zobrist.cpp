#include "zobrist.h"

uint64_t zobrist_pieces[12][64];
uint64_t zobrist_enpassant[8];
uint64_t zobrist_castling[16];
uint64_t zobrist_side;

// PRNG Xorshift64 Generator
uint64_t random_uint64() {
    static uint64_t seed = 0x98f1071ab85db23fULL;
    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;
    return seed * 0x2545F4914F6CDD1DULL;
}

// Initializes all tables with random 64-bit numbers
void init_zobrist() {
    for (int p = 0; p < 12; p++) {
        for (int s = 0; s < 64; s++) {
            zobrist_pieces[p][s] = random_uint64();
        }
    }
    for (int f = 0; f < 8; f++) {
        zobrist_enpassant[f] = random_uint64();
    }
    for (int c = 0; c < 16; c++) {
        zobrist_castling[c] = random_uint64();
    }
    zobrist_side = random_uint64();
}

// Generates the hask key from scratch by analyzing the whole board
uint64_t generate_hash(const Board& board) {
    uint64_t hash = 0ULL;
    
    for (int p = 0; p < 12; p++) {
        uint64_t bitboard = board.bitboards[p];
        while (bitboard) {
            int sq = __builtin_ctzll(bitboard); 
            hash ^= zobrist_pieces[p][sq];
            bitboard &= bitboard - 1;
        }
    }
    
    if (board.enpassant != 64) {
        int file = board.enpassant % 8;
        hash ^= zobrist_enpassant[file];
    }
    
    int castling_rights = (board.wsc) | (board.wlc << 1) | (board.bsc << 2) | (board.blc << 3);
    hash ^= zobrist_castling[castling_rights];

    if (board.turn == BLACK) {
        hash ^= zobrist_side;
    }
    
    return hash;
}