#pragma once

#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include <iostream>

template<uint8_t SIZE, typename Evaluator>
class alphabeta {
public:
  using Score = decltype(Evaluator::eval(Board<SIZE>(), WHITE));
  std::vector<Move<SIZE>> best_moves;

  template<int N>
  struct KillerMove {
    Move<SIZE> moves[N];
    Score scores[N];
  };

  alphabeta(uint8_t player) : player(player) {}

  Score search(Board<SIZE>& state, Move<SIZE>& bestMove, int max_depth) {
    leaf_count = 0;
    start = std::chrono::steady_clock::now();

    killer_moves = std::vector<KillerMove<2>>(max_depth+1, {Move<SIZE>(), Move<SIZE>(), Evaluator::MIN, Evaluator::MIN});
    best_moves = std::vector<Move<SIZE>>(max_depth+1, Move<SIZE>());

    Score score = search(player, state, bestMove, max_depth, 0, Evaluator::MIN, Evaluator::MAX);

    end = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
    std::cout << leaf_count << " leafs evaluated in " << time_span.count() << "s" << std::endl;
    std::cout << leaf_count/time_span.count() << " leafs/s" << std::endl;

    return score;
  }

  Score search(uint8_t p, Board<SIZE>& state, Move<SIZE>& bestMove, int max_depth, int depth, Score alpha, Score beta) {
    if(depth == max_depth) leaf_count++;
    GameStatus status = state.status();

    if(status.over) {
      return status.winner == p ? Evaluator::WIN+(max_depth-depth) : Evaluator::LOSS-(max_depth-depth);
    }

    if(depth == max_depth) {
      return Evaluator::eval(state, p);
    } else {
      Score bestScore = Evaluator::MIN;

      for(int i = 0; i < sizeof(killer_moves[depth].moves)/sizeof(killer_moves[depth].moves[0]); i++) {
        if(state.valid(killer_moves[depth].moves[i]) && killer_moves[depth].scores[i] > Evaluator::MIN) {
          Board<SIZE> check = state;
          check.execute(killer_moves[depth].moves[i]);
          Move<SIZE> move;
          Score score = -search(p == WHITE ? BLACK : WHITE, check, move, max_depth, depth+1, -beta, -alpha);
          if(depth == 0) std::cout << "Considered move " << ptn::to_str(killer_moves[depth].moves[i]) << " with score " << score << std::endl;

          if(score > bestScore) {
            bestScore = score;
            bestMove = killer_moves[depth].moves[i];
            best_moves[depth] = bestMove;
          }

          alpha = util::max(alpha, score);

          if(alpha >= beta) {
            return beta;
          }

          // Once we find a 1 ply win, we can stop searching
          // since we can't do better
          if(bestScore == Evaluator::WIN+max_depth) {
            return bestScore;
          }
        }
      }

      typename Board<SIZE>::Map map(state);
      state.forEachMove(map, [this, p, &bestMove, &bestScore, &state, max_depth, depth, &alpha, &beta](Move<SIZE> m) {
        Board<SIZE> check = state;
        check.execute(m);
        Move<SIZE> move;
        Score score = -search(p == WHITE ? BLACK : WHITE, check, move, max_depth, depth+1, -beta, -alpha);
        if(depth == 0) std::cout << "Considered move " << ptn::to_str(m) << " with score " << score << std::endl;

        if(score > bestScore) {
          bestScore = score;
          bestMove = m;
          best_moves[depth] = bestMove;
        }

        alpha = util::max(alpha, score);

        if(alpha >= beta) {
          for(int i = 0; i < sizeof(killer_moves[depth].moves)/sizeof(killer_moves[depth].moves[0]); i++) {
            if(bestScore > killer_moves[depth].scores[i]) {
              killer_moves[depth].scores[i] = bestScore;
              killer_moves[depth].moves[i] = m;
              break;
            }
          }

          bestScore = beta;
          return BREAK;
        }

        // Once we find a 1 ply win, we can stop searching
        // since we can't do better
        if(bestScore == Evaluator::WIN+max_depth) {
          return BREAK;
        }

        return CONTINUE;
      });

      return bestScore;
    }
  }

private:
  std::vector<KillerMove<2>> killer_moves;
  std::chrono::steady_clock::time_point start;
  std::chrono::steady_clock::time_point end;
  int leaf_count;
  uint8_t player;
};
