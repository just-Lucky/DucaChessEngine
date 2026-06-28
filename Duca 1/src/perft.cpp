#include "perft.h"
#include "movegen.h"
#include "makemove.h"
#include <iostream>
#include <chrono>
#include <iomanip>

uint64_t perft_driver(Board& board, int depth) {
    if (depth == 0) return 1ULL;

    MoveList move_list;
    generate_pseudo_legal_moves(board, move_list);

    uint64_t nodes = 0;
    for (int i = 0; i < move_list.count; i++) {
        UndoRecord undo;
        if (!make_move(board, move_list.moves[i], undo)) continue; 
        
        nodes += perft_driver(board, depth - 1);
        unmake_move(board, move_list.moves[i], undo);
    }
    return nodes;
}


void perft_test(Board& board, int depth) {
    std::cout << "\n========================================\n";
    std::cout << "  STARTING PERFT TEST (Depth " << depth << ")\n"; 
    std::cout << "========================================\n";
    
    auto start_time = std::chrono::high_resolution_clock::now();

    MoveList move_list;
    generate_pseudo_legal_moves(board, move_list);

    uint64_t total_nodes = 0;
    for (int i = 0; i < move_list.count; i++) {
        UndoRecord undo;
        if (!make_move(board, move_list.moves[i], undo)) continue;
        
        uint64_t nodes = perft_driver(board, depth - 1);
        total_nodes += nodes;
        unmake_move(board, move_list.moves[i], undo);

        int src = get_source(move_list.moves[i]);
        int tgt = get_target(move_list.moves[i]);
        char src_f = 'a' + (src % 8); char src_r = '1' + (src / 8);
        char tgt_f = 'a' + (tgt % 8); char tgt_r = '1' + (tgt / 8);

        std::cout << src_f << src_r << tgt_f << tgt_r << " : " << nodes << "\n";
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "----------------------------------------\n";
    std::cout << "Total Nodes analyzed  : " << total_nodes << "\n";
    std::cout << "Elapsed Time          : " << std::fixed << std::setprecision(3) << elapsed.count() << " seconds\n";
    std::cout << "Nodes per Second      : " << (uint64_t)(total_nodes / elapsed.count()) << " nodes/sec\n";
    std::cout << "========================================\n\n";
}