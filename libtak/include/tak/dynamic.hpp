#pragma once

#include <vector>

#include "tak/tak.hpp"
#include "tak/move.hpp"
#include "tak/board.hpp"

// A move that works for any size board
class DynamicMove {
public:
  enum class Type : uint8_t {
    MOVE, PLACE,
  };

  enum class Dir : uint8_t {
    NORTH, SOUTH, EAST, WEST,
  };

  template<uint8_t SIZE>
  operator Move<SIZE>() {
    if(type_ == Type::MOVE) {
      typename Move<SIZE>::Dir dir;
      switch(data.movement.dir) {
      case Dir::NORTH:
        dir = Move<SIZE>::Dir::NORTH;
        break;
      case Dir::SOUTH:
        dir = Move<SIZE>::Dir::SOUTH;
        break;
      case Dir::EAST:
        dir = Move<SIZE>::Dir::EAST;
        break;
      case Dir::WEST:
        dir = Move<SIZE>::Dir::WEST;
        break;
      }
      return Move<SIZE>(x_+y_*SIZE, dir, data.movement.range, data.movement.slides);
    } else {
      return Move<SIZE>(x_+y_*SIZE, data.placement.pieceType);
    }
  }

  template<uint8_t SIZE>
  DynamicMove(Move<SIZE>& move) :
    x_(move.idx()%SIZE), y_(move.idx()/SIZE), data(move)
  {
    if(move.type() == Move<SIZE>::Type::PLACE) {
      type_ = Type::PLACE;
    } else {
      type_ = Type::MOVE;
    }
  }

  inline DynamicMove(uint8_t x, uint8_t y, Piece pieceType) : x_(x), y_(y), type_(Type::PLACE), data(pieceType) {}
  inline DynamicMove(uint8_t x, uint8_t y, Dir dir, uint8_t range, std::vector<uint8_t> slides) :
    x_(x), y_(y), type_(Type::MOVE), data(dir, range, slides) {}

  inline Type type() { return type_; }
  inline uint8_t x() { return x_; }
  inline uint8_t y() { return y_; }

  Piece pieceType() { return data.placement.pieceType; }

  uint8_t range() { return data.movement.range; }
  Dir dir() { return data.movement.dir; }
  uint8_t slides(uint8_t n) { return data.movement.slides[n-1]; }
private:
  int x_, y_;
  Type type_;

  union Data {
    struct { Piece pieceType; } placement;
    struct {
      Dir dir;
      uint8_t range;
      // Big enough to hold slides for any move
      uint8_t slides[7];
      // Used to restore board state for an undo
      Piece undo;
    } movement;

    Data(Piece pieceType) : placement({ pieceType }) {}
    Data(Dir dir, uint8_t range, std::vector<uint8_t> slides) {
      movement.dir = dir;
      movement.range = range;
      int i = 0;
      for(; i < slides.size() && i < 7; i++) {
        movement.slides[i] = slides[i];
      }

      for(; i < 7; i++) {
        movement.slides[i] = 0;
      }
    }
    template<uint8_t SIZE>
    Data(Move<SIZE>& move) {
      if(move.type() == Move<SIZE>::Type::PLACE) {
        placement.pieceType = move.pieceType();
      } else {
        switch(move.dir()) {
        case Move<SIZE>::Dir::NORTH:
          movement.dir = Dir::NORTH;
          break;
        case Move<SIZE>::Dir::SOUTH:
          movement.dir = Dir::SOUTH;
          break;
        case Move<SIZE>::Dir::EAST:
          movement.dir = Dir::EAST;
          break;
        case Move<SIZE>::Dir::WEST:
          movement.dir = Dir::WEST;
          break;
        }
        movement.range = move.range();
        for(int i = 0; i < SIZE-1; i++) {
          movement.slides[i] = move.slides(i+1);
        }
        movement.undo = move.undo();
      }
    }
  } data;
};

class DynamicBoard {
public:
  class Visitor {
  private:
    friend class DynamicBoard;
    virtual void visit(Board<3>& board) = 0;
    virtual void visit(Board<4>& board) = 0;
    virtual void visit(Board<5>& board) = 0;
    virtual void visit(Board<6>& board) = 0;
    virtual void visit(Board<7>& board) = 0;
    virtual void visit(Board<8>& board) = 0;
  };

  DynamicBoard(int size);

  void execute(DynamicMove& move);
  void undo(DynamicMove& move);
  void accept(Visitor& v);
private:
  int size;
  union {
    struct { Board<3> three; };
    struct { Board<4> four; };
    struct { Board<5> five; };
    struct { Board<6> six; };
    struct { Board<7> seven; };
    struct { Board<8> eight; };
  };
};
