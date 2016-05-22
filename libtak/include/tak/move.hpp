#pragma once
#include "game.hpp"
#include <string>

template<uint8_t SIZE>
class Move {
public:
  enum Dir : uint8_t {
    NORTH = (uint8_t)SIZE,
    SOUTH = (uint8_t)(-(int8_t)SIZE),
    WEST = (uint8_t)(-(int8_t)1),
    EAST = (uint8_t)1,
  };

  enum class Type : uint8_t {
    PLACE = 0,
    MOVE = 1,
  };

  //CUDA_CALLABLE inline Move() : idx_(0), type_(Type::PLACE), data(Piece::FLAT) {}
  CUDA_CALLABLE inline Move() = default;

  CUDA_CALLABLE inline Move(uint8_t idx, Piece pieceType) :
    idx_(idx), type_(Type::PLACE), data(pieceType) {}

  CUDA_CALLABLE inline Move(uint8_t idx, Dir dir, Table::Index slides) :
    idx_(idx), type_(Type::MOVE), data(dir, slides) {}

  CUDA_CALLABLE inline Move(uint8_t idx, Dir dir, uint8_t range, uint8_t slides[SIZE-1]) :
    idx_(idx), type_(Type::MOVE), data(dir, range, slides) {}

  CUDA_CALLABLE inline uint8_t idx() { return idx_; }
  CUDA_CALLABLE inline Type type() { return type_; }

  /**
   * If this move is a PLACE, check the piece type to be placed.
   */
  CUDA_CALLABLE inline Piece pieceType() { return data.placement.pieceType; }

  /**
   * If this move is a MOVE, check the direction to move in
   **/
  CUDA_CALLABLE inline Dir dir() { return data.movement.dir; }

  /**
   * If this move is a MOVE, check number of tile this move covers.
   * I.e., the max distance from the source tile pieces will end up
   * as a result of this move.
   */
  CUDA_CALLABLE inline uint8_t range() { return data.movement.range; }

  /**
   * If this move is a MOVE, gets the number of pieces to be placed n
   * tiles from the source tile.
   */
  CUDA_CALLABLE inline uint8_t slides(int n) { return data.movement.slides[n-1]; }

  CUDA_CALLABLE inline Piece& undo() { return data.movement.undo; }
private:
  uint8_t idx_;
  Type type_;

  union Data {
    // Data for if this move is a PLACE
    struct {
      Piece pieceType;
    } placement;
    // Data for if this move is a MOVE
    struct {
      Dir dir;
      uint8_t range;
      uint8_t slides[SIZE-1];
      // Used to put back things the way they were
      Piece undo;
    } movement;

    CUDA_CALLABLE inline Data() = default;
    CUDA_CALLABLE inline Data(Piece pieceType) : placement({pieceType}) {}
    CUDA_CALLABLE inline Data(Dir dir, uint8_t range, uint8_t slides[SIZE-1]) {
      movement.dir = dir;
      movement.range = range;
      for(int i = 0; i < SIZE-1; i++) {
        movement.slides[i] = slides[i];
      }
    }
    CUDA_CALLABLE inline Data(Dir dir, Table::Index slides) {
      movement.dir = dir;
      movement.range = Table::value(slides, 0);
      for(int i = 0; i < SIZE-1; i++) {
        movement.slides[i] = Table::value(slides, i+1);
      }
    }
  } data;
};

template<uint8_t SIZE>
CUDA_CALLABLE inline bool operator==(Move<SIZE>& left, Move<SIZE>& right) {
  if(left.idx() != right.idx() || left.type() != right.type()) return false;
  if(left.type() == Move<SIZE>::Type::PLACE) {
    return left.pieceType() == right.pieceType();
  } else {
    if(left.dir() != right.dir() || left.range() != right.range()) return false;
    for(int i = 1; i <= left.range(); i++) {
      if(left.slides(i) != right.slides(i)) return false;
    }

    return true;
  }
}
