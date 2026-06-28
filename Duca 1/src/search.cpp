#include "search.h"
#include "eval.h"
#include "attacks.h"
#include "makemove.h"
#include "movegen.h"
#include "tt.h"
#include <iostream>
#include <algorithm>

static long long nodes_searched = 0;
static long long quiescence_nodes = 0;
static Move root_best_move = 0;

std::atomic<bool> search_aborted(false);
uint64_t game_history[2048] = {0ULL};
int game_history_ply = 0;
std::chrono::time_point<std::chrono::high_resolution_clock> search_start_time;
long long search_time_limit = -1;

bool is_repetition(uint64_t hash) {
    for (int i = 0; i < game_history_ply; ++i) {
        if (game_history[i] == hash) return true;
    }
    return false;
}

int score_move(const Board& board, Move move, Move tt_move) {
    if (move == tt_move) return 20000;
    int score = 0;
    int flag = get_flag(move);
    int source = get_source(move);
    int target = get_target(move);
    
    int moving_piece = get_piece_on_square(board, source);
    if (moving_piece == -1) return 0;
    
    int piece_type = moving_piece % 6;

    // Priorità 1: Catture (MVV-LVA)
    if (is_capture(move)) {
        int victim;
        if (flag == EP_CAPTURE) {
            victim = 0;
        } else {
            victim = get_piece_on_square(board, target) % 6;
        }
        score = 10000 + piece_values_array[victim] - piece_values_array[piece_type];
    }
    // Priorità 2: Promozioni
    else if (flag >= PR_KNIGHT) {
        score += 9000; 
    }
    // Priorità 3: Mosse Silenziose (Differenziale PST)
    else {
        int target_sq = (board.turn == WHITE) ? target : target ^ 56;
        int source_sq = (board.turn == WHITE) ? source : source ^ 56;
        
        score = pst_tables[piece_type][target_sq] - pst_tables[piece_type][source_sq];
    }
    
    return score;
}

void sort_moves(const Board& board, MoveList& move_list, Move tt_move) {
    int move_scores[256];
    for (int i = 0; i < move_list.count; ++i) {
        move_scores[i] = score_move(board, move_list.moves[i], tt_move);
    }
    
    // Selection Sort
    for (int i = 0; i < move_list.count - 1; ++i) {
        int max_idx = i;
        for (int j = i + 1; j < move_list.count; ++j) {
            if (move_scores[j] > move_scores[max_idx]) {
                max_idx = j;
            }
        }
        std::swap(move_scores[i], move_scores[max_idx]);
        std::swap(move_list.moves[i], move_list.moves[max_idx]);
    }
}

int quiescence_search(Board& board, int alpha, int beta, int ply) {
    if ((nodes_searched + quiescence_nodes) % 2048 == 0) {
        if (search_time_limit != -1) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
            if (elapsed >= search_time_limit) {
                search_aborted = true;
            }
        }
    }
    if (search_aborted) return 0;

    if (ply > 0 && is_repetition(board.hash_key)) {
        return 0;
    }
    quiescence_nodes++;

    int stand_pat = evaluate(board);

    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    MoveList move_list;
    generate_pseudo_legal_moves(board, move_list);
    sort_moves(board, move_list, 0);

    for (int i = 0; i < move_list.count; ++i) {
        Move move = move_list.moves[i];
        
        if (!is_capture(move)) continue;

        UndoRecord undo;
        if (!make_move(board, move, undo)) {
            continue;
        }

        int score = -quiescence_search(board, -beta, -alpha, ply + 1);
        
        unmake_move(board, move, undo);

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

int alpha_beta(Board& board, int alpha, int beta, int depth, int ply) {
    if ((nodes_searched + quiescence_nodes) % 2048 == 0) {
        if (search_time_limit != -1) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
            if (elapsed >= search_time_limit) {
                search_aborted = true;
            }
        }
    }
    if (search_aborted) return 0;

    if (ply > 0 && is_repetition(board.hash_key)) {
        return 0;
    }
    int hash_flag = HASH_ALPHA;
    Move tt_move = 0;
    int tt_score = probe_tt(board.hash_key, depth, alpha, beta, tt_move, ply);
    if (tt_score != UNKNOWN_SCORE) {
        if (ply == 0) root_best_move = tt_move; 
        return tt_score;
    }
    if (depth == 0) {
        return quiescence_search(board, alpha, beta, ply);
    }

    nodes_searched++;

    MoveList move_list;
    generate_pseudo_legal_moves(board, move_list);
    sort_moves(board, move_list, tt_move);
    
    int legal_moves = 0;
    Move best_move_this_node = 0;
    for (int i = 0; i < move_list.count; ++i) {
        Move move = move_list.moves[i];
        UndoRecord undo;

        if (!make_move(board, move, undo)) {
            continue;
        }
        
        legal_moves++;

        int score = -alpha_beta(board, -beta, -alpha, depth - 1, ply + 1);
        
        unmake_move(board, move, undo);

        if (score >= beta) {
            store_tt(board.hash_key, depth, beta, HASH_BETA, move, ply);
            return beta; 
        }

        if (score > alpha) {
            hash_flag = HASH_EXACT;
            alpha = score;
            best_move_this_node = move;
            
            if (ply == 0) {
                root_best_move = move;
            }
        }
    }

    if (legal_moves == 0) {
        int king_piece = (board.turn == WHITE) ? W_KING : B_KING;
        int king_square = __builtin_ctzll(board.bitboards[king_piece]);

        if (is_square_attacked(king_square, board.turn ^ 1, board)) {
            return -MATE_VALUE + ply;
        } else {
            return 0;
        }
    }
    store_tt(board.hash_key, depth, alpha, hash_flag, best_move_this_node, ply);
    return alpha;
}

void search_position(Board& board, int max_depth) {
    root_best_move = 0;

    for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
        nodes_searched = 0;
        quiescence_nodes = 0;

        int score = alpha_beta(board, -INFINITY, INFINITY, current_depth, 0);

        std::cout << "info depth " << current_depth;
        
        if (score > MATE_VALUE - 100) {
            std::cout << " score mate " << (MATE_VALUE - score + 1) / 2;
        } else if (score < -MATE_VALUE + 100) {
            std::cout << " score mate -" << (MATE_VALUE + score + 1) / 2;
        } else {
            std::cout << " score cp " << score;
        }

        std::cout << " nodes " << (nodes_searched + quiescence_nodes);
        if (root_best_move != 0) {
            int src = get_source(root_best_move);
            int tgt = get_target(root_best_move);
            std::cout << " pv ";
            std::cout << (char)('a' + (src % 8)) << (char)('1' + (src / 8))
                      << (char)('a' + (tgt % 8)) << (char)('1' + (tgt / 8));
            int flag = get_flag(root_best_move);
            if (flag >= PR_KNIGHT) {
                if (flag == PR_KNIGHT || flag == PC_KNIGHT) std::cout << "n";
                else if (flag == PR_BISHOP || flag == PC_BISHOP) std::cout << "b";
                else if (flag == PR_ROOK || flag == PC_ROOK) std::cout << "r";
                else std::cout << "q";
            }
        }
        std::cout << "\n";
    }
    int src = get_source(root_best_move);
    int tgt = get_target(root_best_move);
    std::cout << "bestmove ";
    std::cout << (char)('a' + (src % 8)) << (char)('1' + (src / 8))
              << (char)('a' + (tgt % 8)) << (char)('1' + (tgt / 8));
              
    int flag = get_flag(root_best_move);
    if (flag >= PR_KNIGHT) {
        if (flag == PR_KNIGHT || flag == PC_KNIGHT) std::cout << "n";
        else if (flag == PR_BISHOP || flag == PC_BISHOP) std::cout << "b";
        else if (flag == PR_ROOK || flag == PC_ROOK) std::cout << "r";
        else std::cout << "q";
    }
    std::cout << "\n";
}