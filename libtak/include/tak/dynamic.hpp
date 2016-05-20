#pragma once

// A move that works for any size board
class DynamicMove {
  int x, y;

  enum Type {
    MOVE, PLACE,
  } type;

  enum Dir {
    NORTH, SOUTH, EAST, WEST,
  };

  union {
    struct { Piece pieceType; } placement;
    struct {
      Dir dir;
      uint8_t range;
      std::vector<uint8_t> slides;
      // Used to restore board state for an undo
      Piece undo;
    } movement;
  } data;
};

class DynamicBoard {
public:
  class Visitor {
    virtual inline void visit(Board<3>& board) {}
    virtual inline void visit(Board<4>& board) {}
    virtual inline void visit(Board<5>& board) {}
    virtual inline void visit(Board<6>& board) {}
    virtual inline void visit(Board<7>& board) {}
    virtual inline void visit(Board<8>& board) {}
  };

  Board(int size);

  execute(DynamicMove& move);
  undo(DynamicMove& move);
  accept(Visitor& v);
private:
  int size;
  union B {
    Board<3> three;
    Board<4> four;
    Board<5> five;
    Board<6> six;
    Board<7> seven;
    Board<8> eight;

    B(int size);
  } board;
};
