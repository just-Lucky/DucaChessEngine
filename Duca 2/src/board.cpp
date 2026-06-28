#include "board.h"
#include "zobrist.h"
#include <iostream>
#include <iomanip>

void update_occupancies(Board& board) {
    board.occupancies[WHITE] = 0ULL;
    board.occupancies[BLACK] = 0ULL;
    board.occupancies[BOTH]  = 0ULL;

    for (int piece = W_PAWN; piece <= W_KING; ++piece) {
        board.occupancies[WHITE] |= board.bitboards[piece];
    }
    for (int piece = B_PAWN; piece <= B_KING; ++piece) {
        board.occupancies[BLACK] |= board.bitboards[piece];
    }
    
    board.occupancies[BOTH] = board.occupancies[WHITE] | board.occupancies[BLACK];
}

void initialize_starter_board(Board& board) {
    for (int i = 0; i < 12; ++i) {
        board.bitboards[i] = 0ULL;
    }

    board.bitboards[W_PAWN]   = 0x000000000000FF00ULL;
    board.bitboards[W_ROOK]   = square_to_bit(A1) | square_to_bit(H1);
    board.bitboards[W_KNIGHT] = square_to_bit(B1) | square_to_bit(G1);
    board.bitboards[W_BISHOP] = square_to_bit(C1) | square_to_bit(F1);
    board.bitboards[W_QUEEN]  = square_to_bit(D1);
    board.bitboards[W_KING]   = square_to_bit(E1);

    board.bitboards[B_PAWN]   = 0x00FF000000000000ULL;
    board.bitboards[B_ROOK]   = square_to_bit(A8) | square_to_bit(H8);
    board.bitboards[B_KNIGHT] = square_to_bit(B8) | square_to_bit(G8);
    board.bitboards[B_BISHOP] = square_to_bit(C8) | square_to_bit(F8);
    board.bitboards[B_QUEEN]  = square_to_bit(D8);
    board.bitboards[B_KING]   = square_to_bit(E8);

    board.turn = WHITE;
    board.enpassant = 64; // out of range number -> no enpassant
    board.move = 0;
    board.wsc = board.wlc = board.bsc = board.blc = 1;
    update_occupancies(board);
    board.hash_key = generate_hash(board);
}


void print_bitboard(uint64_t bitboard) {
    std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << rank + 1 << " | ";
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            int bit = (bitboard >> square) & 1ULL;
            std::cout << bit << " | ";
        }
        std::cout << "\n  +---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "    a   b   c   d   e   f   g   h\n\n";
}

void parse_fen(Board& board, const std::string& fen) {
    for (int i = 0; i < 12; ++i) board.bitboards[i] = 0ULL;
    board.occupancies[WHITE] = 0ULL;
    board.occupancies[BLACK] = 0ULL;
    board.occupancies[BOTH]  = 0ULL;
    board.turn = WHITE;
    board.enpassant = 64;
    board.wsc = board.wlc = board.bsc = board.blc = 0;
    board.hash_key = 0ULL;

    size_t i = 0;

    int rank = 7; // FEN starts from the eighth rank
    int file = 0; // and the first file

    while (rank >= 0 && i < fen.length() && fen[i] != ' ') {
        char c = fen[i];
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += (c - '0');
        } else {
            int piece = -1;
            if (c == 'P')      piece = W_PAWN;
            else if (c == 'N') piece = W_KNIGHT;
            else if (c == 'B') piece = W_BISHOP;
            else if (c == 'R') piece = W_ROOK;
            else if (c == 'Q') piece = W_QUEEN;
            else if (c == 'K') piece = W_KING;
            else if (c == 'p') piece = B_PAWN;
            else if (c == 'n') piece = B_KNIGHT;
            else if (c == 'b') piece = B_BISHOP;
            else if (c == 'r') piece = B_ROOK;
            else if (c == 'q') piece = B_QUEEN;
            else if (c == 'k') piece = B_KING;

            if (piece != -1) {
                int square = rank * 8 + file;
                board.bitboards[piece] |= (1ULL << square);
                file++;
            }
        }
        i++;
    }

    if (i < fen.length() && fen[i] == ' ') i++;

    // Parses the game turn
    if (i < fen.length()) {
        board.turn = (fen[i] == 'w') ? WHITE : BLACK;
        i++;
    }

    if (i < fen.length() && fen[i] == ' ') i++;

    // Parses castling rights
    while (i < fen.length() && fen[i] != ' ') {
        char c = fen[i];
        if (c == 'K') board.wsc = 1;
        else if (c == 'Q') board.wlc = 1;
        else if (c == 'k') board.bsc = 1;
        else if (c == 'q') board.blc = 1;
        i++;
    }

    if (i < fen.length() && fen[i] == ' ') i++;

    // Parses en passant square
    if (i < fen.length() && fen[i] != ' ') {
        if (fen[i] == '-') {
            board.enpassant = 64;
            i++;
        } else if (i + 1 < fen.length()) {
            int f = fen[i] - 'a';
            int r = fen[i+1] - '1';
            board.enpassant = r * 8 + f;
            i += 2;
        }
    }
    update_occupancies(board);
    board.hash_key = generate_hash(board);
}