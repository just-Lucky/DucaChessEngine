#include "search.h"
#include "eval.h"
#include "attacks.h"
#include "makemove.h"
#include "movegen.h"
#include "tt.h"
#include "zobrist.h"
#include "nnue.h"
#include <iostream>
#include <algorithm>
#include <cstring>

static long long nodes_searched = 0;
static long long quiescence_nodes = 0;
static Move root_best_move = 0;

std::atomic<bool> search_aborted(false);
uint64_t game_history[2048] = {0ULL};
int game_history_ply = 0;
std::chrono::time_point<std::chrono::high_resolution_clock> search_start_time;
long long search_time_limit = -1;
Move killer_moves[2][64];
int history_moves[2][6][64];
uint64_t search_history[128] = {0ULL};
uint8_t tt_global_age = 0;

bool is_repetition(uint64_t hash, int ply) {
    for (int i = ply - 2; i >= 0; i -= 2) {
        if (search_history[i] == hash) return true;
    }
    int start_idx = game_history_ply - 1 - (ply & 1);
    for (int i = start_idx; i >= 0; i -= 2) {
        if (game_history[i] == hash) return true;
    }
    return false;
}

// opposed to generate_pseudo_legal_moves(), this only generates captures. Useful for quiescence search
static void generate_captures(const Board& board, MoveList& list) {
    int color = board.turn;
    int enemy  = color ^ 1;
    int promo_rank  = (color == WHITE) ? 7 : 0;
 
    uint64_t pawns = board.bitboards[(color == WHITE) ? W_PAWN : B_PAWN];
    while (pawns) {
        int src = __builtin_ctzll(pawns);
        uint64_t captures = pawn_attacks[color][src] & board.occupancies[enemy];
        while (captures) {
            int tgt = __builtin_ctzll(captures);
            if (tgt / 8 == promo_rank) {
                list.add_move(encode_move(src, tgt, PC_KNIGHT));
                list.add_move(encode_move(src, tgt, PC_BISHOP));
                list.add_move(encode_move(src, tgt, PC_ROOK));
                list.add_move(encode_move(src, tgt, PC_QUEEN));
            } else {
                list.add_move(encode_move(src, tgt, CAPTURE));
            }
            captures &= captures - 1;
        }
 
        int tgt = (color == WHITE) ? src + 8 : src - 8;
        if (tgt / 8 == promo_rank && !(board.occupancies[BOTH] & (1ULL << tgt))) {
            list.add_move(encode_move(src, tgt, PR_KNIGHT));
            list.add_move(encode_move(src, tgt, PR_BISHOP));
            list.add_move(encode_move(src, tgt, PR_ROOK));
            list.add_move(encode_move(src, tgt, PR_QUEEN));
        }
 
        if (board.enpassant != 64 &&
            (pawn_attacks[color][src] & (1ULL << board.enpassant))) {
            list.add_move(encode_move(src, board.enpassant, EP_CAPTURE));
        }
 
        pawns &= pawns - 1;
    }

    int start_piece = (color == WHITE) ? W_KNIGHT : B_KNIGHT;
    int end_piece   = (color == WHITE) ? W_KING   : B_KING;
 
    for (int piece = start_piece; piece <= end_piece; ++piece) {
        uint64_t bb = board.bitboards[piece];
        while (bb) {
            int src = __builtin_ctzll(bb);
            uint64_t attacks = 0ULL;
 
            if      (piece == W_KNIGHT || piece == B_KNIGHT) attacks = knight_attacks[src];
            else if (piece == W_BISHOP || piece == B_BISHOP) attacks = get_bishop_attacks(src, board.occupancies[BOTH]);
            else if (piece == W_ROOK   || piece == B_ROOK)   attacks = get_rook_attacks(src, board.occupancies[BOTH]);
            else if (piece == W_QUEEN  || piece == B_QUEEN)  attacks = get_queen_attacks(src, board.occupancies[BOTH]);
            else if (piece == W_KING   || piece == B_KING)   attacks = king_attacks[src];
 
            attacks &= board.occupancies[enemy];
 
            while (attacks) {
                int tgt = __builtin_ctzll(attacks);
                list.add_move(encode_move(src, tgt, CAPTURE));
                attacks &= attacks - 1;
            }
 
            bb &= bb - 1;
        }
    }
}


int score_move(const Board& board, Move move, Move tt_move, int ply) {
    if (move == tt_move) return 20000;
    int score = 0;
    int flag = get_flag(move);
    int source = get_source(move);
    int target = get_target(move);
    
    int moving_piece = get_piece_on_square(board, source);
    if (moving_piece == -1) return 0;
    
    int piece_type = moving_piece % 6;

    // Priority 1: Capture (Most Valuable Victim-Least Valuable Attacker)
    if (is_capture(move)) {
        int victim;
        if (flag == EP_CAPTURE) {
            victim = 0;
        } else {
            victim = get_piece_on_square(board, target) % 6;
        }
        score = 10000 + piece_values_array[victim] - piece_values_array[piece_type];
    }
    // Priority 2: Promotions
    else if (flag >= PR_KNIGHT) {
        score += 9000; 
        if (flag == PR_QUEEN || flag == PC_QUEEN) {
            score += 1000;
        }
    }
    // Priority 3: Quiet moves
    else {
        if (ply < 64 && killer_moves[0][ply] == move) {
            return 8000; 
        }
        else if (ply < 64 && killer_moves[1][ply] == move) {
            return 7000;
        }
        else {
            score = history_moves[board.turn][piece_type][target];
            if (score > 6000) score = 6000;
        }
    }
    
    return score;
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
    search_history[ply] = board.hash_key;
    if (ply > 0 && is_repetition(board.hash_key, ply)) {
        return 0;
    }
    quiescence_nodes++;

    int stand_pat = evaluate(board, ply);

    if (stand_pat >= beta) {
        return beta;
    }
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }

    MoveList move_list;
    generate_captures(board, move_list);
    int move_scores[256];
    for (int i = 0; i < move_list.count; ++i) {
        move_scores[i] = score_move(board, move_list.moves[i], 0, ply);
    }

    for (int i = 0; i < move_list.count; ++i) {
        // Incremental Move Picking
        int best_idx = i;
        int best_score = move_scores[i];
        for (int j = i + 1; j < move_list.count; ++j) {
            if (move_scores[j] > best_score) {
                best_score = move_scores[j];
                best_idx = j;
            }
        }
        if (best_idx != i) {
            std::swap(move_scores[i], move_scores[best_idx]);
            std::swap(move_list.moves[i], move_list.moves[best_idx]);
        }

        Move move = move_list.moves[i];

        // Delta Pruning
        int flag = get_flag(move);
        int gain = 0;
        if (flag == EP_CAPTURE) {
            gain = piece_values_array[0];
        } else if (flag < PR_KNIGHT) {
            gain = piece_values_array[board.pieces[get_target(move)] % 6];
        } else if (flag >= PC_KNIGHT) {
            gain = piece_values_array[board.pieces[get_target(move)] % 6] + piece_values_array[4];
        } else {
            gain = piece_values_array[4];
        }
        if (stand_pat + gain + 200 < alpha) continue;

        UndoRecord undo;
        if (!make_move(board, move, undo)) {
            continue;
        }
        nnue_thread_state[ply + 1].accumulator.computedAccumulation = 0;

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
    search_history[ply] = board.hash_key;
    if (ply > 0 && is_repetition(board.hash_key, ply)) {
        return 0;
    }
    int hash_flag = HASH_ALPHA;
    Move tt_move = 0;
    int tt_score = probe_tt(board.hash_key, depth, alpha, beta, tt_move, ply);
    if (tt_score != UNKNOWN_SCORE) {
        if (ply == 0) root_best_move = tt_move; 
        return tt_score;
    }
    if (depth <= 0) {
        return quiescence_search(board, alpha, beta, ply);
    }
    int static_eval = evaluate(board, ply);
    int king_piece = (board.turn == WHITE) ? W_KING : B_KING;
    int king_square = __builtin_ctzll(board.bitboards[king_piece]);
    bool in_check = is_square_attacked(king_square, board.turn ^ 1, board);
    int side = board.turn;
    // Reverse Futility Pruning
    if (depth <= 3 && !in_check && ply > 0 && abs(beta) < MATE_VALUE - 1000) {
        int rfp_margin = 120 * depth; 
        if (static_eval - rfp_margin >= beta) {
            return static_eval;
        }
    }
    // Null Move Pruning
    if (depth >= 3 && ply > 0 && !in_check && static_eval >= beta) {
        uint64_t non_pawns = board.bitboards[side == WHITE ? W_KNIGHT : B_KNIGHT] |
                             board.bitboards[side == WHITE ? W_BISHOP : B_BISHOP] |
                             board.bitboards[side == WHITE ? W_ROOK : B_ROOK] |
                             board.bitboards[side == WHITE ? W_QUEEN : B_QUEEN];
        if(non_pawns != 0){
            UndoRecord null_undo;
            null_undo.enpassant = board.enpassant;
            null_undo.hash_key = board.hash_key;
            auto saved_dirty_piece = board.dirty_piece; 
            int saved_dirty_num = board.dirty_piece.dirtyNum; board.dirty_piece.dirtyNum = 0;
            std::memset(&board.dirty_piece, 0, sizeof(board.dirty_piece));
            if (board.enpassant != 64) {
                board.hash_key ^= zobrist_enpassant[board.enpassant % 8];
                board.enpassant = 64;
            }
            board.turn ^= 1;
            board.hash_key ^= zobrist_side;
            int R = 3 + (depth / 6);
            nnue_thread_state[ply + 1].accumulator.computedAccumulation = 0;
            int null_score = -alpha_beta(board, -beta, -beta + 1, depth - 1 - R, ply + 1);

            board.turn ^= 1;
            board.enpassant = null_undo.enpassant;
            board.hash_key = null_undo.hash_key;
            board.dirty_piece = saved_dirty_piece;
            board.dirty_piece.dirtyNum = saved_dirty_num;
            if (null_score >= beta) {
                return beta;
            }
        }
        
    }
    nodes_searched++;

    MoveList move_list;
    generate_pseudo_legal_moves(board, move_list);
    int move_scores[256];
    for (int i = 0; i < move_list.count; ++i) {
        move_scores[i] = score_move(board, move_list.moves[i], tt_move, ply);
    }
    int best_score = -INF_VAL;
    int legal_moves = 0;
    Move best_move_this_node = tt_move;
    for (int i = 0; i < move_list.count; ++i) {
        // Incremental Move Picking
        for (int j = i + 1; j < move_list.count; ++j) {
            if (move_scores[j] > move_scores[i]) {
                std::swap(move_scores[i], move_scores[j]);
                std::swap(move_list.moves[i], move_list.moves[j]);
            }
        }
        Move move = move_list.moves[i];
        UndoRecord undo;
        bool is_quiet = !is_capture(move) && get_flag(move) < PR_KNIGHT;
        // Futility Pruning
        if (depth <= 4 && !in_check && is_quiet && abs(alpha) < MATE_VALUE - 1000) {
            int fp_margin = 120 * depth;
            if (static_eval + fp_margin <= alpha) {
                continue;
            }
        }

        // Late Move Pruning
        if (depth <= 4 && !in_check && is_quiet && legal_moves > (4 + 2 * depth * depth)) {
            continue;
        }

        int moving_piece = get_piece_on_square(board, get_source(move)) % 6;
        int moving_side = board.turn;

        if (!make_move(board, move, undo)) {
            continue;
        }
        
        legal_moves++;
        int score;

        if (legal_moves == 1) {
            nnue_thread_state[ply + 1].accumulator.computedAccumulation = 0;
            score = -alpha_beta(board, -beta, -alpha, depth - 1, ply + 1);
        } else {
            int reduction = 0;
            // Late Move Reduction
            if (depth >= 3 && legal_moves > 3 && is_quiet && !in_check) {
                reduction = 1 + (depth / 5) + (legal_moves / 10);
                if (reduction >= depth) reduction = depth - 1;
            }

            nnue_thread_state[ply + 1].accumulator.computedAccumulation = 0;
            score = -alpha_beta(board, -alpha - 1, -alpha, depth - 1 - reduction, ply + 1);
            
            if (score > alpha && reduction > 0) {
                nnue_thread_state[ply + 1].accumulator.computedAccumulation = 0;
                score = -alpha_beta(board, -alpha - 1, -alpha, depth - 1, ply + 1);
            }
            // Principal Variation Search
            if (score > alpha && score < beta) {
                nnue_thread_state[ply + 1].accumulator.computedAccumulation = 0;
                score = -alpha_beta(board, -beta, -alpha, depth - 1, ply + 1);
            }
        }
        
        unmake_move(board, move, undo);
        if(score > best_score) best_score = score;
        if (score >= beta) {
            if (!is_capture(move)) {
                history_moves[moving_side][moving_piece][get_target(move)] += depth * depth;
                if (history_moves[moving_side][moving_piece][get_target(move)] > 4000) {
                    for (int c = 0; c < 2; c++) {
                        for (int p = 0; p < 6; p++) {
                            for (int s = 0; s < 64; s++) {
                                history_moves[c][p][s] /= 2;
                            }
                        }
                    }
                }
                if (ply < 64 && killer_moves[0][ply] != move) {
                    killer_moves[1][ply] = killer_moves[0][ply];
                    killer_moves[0][ply] = move;
                }
            }
            store_tt(board.hash_key, depth, best_score, HASH_BETA, move, ply);
            return best_score; 
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

        if (in_check) {
            return -MATE_VALUE + ply;
        } else {
            return 0;
        }
    }
    store_tt(board.hash_key, depth,  best_score, hash_flag, best_move_this_node, ply);
    return best_score;
}

void search_position(Board& board, int max_depth) {
    tt_global_age++;
    for(int i=0; i<64; i++) { killer_moves[0][i] = 0; killer_moves[1][i] = 0; }
    for(int c=0; c<2; c++) for(int p=0; p<6; p++) for(int s=0; s<64; s++) history_moves[c][p][s] = 0;
    for(int i = 0; i < 128; i++) search_history[i] = 0ULL;
    root_best_move = 0;
    nnue_thread_state[0].accumulator.computedAccumulation = 0;
    int last_score = 0;
    long long total_nodes = 0;
    for (int current_depth = 1; current_depth <= max_depth; ++current_depth) {
        nodes_searched = 0;
        quiescence_nodes = 0;

        int score;
        if (current_depth >= 5) {
            // Asymmetric Aspiration Windows
            int awindow = 64;
            int bwindow = 64;
            int alpha = last_score - awindow;
            int beta = last_score + bwindow;
            int fail_count = 0;
            int search_depth = current_depth;
            while (!search_aborted) {
                score = alpha_beta(board, alpha, beta, search_depth, 0);
                if (search_aborted) break;
                int total_depth_reduction = 0;
                if (score <= alpha) {
                    fail_count++;
                    int multiplier = 380 + (fail_count * 100);
                    awindow += (multiplier * awindow) / 256;
                    alpha = last_score - awindow;
                    if (fail_count == 2 && search_depth > 18 && total_depth_reduction < 3) {
                        search_depth--;
                        total_depth_reduction++;
                    } else if (fail_count >= 3 && total_depth_reduction < 3) {
                        int can_reduce = std::min(
                            3 - total_depth_reduction,
                            search_depth > 28 ? 3 : 
                            search_depth > 24 ? 2 :
                            search_depth > 18 ? 1 : 0
                        );
                        search_depth -= can_reduce;
                        total_depth_reduction += can_reduce;
                    }
                }else if(score >= beta){
                    fail_count++;
                    int multiplier = 380 + (fail_count * 100);
                    bwindow += (multiplier * bwindow) / 256;
                    beta = last_score + bwindow;
                    if (fail_count == 2 && search_depth > 18 && total_depth_reduction < 3) {
                        search_depth--;
                        total_depth_reduction++;
                    } else if (fail_count >= 3 && total_depth_reduction < 3) {
                        int can_reduce = std::min(3 - total_depth_reduction,
                                                search_depth > 28 ? 3 :
                                                search_depth > 24 ? 2 :
                                                search_depth > 18 ? 1 : 0);
                        search_depth -= can_reduce;
                        total_depth_reduction += can_reduce;
                    }
                }else {
                    if (search_depth < current_depth) {
                        search_depth = current_depth;
                    } else {
                        break;
                    }
                }
            }
        } else {
            score = alpha_beta(board, -INF_VAL, INF_VAL, current_depth, 0);
        }
        last_score = score;
        total_nodes += nodes_searched + quiescence_nodes;
        if (search_aborted) break;

        std::cout << "info depth " << current_depth;
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - search_start_time).count();
        long long nps = (elapsed > 0) ? (total_nodes * 1000 / elapsed) : 0;
        if (score > MATE_VALUE - 100) {
            std::cout << " score mate " << (MATE_VALUE - score + 1) / 2;
        } else if (score < -MATE_VALUE + 100) {
            std::cout << " score mate -" << (MATE_VALUE + score + 1) / 2;
        } else {
            std::cout << " score cp " << score;
        }

        std::cout << " time " << elapsed << " nodes " << total_nodes << " nps " << nps;
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