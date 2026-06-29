#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include "board.h"
#include "attacks.h"
#include "zobrist.h"
#include "tt.h"
#include "uci.h"
#include "nnue.h"

bool read_tt_size(const std::string& fileName, int& size) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        return false;
    }
    if (file >> size) {
        return true;
    }
    return false;
}

bool read_thread_num(const std::string& fileName, int& num) {
    std::ifstream file(fileName);
    if(!file.is_open()) {
        std::cout << "An error occured while reading the file." << std::endl;
        return false;
    }
    if(file >> num){
        if(num < 1 || num > std::thread::hardware_concurrency()){
            std::cout << "The selected thread number is either less than 1 or greater than your cpu's cores.\nYour cpu has " << std::thread::hardware_concurrency() << " cores." << std::endl;
            return false;
        }
        return true;
    }
    std::cout << "An error occured while reading the file." << std::endl;
    return false;
}

int main() {
    init_leaping_attacks();
    init_sliders_attacks();
    init_zobrist();
    int size = 0;
    int num_search_threads = 0;
    if(read_tt_size("tt", size)){
        std::cout << "Selected Transposition Table Dimension: " << size << "MB" << std::endl;
    }else{
        std::cout << "An error occured while reading the file." << std::endl;
        return 1;
    }
    if(!init_tt(size)) return 1;
    if(read_thread_num("threads", num_search_threads)){
        std::cout << "Using " << num_search_threads << " search threads." << std::endl;
    }else{
        return 1;
    }
    nnue_init("nn-04cf2b4ed1da.nnue");
    uci_loop(num_search_threads);
    free_tt();
    return 0;
}