#pragma once

#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include <vector>
#include <iostream>

template<uint8_t SIZE, typename Evaluator>
class alphabeta {
public:
  using Score = decltype(Evaluator::eval(Board<SIZE>(), WHITE));
private:
  template<int N>
  struct KillerMove {
    static const int size = N;
    Move<SIZE> moves[N];
    Score scores[N];
  };

  std::vector<KillerMove<2>> killer_moves;
  std::chrono::steady_clock::time_point start;
  std::chrono::steady_clock::time_point end;
  int leaf_count;
  uint8_t player;
public:

  alphabeta(uint8_t player) : player(player) {}

  Score search(Board<SIZE>& state, Move<SIZE>& bestMove, int max_depth) {
    leaf_count = 0;
    start = std::chrono::steady_clock::now();

    killer_moves = std::vector<KillerMove<2>>(max_depth+1, {Move<SIZE>(), Move<SIZE>(), Evaluator::MIN, Evaluator::MIN});

    Move<SIZE> pv[max_depth];
    Score score = search(player, state, pv, max_depth, 0, Evaluator::MIN, Evaluator::MAX);

    end = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
    std::cout << "Principal variation:";
    for(auto move : pv) {
      std::cout << " " << ptn::to_str(move);
    }
    std::cout << std::endl;
    std::cout << leaf_count << " leafs evaluated in " << time_span.count() << "s" << std::endl;
    std::cout << leaf_count/time_span.count() << " leafs/s" << std::endl;

    bestMove = pv[0];

    return score;
  }

  Score search(uint8_t p, Board<SIZE>& state, Move<SIZE>* pv, int max_depth, int depth, Score alpha, Score beta) {
    if(depth == max_depth) leaf_count++;
    GameStatus status = state.status();

    if(status.over) {
      return status.winner == p ? Evaluator::WIN+(max_depth-depth) : Evaluator::LOSS-(max_depth-depth);
    }

    if(depth == max_depth) {
      return Evaluator::eval(state, p);
    } else {
      typename Board<SIZE>::Map map(state);

      struct MoveAndScore {
        Move<SIZE> m;
        int s;
      };

      auto score_move = [this,depth](Move<SIZE>& m) {
        for(auto move : killer_moves[depth].moves) {
          if(move == m) return 1;
        }

        return 0;
      };

      int numMoves = 0;
      state.forEachMove(map, [&numMoves](Move<SIZE> m) {
        numMoves++;
        return CONTINUE;
      });
      MoveAndScore moves[numMoves];
      numMoves = 0;
      state.forEachMove(map, [&moves, &numMoves, score_move](Move<SIZE> m) {
        moves[numMoves++] = MoveAndScore{ m, score_move(m)};
        return CONTINUE;
      });

      // Sort the moves, high to low
      // (insertion sort)
      for(int i = 1; i < numMoves; i++) {
        MoveAndScore m = moves[i];
        int j = i - 1;
        for(; j > 0 && moves[j].s < moves[i].s; j--) {
          moves[j+1] = moves[j];
        }
        moves[j+1] = m;
      }

      Move<SIZE> line[max_depth-depth-1];

      Score bestScore = Evaluator::MIN;
      for(int i = 0; i < numMoves; i++) {
        Move<SIZE>& m = moves[i].m;
        Board<SIZE> check = state;
        check.execute(m);
        Score score = -search(p == WHITE ? BLACK : WHITE, check, line, max_depth, depth+1, -beta, -alpha);
        if(depth == 0) std::cout << "Considered move " << ptn::to_str(m) << " with score " << score << std::endl;

        if(score >= beta) {
          for(int i = 0; i < sizeof(killer_moves[depth].moves)/sizeof(killer_moves[depth].moves[0]); i++) {
            // Don't put a move in the table if it's already in the table
            if(killer_moves[depth].moves[i] == m) break;
            if(score > killer_moves[depth].scores[i]) {
              killer_moves[depth].scores[i] = score;
              killer_moves[depth].moves[i] = m;
              break;
            }
          }

          return beta;
        }

        if(score > alpha) {
          alpha = score;
          pv[0] = m;
          std::copy(line, line+max_depth-depth-1, pv+1);
        }

        // Once we find a 1 ply win, we can stop searching
        // since we can't do better
        if(alpha == Evaluator::WIN+max_depth) {
          break;
        }
      }

      return alpha;
      /*
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
      */
    }
  }

};
