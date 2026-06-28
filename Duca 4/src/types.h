#pragma once
#include <cstdint>

enum MoveFlags {
    QUIET_MOVE = 0,         // Normal move
    DOUBLE_PAWN_PUSH = 1,   // Pawn pushed two squares from their starting rank
    KING_CASTLE = 2,        // Short castle
    QUEEN_CASTLE = 3,       // Long Castle
    CAPTURE = 4,            // Capture
    EP_CAPTURE = 5,         // En Passant Capture
    
    // Promotions without capture
    PR_KNIGHT = 8,
    PR_BISHOP = 9,
    PR_ROOK = 10,
    PR_QUEEN = 11,
    
    // Promotions with capture
    PC_KNIGHT = 12,
    PC_BISHOP = 13,
    PC_ROOK = 14,
    PC_QUEEN = 15
};

enum Squares {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};

enum Pieces {
    W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
};

enum Colors {
    WHITE, BLACK, BOTH
};

typedef uint16_t Move;