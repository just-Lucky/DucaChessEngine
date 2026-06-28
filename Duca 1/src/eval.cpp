#include "eval.h"
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

int evaluate(const Board& board) {
    int score = 0;
    int white_material = 0;
    int black_material = 0;

    for (int p = W_PAWN; p <= W_KING; ++p) {
        uint64_t bitboard = board.bitboards[p];
        int piece_type = p % 6;
        
        while (bitboard) {
            int sq = __builtin_ctzll(bitboard);
            score += piece_values_array[piece_type];
            score += pst_tables[piece_type][sq];

            if (piece_type != 5) {
                white_material += piece_values_array[piece_type];
            }
            
            bitboard &= bitboard - 1;
        }
    }

    for (int p = B_PAWN; p <= B_KING; ++p) {
        uint64_t bitboard = board.bitboards[p];
        int piece_type = p % 6;
        
        while (bitboard) {
            int sq = __builtin_ctzll(bitboard);
            score -= piece_values_array[piece_type];
            score -= pst_tables[piece_type][sq ^ 56];
            
            if (piece_type != 5) {
                black_material += piece_values_array[piece_type];
            }
            
            bitboard &= bitboard - 1;
        }
    }

    if (score > 300 && black_material < 1500) {
        int wk_sq = __builtin_ctzll(board.bitboards[W_KING]);
        int bk_sq = __builtin_ctzll(board.bitboards[B_KING]);
        
        int wk_rank = wk_sq / 8, wk_file = wk_sq % 8;
        int bk_rank = bk_sq / 8, bk_file = bk_sq % 8;
        
        int bk_center_dist = std::max(std::abs(bk_rank - 3), std::abs(bk_file - 3)) +
                             std::max(std::abs(bk_rank - 4), std::abs(bk_file - 4));
        
        int king_dist = std::max(std::abs(wk_rank - bk_rank), std::abs(wk_file - bk_file));

        int factor = (1500 - black_material) / 100;
        if (factor < 1) factor = 1;

        score += bk_center_dist * 10 * factor;
        score += (8 - king_dist) * 5 * factor;
    }
    else if (score < -300 && white_material < 1500) {
        int wk_sq = __builtin_ctzll(board.bitboards[W_KING]);
        int bk_sq = __builtin_ctzll(board.bitboards[B_KING]);
        
        int wk_rank = wk_sq / 8, wk_file = wk_sq % 8;
        int bk_rank = bk_sq / 8, bk_file = bk_sq % 8;
        
        int wk_center_dist = std::max(std::abs(wk_rank - 3), std::abs(wk_file - 3)) +
                             std::max(std::abs(wk_rank - 4), std::abs(wk_file - 4));

        int king_dist = std::max(std::abs(wk_rank - bk_rank), std::abs(wk_file - bk_file));
        
        int factor = (1500 - white_material) / 100;
        if (factor < 1) factor = 1;

        score -= wk_center_dist * 10 * factor;
        score -= (8 - king_dist) * 5 * factor;
    }

    return (board.turn == WHITE) ? score : -score;
}