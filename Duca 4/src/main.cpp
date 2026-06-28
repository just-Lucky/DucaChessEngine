#include <iostream>
#include "board.h"
#include "attacks.h"
#include "zobrist.h"
#include "tt.h"
#include "uci.h"
#include "nnue.h"
// #include "perft.h"
// #include "eval.h"
// #include "search.h"

int main() {
    init_leaping_attacks();
    init_sliders_attacks();
    init_zobrist();
    init_tt(256);
    nnue_init("nn-04cf2b4ed1da.nnue");
    uci_loop();
    free_tt();
    return 0;
}