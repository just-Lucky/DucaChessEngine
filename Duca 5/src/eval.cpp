#include "eval.h"
#include "nnue.h"
#include <cmath>
#include <algorithm>


const int pawn_pst[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     50, 50, 50, 50, 50, 50, 50, 50,
     10, 10, 20, 30, 30, 20, 10, 10,
      5,  5, 10, 25, 25, 10,  5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5, -5,-10,  0,  0,-10, -5,  5,
      5, 10, 10,-20,-20, 10, 10,  5,
      0,  0,  0,  0,  0,  0,  0,  0
};

const int knight_pst[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int bishop_pst[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

const int rook_pst[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10, 10, 10, 10, 10,  5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
     -5,  0,  0,  0,  0,  0,  0, -5,
      0,  0,  0,  5,  5,  0,  0,  0
};

const int queen_pst[64] = {
    -20,-10,-10, -5, -5,-10,-10,-20,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -10,  0,  5,  5,  5,  5,  0,-10,
     -5,  0,  5,  5,  5,  5,  0, -5,
      0,  0,  5,  5,  5,  5,  0, -5,
    -10,  5,  5,  5,  5,  5,  0,-10,
    -10,  0,  5,  0,  0,  0,  0,-10,
    -20,-10,-10, -5, -5,-10,-10,-20
};

const int king_pst[64] = {
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -10,-20,-20,-20,-20,-20,-20,-10,
     20, 20,  0,  0,  0,  0, 20, 20,
     20, 30, 10,  0,  0, 10, 30, 20
};

const int* pst_tables[6] = { pawn_pst, knight_pst, bishop_pst, rook_pst, queen_pst, king_pst };
const int piece_values_array[6] = { VAL_PAWN, VAL_KNIGHT, VAL_BISHOP, VAL_ROOK, VAL_QUEEN, VAL_KING };

const int duca_to_nnue[12] = {
    6, 5, 4, 3, 2, 1,
    12, 11, 10, 9, 8, 7
};

NNUEdata nnue_thread_state[1024];

int evaluate(const Board& board, int ply) {
    int pieces[33];
    int squares[33];
    int index = 2; 

    pieces[0] = duca_to_nnue[W_KING];
    squares[0] = __builtin_ctzll(board.bitboards[W_KING]);
    
    pieces[1] = duca_to_nnue[B_KING];
    squares[1] = __builtin_ctzll(board.bitboards[B_KING]);

    for (int p = 0; p < 12; ++p) {
        if (p == W_KING || p == B_KING) continue;
        
        uint64_t bitboard = board.bitboards[p];
        while (bitboard) {
            int sq = __builtin_ctzll(bitboard);
            pieces[index] = duca_to_nnue[p];
            squares[index] = sq;
            index++;
            bitboard &= bitboard - 1;
        }
    }
    
    pieces[index] = 0;
    squares[index] = 0;

    if (ply >= 1024) ply = 1023;

    nnue_thread_state[ply].dirtyPiece = board.dirty_piece;

    NNUEdata* nnue_data[3];
    nnue_data[0] = &nnue_thread_state[ply];
    nnue_data[1] = (ply >= 1) ? &nnue_thread_state[ply - 1] : nullptr;
    nnue_data[2] = (ply >= 2) ? &nnue_thread_state[ply - 2] : nullptr;

    if (ply == 0) {
        nnue_data[0]->accumulator.computedAccumulation = 0;
    }

    return nnue_evaluate_incremental(board.turn, pieces, squares, nnue_data); 
}