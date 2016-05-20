#pragma once

#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include <iostream>

template<uint8_t SIZE>
class alphabeta {
public:

  struct KillerMove {
    Move<SIZE> move1;
    Move<SIZE> move2;
    int score1;
    int score2;
  };

  alphabeta(uint8_t player) : player(player) {}

  template<typename Evaluator>
  int search(Board<SIZE>& state, Move<SIZE>& bestMove, Evaluator eval, int max_depth) {
    leaf_count = 0;
    start = std::chrono::steady_clock::now();

    killer_moves = std::vector<KillerMove>(max_depth+1, KillerMove{Move<SIZE>(), Move<SIZE>(), Evaluator::MIN, Evaluator::MIN});

    int score = search(player, state, bestMove, eval, max_depth, 0, Evaluator::MIN, Evaluator::MAX);

    end = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
    std::cout << leaf_count << " leafs evaluated in " << time_span.count() << "s" << std::endl;
    std::cout << leaf_count/time_span.count() << " leafs/s" << std::endl;

    return score;
  }

  template<typename Evaluator>
  int search(uint8_t p, Board<SIZE>& state, Move<SIZE>& bestMove, Evaluator eval, int max_depth, int depth, int alpha, int beta) {
    if(depth == max_depth) leaf_count++;
    GameStatus status = state.status();

    if(status.over) {
      return status.winner == p ? Evaluator::WIN+(max_depth-depth) : Evaluator::LOSS-(max_depth-depth);
    }

    if(depth == max_depth) {
      return eval(state, p);
    } else {
      int bestScore = Evaluator::MIN;

      if(state.valid(killer_moves[depth].move1) && killer_moves[depth].score1 > Evaluator::MIN) {
        Board<SIZE> check = state;
        check.execute(killer_moves[depth].move1);
        Move<SIZE> move;
        int score = -search(p == WHITE ? BLACK : WHITE, check, move, eval, max_depth, depth+1, -beta, -alpha);
        if(depth == 0) std::cout << "Considered move " << ptn::to_str(killer_moves[depth].move1) << " with score " << score << std::endl;

        if(score > bestScore) {
          bestScore = score;
          bestMove = killer_moves[depth].move1;
        }

        alpha = util::max(alpha, score);

        if(alpha >= beta) {
          return beta;
        }
      }

      if(state.valid(killer_moves[depth].move2) && killer_moves[depth].score2 > Evaluator::MIN) {
        Board<SIZE> check = state;
        check.execute(killer_moves[depth].move2);
        Move<SIZE> move;
        int score = -search(p == WHITE ? BLACK : WHITE, check, move, eval, max_depth, depth+1, -beta, -alpha);
        if(depth == 0) std::cout << "Considered move " << ptn::to_str(killer_moves[depth].move2) << " with score " << score << std::endl;

        if(score > bestScore) {
          bestScore = score;
          bestMove = killer_moves[depth].move2;
        }

        alpha = util::max(alpha, score);

        if(alpha >= beta) {
          return beta;
        }
      }

      typename Board<SIZE>::Map map(state);
      state.forEachMove(map, [this, p, &eval, &bestMove, &bestScore, &state, max_depth, depth, &alpha, &beta](Move<SIZE> m) {
        Board<SIZE> check = state;
        check.execute(m);
        Move<SIZE> move;
        int score = -search(p == WHITE ? BLACK : WHITE, check, move, eval, max_depth, depth+1, -beta, -alpha);
        if(depth == 0) std::cout << "Considered move " << ptn::to_str(m) << " with score " << score << std::endl;

        if(score > bestScore) {
          bestScore = score;
          bestMove = m;
        }

        alpha = util::max(alpha, score);

        if(alpha >= beta) {
          if(bestScore > killer_moves[depth].score1) {
            killer_moves[depth].score1 = bestScore;
            killer_moves[depth].move1 = m;
          } else if(bestScore > killer_moves[depth].score2) {
            killer_moves[depth].score2 = bestScore;
            killer_moves[depth].move2 = m;
          }
          bestScore = beta;
          return BREAK;
        }

        return CONTINUE;
      });

      return bestScore;
    }
  }

private:
  std::vector<KillerMove> killer_moves;
  std::chrono::steady_clock::time_point start;
  std::chrono::steady_clock::time_point end;
  int leaf_count;
  uint8_t player;
};
