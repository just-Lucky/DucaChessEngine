#include <iostream>
#include "board.h"
#include "attacks.h"
#include "zobrist.h"
#include "tt.h"
#include "uci.h"

int main() {
    init_leaping_attacks();
    init_sliders_attacks();
    init_zobrist();
    init_tt(256);
    uci_loop();
    free_tt();
    return 0;
}