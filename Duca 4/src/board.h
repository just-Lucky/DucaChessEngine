#pragma once
#include "types.h"
#include "nnue.h"
#include <string>

struct Board {
    uint64_t bitboards[12];
    uint64_t occupancies[3]; 
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