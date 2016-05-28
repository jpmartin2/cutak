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

  template<size_t NUM_ENTRIES>
  struct TranspositionTable {
  private:
    struct InternalEntry {
      //Board<SIZE> b;
      uint64_t hash;
      uint64_t data;
    };

    std::array<InternalEntry, NUM_ENTRIES> table;
  public:
    struct Entry {
    private:
      friend class TranspositionTable;
      uint64_t data;
      Entry(uint64_t data) : data(data) {}
    public:
      enum Type : uint8_t {
        INVALID = 0, ALPHA, BETA, EXACT,
      };

      Entry(Type type, int depth, int score) :
        data((((uint64_t)type)<<62) | (((uint64_t)depth)<<32) | ((uint64_t)score))
      {}

      Entry() : data(((uint64_t)INVALID)<<62) {}

      Type type() {
        return (Type)(data>>62);
      }

      int depth() {
        return (data&0x3FFFFFFF00000000)>>32;
      }

      int score() {
        return data&0x00000000FFFFFFFF;
      }
    };

    inline util::option<Entry> get(Board<SIZE>& b) {
      uint64_t hash = b.hash();
      auto e = table[hash % NUM_ENTRIES];
      if((e.hash ^ e.data) != hash) return util::option<Entry>::None;
      else {
        //if(e.b != b) {
          //std::cout << "Hash collision: " << hash << std::endl;
          //std::cout << tps::to_str(e.b) << std::endl;
          //std::cout << tps::to_str(b) << std::endl;
        //}
        return util::option<Entry>(Entry(e.data));
      }
    }

    inline void put(Board<SIZE>& b, Entry e) {
      auto ex = get(b);
      uint64_t hash = b.hash();
      if(ex && ex->depth() >= e.depth()) return;
      //table[hash % NUM_ENTRIES] = InternalEntry { b, hash^e.data, e.data };
      table[hash % NUM_ENTRIES] = InternalEntry { hash^e.data, e.data };
    }
  };

  int hits;

  using TT = TranspositionTable<(1<<25)>;
  std::unique_ptr<TT> ttable;

  std::vector<KillerMove<2>> killer_moves;
  std::chrono::steady_clock::time_point start;
  std::chrono::steady_clock::time_point end;
  int leaf_count;
  uint8_t player;
public:
  alphabeta(uint8_t player) : player(player) {}

  Score search(Board<SIZE>& state, Move<SIZE>& bestMove, int max_depth) {
    // NO TT
    leaf_count = 0;

    killer_moves = std::vector<KillerMove<2>>(max_depth+1, {Move<SIZE>(), Move<SIZE>(), Evaluator::MIN, Evaluator::MIN});
    start = std::chrono::steady_clock::now();

    Move<SIZE> pv[max_depth];
    Score score = search(player, state, pv, max_depth, 0, Evaluator::MIN, Evaluator::MAX);

    end = std::chrono::steady_clock::now();
    std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
    std::cout << "Principal variation (no tt):";
    for(auto move : pv) {
      std::cout << " " << ptn::to_str(move);
    }
    std::cout << std::endl;
    std::cout << leaf_count << " leafs evaluated in " << time_span.count() << "s" << std::endl;
    std::cout << leaf_count/time_span.count() << " leafs/s" << std::endl;

    // W/ TT
    hits = 0;
    leaf_count = 0;
    ttable = std::unique_ptr<TT>(new TT());
    killer_moves = std::vector<KillerMove<2>>(max_depth+1, {Move<SIZE>(), Move<SIZE>(), Evaluator::MIN, Evaluator::MIN});

    start = std::chrono::steady_clock::now();
    score = search(player, state, pv, max_depth, 0, Evaluator::MIN, Evaluator::MAX);
    end = std::chrono::steady_clock::now();
    time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
    std::cout << "Principal variation (w/ tt):";
    for(auto move : pv) {
      std::cout << " " << ptn::to_str(move);
    }
    ttable.reset();
    std::cout << std::endl;
    std::cout << leaf_count << " leafs evaluated in " << time_span.count() << "s" << std::endl;
    std::cout << leaf_count/time_span.count() << " leafs/s" << std::endl;
    std::cout << "Hits: " << hits << std::endl;

    killer_moves = std::vector<KillerMove<2>>(max_depth+1, {Move<SIZE>(), Move<SIZE>(), Evaluator::MIN, Evaluator::MIN});

    bestMove = pv[0];

    return score;
  }

  Score search(uint8_t p, Board<SIZE>& state, Move<SIZE>* pv, int max_depth, int depth, Score alpha, Score beta) {
    using Entry = typename TT::Entry;
    Score init_alpha = alpha;
    if(ttable) {
      auto e = ttable->get(state);
      if(e && e->depth() <= depth) {
        hits++;
        switch(e->type()) {
        case Entry::EXACT:
          return e->score();
        case Entry::BETA:
          alpha = util::max(alpha, e->score());
          //if(e->score() >= beta) return e->score();
          break;
        case Entry::ALPHA:
          //if(e->score() <= alpha) return e->score();
          beta = util::min(beta, e->score());
          break;
        default:
          break;
        }
        hits--;
        if(alpha >= beta) {
          return e->score();
        }
      }
    }

    if(depth == max_depth) leaf_count++;
    GameStatus status = state.status();

    if(status.over) {
      int s = status.winner == p ? Evaluator::WIN+(max_depth-depth) : Evaluator::LOSS-(max_depth-depth);
      if(ttable) ttable->put(state, Entry(Entry::EXACT, depth, s));
      return s;
    }

    if(depth == max_depth) {
      int s = Evaluator::eval(state, p);
      if(ttable) ttable->put(state, Entry(Entry::EXACT, depth, s));
      return s;
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

      Score bestScore = Evaluator::MIN;
      Move<SIZE> line[max_depth-depth-1];
      for(int i = 0; i < numMoves; i++) {
        Move<SIZE>& m = moves[i].m;
        Board<SIZE> check = state;
        check.execute(m);
        Score score = -search(p == WHITE ? BLACK : WHITE, check, line, max_depth, depth+1, -beta, -alpha);
        if(depth == 0) std::cout << "Considered move " << ptn::to_str(m) << " with score " << score << std::endl;

        if(score > bestScore) {
          bestScore = score;
          pv[0] = m;
          std::copy(line, line+max_depth-depth-1, pv+1);
        }
        alpha = util::max(alpha, score);

        if(bestScore >= beta) {
          for(int i = 0; i < sizeof(killer_moves[depth].moves)/sizeof(killer_moves[depth].moves[0]); i++) {
            // Don't put a move in the table if it's already in the table
            if(killer_moves[depth].moves[i] == m) break;
            if(score > killer_moves[depth].scores[i]) {
              killer_moves[depth].scores[i] = score;
              killer_moves[depth].moves[i] = m;
              break;
            }
          }

          if(ttable) ttable->put(state, Entry(Entry::BETA, depth, bestScore));
          return bestScore;
        }

        // Once we find a 1 ply win, we can stop searching
        // since we can't do better
        if(bestScore == Evaluator::WIN+max_depth) {
          break;
        }
      }

      if(ttable) {
        if(bestScore > init_alpha) {
          ttable->put(state, Entry(Entry::EXACT, depth, bestScore));
        } else {
          ttable->put(state, Entry(Entry::ALPHA, depth, bestScore));
        }
      }
      return bestScore;
    }
  }
};
