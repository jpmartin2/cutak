#pragma once

#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include <iostream>

class alphabeta {
public:
  alphabeta(uint8_t player) : player(player) {}

  template<uint8_t SIZE, typename Evaluator>
  int search(Board<SIZE>& state, Move<SIZE>& bestMove, Evaluator eval, int depth, int alpha = Evaluator::MIN, int beta = Evaluator::MAX) {
    GameStatus status = state.status();

    if(status.over) {
      return status.winner == player ? Evaluator::WIN : Evaluator::LOSS;
    }

    if(depth == 0) {
      return eval(state);
    } else if(state.curPlayer == player) {
      typename Board<SIZE>::Map map(state);
      state.forEachMove(map, [this, &eval, &bestMove, &state, depth, &alpha, &beta](Move<SIZE> m) {
        Board<SIZE> b = state;
        state.execute(m);
        Move<SIZE> move;
        int c = search(state, move, eval, depth-1, alpha, beta);
        state.undo(m);
        if(state != b) {
          std::cout << "ERROR! state not equal" << std::endl;
        }

        if(c > alpha) {
          alpha = c;
          bestMove = m;
        }

        if(beta <= alpha) return BREAK;
        else return CONTINUE;
      });

      return alpha;
    } else {
      typename Board<SIZE>::Map map(state);
      state.forEachMove(map, [this, &eval, &bestMove, &state, depth, &alpha, &beta](Move<SIZE> m) {
        Board<SIZE> b = state;
        state.execute(m);
        Move<SIZE> move;
        int c = search(state, move, eval, depth-1, alpha, beta);
        state.undo(m);
        if(state != b) {
          std::cout << "ERROR! state not equal" << std::endl;
        }

        if(c < beta) {
          beta = c;
          bestMove = m;
        }

        if(beta <= alpha) return BREAK;
        else return CONTINUE;
      });

      return beta;
    }
  }

private:
  uint8_t player;
};
