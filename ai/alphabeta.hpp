#pragma once

#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include <vector>
#include <iostream>
#include <atomic>

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
      std::atomic<uint64_t> hash;
      std::atomic<uint64_t> data;

      InternalEntry() : hash(0), data(0) {}
    };

    std::array<InternalEntry, NUM_ENTRIES> table;
  public:
    struct Entry {
    private:
      friend struct TranspositionTable;
      uint64_t data;
      Entry(uint64_t data) : data(data) {}
    public:
      enum Type : uint8_t {
        INVALID = 0, ALPHA = 1, BETA = 2, EXACT = 3,
      };

      Entry(Type type, int depth, int16_t score) :
        Entry(type, depth, score, Move<SIZE>(0, Piece::INVALID))
      {}

      Entry(Type type, int depth, int16_t score, const Move<SIZE>& bestMove) :
        data((((uint64_t)type&0x3)<<62) | (((uint64_t)depth&0x1FFF)<<49) | ((uint64_t)score&0xFFFF)<<33)
      {
        data |= (static_cast<uint64_t>(bestMove.type())<<32) | (static_cast<uint64_t>(bestMove.idx())<<26);
        switch(bestMove.type()) {
        case Move<SIZE>::Type::PLACE:
          data |= static_cast<uint8_t>(bestMove.pieceType());
          break;
        case Move<SIZE>::Type::MOVE: {
          switch(bestMove.dir()) {
          case Move<SIZE>::Dir::NORTH:
            data |= static_cast<uint64_t>(0)<<24;
            break;
          case Move<SIZE>::Dir::SOUTH:
            data |= static_cast<uint64_t>(1)<<24;
            break;
          case Move<SIZE>::Dir::EAST:
            data |= static_cast<uint64_t>(2)<<24;
            break;
          case Move<SIZE>::Dir::WEST:
            data |= static_cast<uint64_t>(3)<<24;
            break;
          }
          data |= bestMove.range();
          for(int i = 1; i < SIZE; i++) {
            data |= bestMove.slides(i)<<(i*3);
          }
          break;
          }
        }
      }

      Entry() : data(((uint64_t)INVALID)<<62) {}

      Type type() {
        return (Type)(data>>62);
      }

      int depth() {
        return (data&0x3FFE000000000000)>>49;
      }

      Score score() {
        return (data&0x0001FFFE00000000)>>33;
      }

      Move<SIZE> move() {
        typename Move<SIZE>::Type t = (typename Move<SIZE>::Type)((data&0x0000000100000000)>>32);
        uint8_t idx = (data&0x00000000FC000000) >> 26;
        switch(t) {
        case Move<SIZE>::Type::PLACE: {
          Piece pieceType = (Piece)(data&0x0000000000000003);
          return Move<SIZE>(idx, pieceType);
          }
        case Move<SIZE>::Type::MOVE: {
          uint8_t slides[SIZE];
          typename Move<SIZE>::Dir dir;
          switch((data&0x0000000003000000)>>24) {
          case 0:
            dir = Move<SIZE>::Dir::NORTH;
            break;
          case 1:
            dir = Move<SIZE>::Dir::SOUTH;
            break;
          case 2:
            dir = Move<SIZE>::Dir::EAST;
            break;
          case 3:
            dir = Move<SIZE>::Dir::WEST;
            break;
          }
          for(int i = 0; i < SIZE; i++) {
            slides[i] = (data>>(i*3))&0x7;
          }
          return Move<SIZE>(idx, dir, slides[0], &slides[1]);
          }
        default:
          std::cout << "Error: invalid move from hash table" << std::endl;
          // Shouldn't happen
          return Move<SIZE>();
        }
      }
    };

    inline util::option<Entry> get(Board<SIZE>& b) {
      uint64_t hash = b.hash();
      auto& e = table[hash % NUM_ENTRIES];
      auto h = e.hash.load(std::memory_order_relaxed);
      auto d = e.data.load(std::memory_order_relaxed);
      if(Entry(d).type() == Entry::INVALID || (h ^ d) != hash) {
        return util::option<Entry>::None;
      } else {
        return util::option<Entry>(Entry(d));
      }
    }

    inline void put(Board<SIZE>& b, Entry e) {
      uint64_t hash = b.hash();
      // Always replace for now because of MTD-f
      // (we need root node to update the best move!)
      //auto ex = get(b);
      //if(ex && ex->depth() >= e.depth()) return;
      table[hash % NUM_ENTRIES].hash.store(hash^e.data, std::memory_order_relaxed);
      table[hash % NUM_ENTRIES].data.store(e.data, std::memory_order_relaxed);
    }
  };

  int hits;

  //using TT = TranspositionTable<(1<<25)>;
  using TT = TranspositionTable<(1<<18)>;
  std::unique_ptr<TT> ttable;

  std::vector<KillerMove<2>> killer_moves;
  std::chrono::steady_clock::time_point start;
  std::chrono::steady_clock::time_point end;
  int leaf_count;

  const static int NULL_MOVE_REDUCTION = 3;
public:
  Score search(Board<SIZE>& state, Move<SIZE>& bestMove, int max_depth) {
    std::chrono::duration<double> time_span;

    hits = 0;
    leaf_count = 0;
    if(!ttable) {
      std::cout << "Recreating ttable" << std::endl;
      ttable = std::unique_ptr<TT>(new TT());
    }
    killer_moves = std::vector<KillerMove<2>>(max_depth+1, {Move<SIZE>(), Move<SIZE>(), Evaluator::MIN, Evaluator::MIN});

    start = std::chrono::steady_clock::now();

    Score score = 0;
    Score lastScore = 0;

    for(int d = 1; d <= max_depth; d++) {
      Score t = lastScore;
      lastScore = score;
      score = mtdf(bestMove, t, state, d);
      auto entry = ttable->get(state);
      if(entry) {
        std::cout << "Best move for depth "<<d<<" "<<ptn::to_str(entry->move())<< std::endl;
      } else {
        std::cout << "Couldn't get move for depth " << d << std::endl;
      }
    }

    Move<SIZE> move;
    Board<SIZE> state_copy = state;
    while(true) {
      auto e = ttable->get(state_copy);
      if(e) {
        move = e->move();
        if(!(move.type() == Move<SIZE>::Type::PLACE && move.pieceType() == Piece::INVALID)) {
          std::cout << ptn::to_str(move) << " ";
          state_copy.execute(move);
        } else {
          break;
        }
      } else {
        break;
      }
    }
    std::cout << std::endl;

    end = std::chrono::steady_clock::now();
    time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end-start);
    std::cout << std::endl;
    std::cout << leaf_count << " leafs evaluated in " << time_span.count() << "s" << std::endl;
    std::cout << leaf_count/time_span.count() << " leafs/s" << std::endl;
    std::cout << "Hits: " << hits << std::endl;

    return score;
  }

  Score mtdf(Move<SIZE>& bestMove, Score guess, Board<SIZE>& state, int max_depth) {
    Score upperBound = Evaluator::MAX, lowerBound = Evaluator::MIN;
    while(lowerBound < upperBound) {
      Score beta = util::max<Score>(guess, lowerBound+1);
      guess = negamax(state, bestMove, 0, max_depth, beta-1, beta);
      if(guess < beta) upperBound = guess;
      else lowerBound = guess;
    }

    return guess;
  }

  Score negamax(Board<SIZE>& state, Move<SIZE>& bestMove, int ply, int depth, Score alpha, Score beta) {
    using Entry = typename TT::Entry;
    Score init_alpha = alpha;
    util::option<Entry> e;
    if(ttable) {
      e = ttable->get(state);
      if(e && e->depth() >= depth) {
        hits++;
        switch(e->type()) {
        case Entry::EXACT:
          return e->score();
        case Entry::BETA:
          alpha = util::max(alpha, e->score());
          if(alpha >= beta) {
            return e->score();
          }
          break;
        case Entry::ALPHA:
          beta = util::min(beta, e->score());
          if(alpha >= beta) {
            return e->score();
          }
          break;
        default:
          break;
        }
        hits--;
      }
    }

    if(depth <= 0) leaf_count++;
    GameStatus status = state.status();

    if(status.over) {
      int s = status.winner == state.curPlayer ? Evaluator::WIN+depth : Evaluator::LOSS-depth;
      if(ttable) ttable->put(state, Entry(Entry::EXACT, depth, s));
      return s;
    }

    if(depth <= 0) {
      if(depth < 0) {
        std::cout << "Error: depth " << depth << std::endl;
      }
      int s = Evaluator::eval(state, state.curPlayer);
      if(ttable) ttable->put(state, Entry(Entry::EXACT, depth, s));
      return s;
    } else {
      typename Board<SIZE>::Map map(state);

      struct MoveAndScore {
        Move<SIZE> m;
        int s;
      };

      bool nullCutoff = false;

      util::option<Move<SIZE>> hash_move;
      if(e) {
        hash_move = e->move();
      }

      auto score_move = [this,depth,&hash_move](Move<SIZE>& m) {
        if(hash_move && *hash_move == m) return 2;
        for(auto move : killer_moves[depth].moves) {
          if(move == m) return 1;
        }

        return 0;
      };

      std::vector<MoveAndScore> moves;
      state.forEachMove(map, [&moves, score_move](Move<SIZE> m) {
        moves.push_back({m, score_move(m)});
        return CONTINUE;
      });
      auto numMoves = moves.size();
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
      //Move<SIZE> bestMove;
      for(int i = 0; i < numMoves; i++) {
        Move<SIZE>& m = moves[i].m;
        Board<SIZE> check = state;
        check.execute(m);
        Move<SIZE> bm;
        Score score = -negamax(check, bm, ply+1, depth-1, -beta, -alpha);

        if(score > bestScore) {
          bestScore = score;
          bestMove = m;
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

          if(ttable) {
            ttable->put(state, Entry(Entry::BETA, depth, bestScore, bestMove));
          }
          return bestScore;
        }
      }

      if(ttable) {
        if(bestScore > init_alpha) {
          ttable->put(state, Entry(Entry::EXACT, depth, bestScore, bestMove));
        } else {
          ttable->put(state, Entry(Entry::ALPHA, depth, bestScore, bestMove));
        }
      }
      return bestScore;
    }
  }

  bool nullOK(int depth, Board<SIZE>& state) {
    //if(depth < NULL_MOVE_REDUCTION+1) return false;
    if(state.white.flats < 4 || state.black.flats < 4) return false;
    return true;
  }
};

/*
template<typename... T>
class ExtensionList;

template<>
class ExtensionList<> {
  void search() {}
  void expand() {}
  void eval() {}
  void update() {}
  void ret() {}
};

template<typename T1, typename... T>
class ExtensionList<T1, T...> {
  T1& t1;
  ExtensionList<T...> rest;

  void search() {
    t1.search();
    rest.search();
  }

  void expand() {
    t1.expand();
    rest.expand();
  }

  void eval() {
    t1.eval();
    rest.eval();
  }

  void update() {
    t1.update();
    rest.update();
  }

  void ret() {
    t1.ret();
    rest.ret();
  }
};

template<typename... Extensions>
class Negamax {
public:
private:
  struct Node {
    Score alpha, beta;
    enum class State {
      SEARCH, EXPAND, EVAL, UPDATE, RETURN,
    } state;
  };

  std::vector<Node> nodes;
  ExtensionList<Extensions...> extensions;

  inline void search(Node& node) {
    extensions.search();
  }

  inline void expand(Node& node) {
  }

  Score search(Game& g) {
    while(nodes.size() > 0) {
      Node& node = nodes.back();
      switch(node.state) {
      case SEARCH:
        search(node);
        break;
      case EXPAND:
        expand(node);
        break;
      case EVAL:
        eval(node);
        break;
      case UPDATE:
        update(node);
        break;
      case RETURN:
        ret(node);
        break;
      }

      nodes.pop();
    }
  }
};
*/
