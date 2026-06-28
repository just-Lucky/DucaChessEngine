#include <iostream>
#include "makemove.h"
#include "zobrist.h"
#include "attacks.h"

    // Maschera per aggiornare i diritti di arrocco in base alla casa toccata.
    // Indici: A1=0, E1=4, H1=7, A8=56, E8=60, H8=63. Gli altri non modificano l'arrocco (15).
    const int castling_rights_update[64] = {
        13, 15, 15, 15, 12, 15, 15, 14,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        7, 15, 15, 15,  3, 15, 15, 11
    };

bool make_move(Board& board, Move move, UndoRecord& undo) {
    int source = get_source(move);
    int target = get_target(move);
    int flag   = get_flag(move);
    int piece  = get_piece_on_square(board, source);
    int color  = board.turn;
    int captured = -1;

    if (is_capture(move)) {
        if (flag == EP_CAPTURE) {
            captured = (color == WHITE) ? B_PAWN : W_PAWN;
        } else {
            captured = get_piece_on_square(board, target);
        }
    }

    undo.enpassant = board.enpassant;
    undo.wsc = board.wsc; undo.wlc = board.wlc;
    undo.bsc = board.bsc; undo.blc = board.blc;
    undo.hash_key = board.hash_key;
    undo.captured_piece = captured;

    if (board.enpassant != 64) {
        board.hash_key ^= zobrist_enpassant[board.enpassant % 8];
        board.enpassant = 64;
    }

    board.bitboards[piece] ^= (1ULL << source);
    board.hash_key ^= zobrist_pieces[piece][source];

    if (captured != -1) {
        if (flag == EP_CAPTURE) {
            int ep_pawn_sq = (color == WHITE) ? target - 8 : target + 8;
            board.bitboards[captured] ^= (1ULL << ep_pawn_sq);
            board.hash_key ^= zobrist_pieces[captured][ep_pawn_sq];
        } else {
            board.bitboards[captured] ^= (1ULL << target);
            board.hash_key ^= zobrist_pieces[captured][target];
        }
    }

    if (flag >= PR_KNIGHT) {
        int promoted_piece;
        if (flag == PR_KNIGHT || flag == PC_KNIGHT) promoted_piece = (color == WHITE) ? W_KNIGHT : B_KNIGHT;
        else if (flag == PR_BISHOP || flag == PC_BISHOP) promoted_piece = (color == WHITE) ? W_BISHOP : B_BISHOP;
        else if (flag == PR_ROOK || flag == PC_ROOK) promoted_piece = (color == WHITE) ? W_ROOK : B_ROOK;
        else promoted_piece = (color == WHITE) ? W_QUEEN : B_QUEEN;
        
        board.bitboards[promoted_piece] ^= (1ULL << target);
        board.hash_key ^= zobrist_pieces[promoted_piece][target];
    }else{
        board.bitboards[piece] ^= (1ULL << target);
        board.hash_key ^= zobrist_pieces[piece][target];
    }

    if (flag == KING_CASTLE) {
        if (color == WHITE) {
            board.bitboards[W_ROOK] ^= (1ULL << H1) | (1ULL << F1);
            board.hash_key ^= zobrist_pieces[W_ROOK][H1] ^ zobrist_pieces[W_ROOK][F1];
        } else {
            board.bitboards[B_ROOK] ^= (1ULL << H8) | (1ULL << F8);
            board.hash_key ^= zobrist_pieces[B_ROOK][H8] ^ zobrist_pieces[B_ROOK][F8];
        }
    } else if (flag == QUEEN_CASTLE) {
        if (color == WHITE) {
            board.bitboards[W_ROOK] ^= (1ULL << A1) | (1ULL << D1);
            board.hash_key ^= zobrist_pieces[W_ROOK][A1] ^ zobrist_pieces[W_ROOK][D1];
        } else {
            board.bitboards[B_ROOK] ^= (1ULL << A8) | (1ULL << D8);
            board.hash_key ^= zobrist_pieces[B_ROOK][A8] ^ zobrist_pieces[B_ROOK][D8];
        }
    }

    if (flag == DOUBLE_PAWN_PUSH) {
        board.enpassant = (color == WHITE) ? target - 8 : target + 8;
        board.hash_key ^= zobrist_enpassant[board.enpassant % 8];
    }

    int old_rights = board.wsc | (board.wlc << 1) | (board.bsc << 2) | (board.blc << 3);
    int current_rights = old_rights & castling_rights_update[source] & castling_rights_update[target];
    
    board.wsc = current_rights & 1;
    board.wlc = (current_rights >> 1) & 1;
    board.bsc = (current_rights >> 2) & 1;
    board.blc = (current_rights >> 3) & 1;

    if (old_rights != current_rights) {
        board.hash_key ^= zobrist_castling[old_rights];
        board.hash_key ^= zobrist_castling[current_rights];
    }

    board.turn ^= 1;
    board.hash_key ^= zobrist_side;
    update_occupancies(board);

    int king_square = __builtin_ctzll(board.bitboards[(color == WHITE) ? W_KING : B_KING]);
    if(is_square_attacked(king_square, board.turn, board)){
        unmake_move(board, move, undo);
        return false;
    }
    return true; 
}

void unmake_move(Board& board, Move move, const UndoRecord& undo) {
    int source = get_source(move);
    int target = get_target(move);
    int flag   = get_flag(move);
    
    board.turn ^= 1;
    int color = board.turn;

    board.enpassant = undo.enpassant;
    board.wsc = undo.wsc; board.wlc = undo.wlc;
    board.bsc = undo.bsc; board.blc = undo.blc;
    board.hash_key = undo.hash_key;

    int piece;
    if (flag >= PR_KNIGHT) {
        int promoted_piece;
        if (flag == PR_KNIGHT || flag == PC_KNIGHT) promoted_piece = (color == WHITE) ? W_KNIGHT : B_KNIGHT;
        else if (flag == PR_BISHOP || flag == PC_BISHOP) promoted_piece = (color == WHITE) ? W_BISHOP : B_BISHOP;
        else if (flag == PR_ROOK || flag == PC_ROOK) promoted_piece = (color == WHITE) ? W_ROOK : B_ROOK;
        else promoted_piece = (color == WHITE) ? W_QUEEN : B_QUEEN;

        board.bitboards[promoted_piece] ^= (1ULL << target);
        piece = (color == WHITE) ? W_PAWN : B_PAWN;
    } else {
        piece = get_piece_on_square(board, target);
        board.bitboards[piece] ^= (1ULL << target);
    }

    board.bitboards[piece] ^= (1ULL << source);

    if (undo.captured_piece != -1) {
        if (flag == EP_CAPTURE) {
            int ep_pawn_sq = (color == WHITE) ? target - 8 : target + 8;
            board.bitboards[undo.captured_piece] ^= (1ULL << ep_pawn_sq);
        } else {
            board.bitboards[undo.captured_piece] ^= (1ULL << target);
        }
    }

    if (flag == KING_CASTLE) {
        if (color == WHITE) board.bitboards[W_ROOK] ^= (1ULL << H1) | (1ULL << F1);
        else board.bitboards[B_ROOK] ^= (1ULL << H8) | (1ULL << F8);
    } else if (flag == QUEEN_CASTLE) {
        if (color == WHITE) board.bitboards[W_ROOK] ^= (1ULL << A1) | (1ULL << D1);
        else board.bitboards[B_ROOK] ^= (1ULL << A8) | (1ULL << D8);
    }

    update_occupancies(board);
}