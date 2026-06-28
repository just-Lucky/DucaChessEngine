#include <iostream>
#include <fstream>
#include <string>
#include "board.h"
#include "attacks.h"
#include "zobrist.h"
#include "tt.h"
#include "uci.h"
#include "nnue.h"

bool read_tt_size(const std::string& nomeFile, int& numero) {
    std::ifstream file(nomeFile);
    if (!file.is_open()) {
        return false;
    }
    if (file >> numero) {
        return true;
    }
    return false;
}

int main() {
    init_leaping_attacks();
    init_sliders_attacks();
    init_zobrist();
    int size = 0;
    if(read_tt_size("tt", size)){
        std::cout << "Selected Transposition Table Dimension: " << size << "MB" << std::endl;
    }else{
        std::cout << "An error occured while reading the file." << std::endl;
        return 1;
    }
    if(!init_tt(size)) return 1;
    nnue_init("nn-04cf2b4ed1da.nnue");
    uci_loop();
    free_tt();
    return 0;
}