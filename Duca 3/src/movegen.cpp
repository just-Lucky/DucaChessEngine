#include "movegen.h"
#include "attacks.h"

namespace {
    inline void add_pawn_promotions(MoveList& list, int src, int tgt, bool is_capture) {
        int base_flag = is_capture ? PC_KNIGHT : PR_KNIGHT;
        list.add_move(encode_move(src, tgt, base_flag));     // N
        list.add_move(encode_move(src, tgt, base_flag + 1)); // B
        list.add_move(encode_move(src, tgt, base_flag + 2)); // R
        list.add_move(encode_move(src, tgt, base_flag + 3)); // Q
    }
}

// generates a list of pseudo-legal moves following pieces' movement patterns, move legality is later verified in make_move()
void generate_pseudo_legal_moves(const Board& board, MoveList& move_list) {
    int color = board.turn;
    int enemy = color ^ 1;
    uint64_t empty_squares = ~board.occupancies[BOTH];

    // Pawn generation
    uint64_t pawns = board.bitboards[(color == WHITE) ? W_PAWN : B_PAWN];
    
    while (pawns) {
        int src = __builtin_ctzll(pawns);
        int tgt = (color == WHITE) ? src + 8 : src - 8;
        int promo_rank = (color == WHITE) ? 7 : 0;
        int start_rank = (color == WHITE) ? 1 : 6;

        // Forward movement
        if (empty_squares & (1ULL << tgt)) {
            if (tgt / 8 == promo_rank) {
                add_pawn_promotions(move_list, src, tgt, false);
            } else {
                move_list.add_move(encode_move(src, tgt, QUIET_MOVE));
                
                // Double pawn push
                int double_tgt = (color == WHITE) ? tgt + 8 : tgt - 8;
                if ((src / 8 == start_rank) && (empty_squares & (1ULL << double_tgt))) {
                    move_list.add_move(encode_move(src, double_tgt, DOUBLE_PAWN_PUSH));
                }
            }
        }

        // Diagonal captures
        uint64_t attacks = pawn_attacks[color][src] & board.occupancies[enemy];
        while (attacks) {
            int cap_tgt = __builtin_ctzll(attacks);
            if (cap_tgt / 8 == promo_rank) {
                add_pawn_promotions(move_list, src, cap_tgt, true);
            } else {
                move_list.add_move(encode_move(src, cap_tgt, CAPTURE));
            }
            attacks &= attacks - 1;
        }

        // En passant
        if (board.enpassant != 64) {
            if (pawn_attacks[color][src] & (1ULL << board.enpassant)) {
                move_list.add_move(encode_move(src, board.enpassant, EP_CAPTURE));
            }
        }

        pawns &= pawns - 1;
    }

    // Knights, Bishops, Rooks, Queens and Kings Generation
    int start_piece = (color == WHITE) ? W_KNIGHT : B_KNIGHT;
    int end_piece = (color == WHITE) ? W_KING : B_KING;

    for (int piece = start_piece; piece <= end_piece; ++piece) {
        uint64_t bitboard = board.bitboards[piece];
        
        while (bitboard) {
            int src = __builtin_ctzll(bitboard);
            uint64_t attacks = 0ULL;

            if (piece == W_KNIGHT || piece == B_KNIGHT) attacks = knight_attacks[src];
            else if (piece == W_BISHOP || piece == B_BISHOP) attacks = get_bishop_attacks(src, board.occupancies[BOTH]);
            else if (piece == W_ROOK || piece == B_ROOK) attacks = get_rook_attacks(src, board.occupancies[BOTH]);
            else if (piece == W_QUEEN || piece == B_QUEEN) attacks = get_queen_attacks(src, board.occupancies[BOTH]);
            else if (piece == W_KING || piece == B_KING) attacks = king_attacks[src];

            attacks &= ~board.occupancies[color];

            while (attacks) {
                int tgt = __builtin_ctzll(attacks);
                int flag = (board.occupancies[enemy] & (1ULL << tgt)) ? CAPTURE : QUIET_MOVE; 
                move_list.add_move(encode_move(src, tgt, flag));
                attacks &= attacks - 1;
            }

            bitboard &= bitboard - 1;
        }
    }

    // Castle Generation
    if (color == WHITE) {
        if (board.wsc && !(board.occupancies[BOTH] & ((1ULL << F1) | (1ULL << G1)))) {
            if (!is_square_attacked(E1, BLACK, board) && !is_square_attacked(F1, BLACK, board) && !is_square_attacked(G1, BLACK, board)) {
                move_list.add_move(encode_move(E1, G1, KING_CASTLE));
            }
        }
        if (board.wlc && !(board.occupancies[BOTH] & ((1ULL << D1) | (1ULL << C1) | (1ULL << B1)))) {
            if (!is_square_attacked(E1, BLACK, board) && !is_square_attacked(D1, BLACK, board) && !is_square_attacked(C1, BLACK, board)) {
                move_list.add_move(encode_move(E1, C1, QUEEN_CASTLE));
            }
        }
    } else {
        if (board.bsc && !(board.occupancies[BOTH] & ((1ULL << F8) | (1ULL << G8)))) {
            if (!is_square_attacked(E8, WHITE, board) && !is_square_attacked(F8, WHITE, board) && !is_square_attacked(G8, WHITE, board)) {
                move_list.add_move(encode_move(E8, G8, KING_CASTLE));
            }
        }
        if (board.blc && !(board.occupancies[BOTH] & ((1ULL << D8) | (1ULL << C8) | (1ULL << B8)))) {
            if (!is_square_attacked(E8, WHITE, board) && !is_square_attacked(D8, WHITE, board) && !is_square_attacked(C8, WHITE, board)) {
                move_list.add_move(encode_move(E8, C8, QUEEN_CASTLE));
            }
        }
    }
}