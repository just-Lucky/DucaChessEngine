#pragma once
#include "types.h"
#include "nnue.h"
#include <string>

struct Board {
    uint64_t bitboards[12];
    uint64_t occupancies[3]; 
    int pieces[64];
    int turn;
    int enpassant; // square where an enpassant is available (0-63 or 64 if no enpassant is available)
    int move;
    int wsc, wlc, bsc, blc;
    uint64_t hash_key;
    DirtyPiece dirty_piece;
};

struct UndoRecord {
    int enpassant;
    int wsc, wlc, bsc, blc;
    uint64_t hash_key;
    int captured_piece; // -1 for quiet moves
};

inline uint64_t square_to_bit(int square) { return 1ULL << square; }
void initialize_starter_board(Board& board);
void print_bitboard(uint64_t bitboard);
void update_occupancies(Board& board);
void parse_fen(Board& board, const std::string& fen);
inline void rebuild_pieces(Board& board) {
    for (int i = 0; i < 64; i++) board.pieces[i] = -1;
    for (int p = 0; p < 12; p++) {
        uint64_t bb = board.bitboards[p];
        while (bb) {
            int sq = __builtin_ctzll(bb);
            board.pieces[sq] = p;
            bb &= bb - 1;
        }
    }
}