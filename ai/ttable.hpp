#pragma once

#include "tak/tak.hpp"

namespace search {

template<size_t SIZE, size_t NUM_ENTRIES>
struct TTable {
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
    friend struct TTable;
    uint64_t data;
    Entry(uint64_t data) : data(data) {}
  public:
    using Score = int16_t;
    enum Type : uint8_t {
      INVALID = 0, ALPHA = 1, BETA = 2, EXACT = 3,
    };

    Entry(Type type, int depth, int16_t score) :
      data((((uint64_t)type&0x3)<<62) | (((uint64_t)depth&0x1FFF)<<49) | (((uint64_t)score&0xFFFF)<<33) | 3)
    {}

    Entry(Type type, int depth, int16_t score, Move<SIZE>& bestMove) :
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

    Entry() : data((((uint64_t)INVALID)<<62) | 3) {}

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

  inline Entry get(Board<SIZE>& b) {
    uint64_t hash = b.hash();
    auto& e = table[hash % NUM_ENTRIES];
    auto h = e.hash.load(std::memory_order_relaxed);
    auto d = e.data.load(std::memory_order_relaxed);
    if(Entry(d).type() == Entry::INVALID || (h ^ d) != hash) {
      return Entry();
    } else {
      return Entry(d);
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

} // namespace search
