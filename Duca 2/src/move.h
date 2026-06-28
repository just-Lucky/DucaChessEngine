#pragma once
#include "types.h"

// Source square, Target square and move flag are packed into 16 bits
inline Move encode_move(int source, int target, int flag) {
    return (source) | (target << 6) | (flag << 12);
}

// Extracts the source square(first 6 bits)
inline int get_source(Move move) {
    return move & 0x3F;
}

// Extracts the target square(next 6 bits)
inline int get_target(Move move) {
    return (move >> 6) & 0x3F;
}

// Extracts the move flag(last 4 bits)
inline int get_flag(Move move) {
    return (move >> 12) & 0xF;
}

// checks if the move is any type of capture
inline bool is_capture(Move move) {
    int flag = get_flag(move);
    return (flag == CAPTURE || flag == EP_CAPTURE || flag >= PC_KNIGHT);
}