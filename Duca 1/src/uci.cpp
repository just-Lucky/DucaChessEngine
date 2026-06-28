#include "uci.h"
#include "board.h"
#include "movegen.h"
#include "makemove.h"
#include "search.h"
#include "tt.h"
#include <iostream>
#include <string>
#include <sstream>
#include <thread>

std::thread search_thread;

// Helper per convertire una mossa interna a 16 bit in formato UCI
std::string move_to_string(Move move) {
    if (!move) return "0000";
    int src = get_source(move);
    int tgt = get_target(move);
    
    std::string s = "";
    s += (char)('a' + (src % 8));
    s += (char)('1' + (src / 8));
    s += (char)('a' + (tgt % 8));
    s += (char)('1' + (tgt / 8));
    
    int flag = get_flag(move);
    if (flag >= PR_KNIGHT) {
        if (flag == PR_KNIGHT || flag == PC_KNIGHT) s += "n";
        else if (flag == PR_BISHOP || flag == PC_BISHOP) s += "b";
        else if (flag == PR_ROOK || flag == PC_ROOK) s += "r";
        else if (flag == PR_QUEEN || flag == PC_QUEEN) s += "q";
    }
    return s;
}

// Helper per parsare una stringa UCI e trovare la corrispondente mossa pseudo-legale valida
Move parse_move(Board& board, const std::string& move_str) {
    MoveList move_list;
    generate_pseudo_legal_moves(board, move_list);
    
    for (int i = 0; i < move_list.count; ++i) {
        Move move = move_list.moves[i];
        if (move_to_string(move) == move_str) {
            UndoRecord undo;
            if (make_move(board, move, undo)) {
                unmake_move(board, move, undo);
                return move;
            }
        }
    }
    return 0;
}

// Parsa il comando "position [startpos | fen ...] moves ..."
void parse_position(Board& board, std::stringstream& ss) {
    std::string token;
    ss >> token;
    game_history_ply = 0;
    if (token == "startpos") {
        initialize_starter_board(board);
        game_history[game_history_ply++] = board.hash_key;
        ss >> token;
    } else if (token == "fen") {
        std::string fen_str = "";
        for (int i = 0; i < 6; ++i) {
            std::string part;
            if (ss >> part) {
                fen_str += part + " ";
            }
        }
        parse_fen(board, fen_str);
        game_history[game_history_ply++] = board.hash_key;
        ss >> token;
    }
    
    if (token == "moves") {
        std::string move_str;
        while (ss >> move_str) {
            Move move = parse_move(board, move_str);
            if (move != 0) {
                UndoRecord undo;
                make_move(board, move, undo);
                game_history[game_history_ply++] = board.hash_key;
            }
        }
    }
    update_occupancies(board);
}

void parse_go(Board& board, std::stringstream& ss) {
    std::string token;
    int depth = 8;
    long long wtime = -1, btime = -1, winc = 0, binc = 0, movestogo = 30, movetime = -1;
    bool infinite = false;
    
    while (ss >> token) {
        if (token == "depth"){
            ss >> depth;
        }else if(token == "wtime"){
            ss >> wtime;
        }else if(token == "btime"){
            ss >> btime;
        }else if(token == "winc"){
            ss >> winc;
        }else if(token == "binc"){
            ss >> binc;
        }else if(token == "movestogo"){
            ss >> movestogo;
        }else if(token == "movetime"){
            ss >> movetime;
        }else if(token == "infinite"){
            infinite = true;
        }
    }

    if (movetime != -1) {
        search_time_limit = movetime - 20;
    } else if (infinite) {
        search_time_limit = -1;
    } else {
        long long my_time = (board.turn == WHITE) ? wtime : btime;
        long long my_inc  = (board.turn == WHITE) ? winc : binc;
        
        if (my_time != -1) {
            search_time_limit = (my_time / movestogo) + (my_inc * 3 / 4) - 20;
            if (search_time_limit > my_time) search_time_limit = my_time - 50;
            if (search_time_limit < 10) search_time_limit = 10;
        }
    }
    if (search_thread.joinable()) {
        search_aborted = true;
        search_thread.join();
    }

    search_aborted = false;
    search_start_time = std::chrono::high_resolution_clock::now();

    search_thread = std::thread(search_position, std::ref(board), depth);
}

void uci_loop() {
    setbuf(stdout, NULL);
    setbuf(stdin, NULL);
    
    Board board;
    initialize_starter_board(board);
    
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        std::stringstream ss(line);
        std::string command;
        ss >> command;
        
        if (command == "uci") {
            std::cout << "id name Duca1\n";
            std::cout << "id author Bruscagin\n";
            std::cout << "uciok\n";
        } 
        else if (command == "isready") {
            std::cout << "readyok\n";
        } 
        else if (command == "ucinewgame") {
            clear_tt();
            initialize_starter_board(board);
            game_history_ply = 0;
        } 
        else if (command == "position") {
            parse_position(board, ss);
        } 
        else if (command == "go") {
            parse_go(board, ss);
        } 
        else if (command == "stop") {
            if (search_thread.joinable()) {
                search_aborted = true;
                search_thread.join();
            }
        }
        else if (command == "quit") {
            if (search_thread.joinable()) {
                search_aborted = true;
                search_thread.join();
            }
            break;
        }
    }
}