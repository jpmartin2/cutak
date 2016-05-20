#pragma once

#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include <iostream>

class alphabeta {
public:
  alphabeta(uint8_t player) : player(player) {}

  template<uint8_t SIZE, typename Evaluator>
  int search(Board<SIZE>& state, Move<SIZE>& bestMove, Evaluator eval, int max_depth, int depth = 0, int alpha = Evaluator::MIN, int beta = Evaluator::MAX) {
    if(depth == 0) {
      leaf_count = 0;
      start = std::chrono::steady_clock::now();
      int c = 0;
//      best_moves.clear();
    }
    if(depth == max_depth) leaf_count++;
    GameStatus status = state.status();

    if(status.over) {
      return status.winner == player ? Evaluator::WIN+(max_depth-depth) : Evaluator::LOSS-(max_depth-depth);
    }

    if(depth == max_depth) {
      return eval(state, player);
    } else if(state.curPlayer == player) {
      typename Board<SIZE>::Map map(state);
      state.forEachMove(map, [this, &eval, &bestMove, &state, max_depth, depth, &alpha, &beta](Move<SIZE> m) {
        Board<SIZE> check = state;
        check.execute(m);
        Move<SIZE> move;
        int c = search(check, move, eval, max_depth, depth+1, alpha, beta);
        if(depth == 0) std::cout << "Considered move " << ptn::to_str(m) << " with score " << c << std::endl;
        //state.undo(m);

        if(state != check) {
          //std::cout << "Error undoing move!" << std::endl;
        }

        if(beta <= alpha) return BREAK;

        if(c > alpha) {
          alpha = c;
          bestMove = m;
        }

        return CONTINUE;
      });

      if(depth == 0) {
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
        std::cout << leaf_count << " leafs evaluated in " << time_span.count() << std::endl;
      }

      return alpha;
    } else {
      typename Board<SIZE>::Map map(state);
      state.forEachMove(map, [this, &eval, &bestMove, &state, max_depth, depth, &alpha, &beta](Move<SIZE> m) {
        //std::cout << "Considering move " << ptn::to_str(m) << std::endl;
        Board<SIZE> check = state;
        check.execute(m);
        Move<SIZE> move;
        int c = search(check, move, eval, max_depth, depth+1, alpha, beta);
        //state.undo(m);

        if(state != check) {
          //std::cout << "Error undoing move!" << std::endl;
        }

        if(beta <= alpha) return BREAK;

        if(c < beta) {
          beta = c;
          bestMove = m;
        }

        return CONTINUE;
      });

      if(depth == 0) {
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
        std::cout << leaf_count << " leafs evaluated in " << time_span.count() << std::endl;
      }

      return beta;
    }
  }

private:
  //std::vector<Move<SIZE>> best_moves;
  std::chrono::steady_clock::time_point start;
  std::chrono::steady_clock::time_point end;
  int leaf_count;
  uint8_t player;
};
