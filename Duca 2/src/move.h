#pragma once
#include "types.h"

// Unisce partenza, arrivo e flag in un solo numero a 16 bit
inline Move encode_move(int source, int target, int flag) {
    return (source) | (target << 6) | (flag << 12);
}

// Estrae la casa di partenza (i primi 6 bit)
inline int get_source(Move move) {
    return move & 0x3F;
}

// Estrae la casa di arrivo (i secondi 6 bit)
inline int get_target(Move move) {
    return (move >> 6) & 0x3F;
}

// Estrae il flag (gli ultimi 4 bit)
inline int get_flag(Move move) {
    return (move >> 12) & 0xF;
}

// controlla se la mossa è una cattura (qualsiasi tipo)
inline bool is_capture(Move move) {
    int flag = get_flag(move);
    return (flag == CAPTURE || flag == EP_CAPTURE || flag >= PC_KNIGHT);
}