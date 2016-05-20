#include "tak/dynamic.hpp"

DynamicBoard::B::B(int size) {
  switch(size) {
  case 3:
    three = Board<3>();
    break;
  case 4:
    four = Board<4>();
    break;
  case 5:
    five = Board<5>();
    break;
  case 6:
    six = Board<6>();
    break;
  case 7:
    seven = Board<7>();
    break;
  case 8:
    eight = Board<8>();
    break;
  default:
    std::cout << "Boards of size < 3 or > 8 not supported!" << std::endl;
    break;
  }
}

DynamicBoard::DynamicBoard(int size) : size(size), board(size) {}

void DynamicBoard::execute(DynamicMove& move) {
  switch(size) {
  case 3: {
    Move<3> m = move;
    board.three.execute(m);
    break;
          }
  case 4: {
    Move<4> m = move;
    board.four.execute(m);
    break;
          }
  case 5: {
    Move<5> m = move;
    board.five.execute(m);
    break;
          }
  case 6: {
    Move<6> m = move;
    board.six.execute(m);
    break;
          }
  case 7: {
    Move<7> m = move;
    board.seven.execute(m);
    break;
          }
  case 8: {
    Move<8> m = move;
    board.eight.execute(m);
    break;
          }
  default:
    break;
  }
}

void DynamicBoard::undo(DynamicMove& move) {
  switch(size) {
  case 3: {
    Move<3> m = move;
    board.three.undo(m);
    break;
          }
  case 4: {
    Move<4> m = move;
    board.four.undo(m);
    break;
          }
  case 5: {
    Move<5> m = move;
    board.five.undo(m);
    break;
          }
  case 6: {
    Move<6> m = move;
    board.six.undo(m);
    break;
          }
  case 7: {
    Move<7> m = move;
    board.seven.undo(m);
    break;
          }
  case 8: {
    Move<8> m = move;
    board.eight.undo(m);
    break;
          }
  default:
    break;
  }
}

void DynamicBoard::accept(DynamicBoard::Visitor& v) {
  switch(size) {
  case 3:
    v.visit(board.three);
    break;
  case 4:
    v.visit(board.four);
    break;
  case 5:
    v.visit(board.five);
    break;
  case 6:
    v.visit(board.six);
    break;
  case 7:
    v.visit(board.seven);
    break;
  case 8:
    v.visit(board.eight);
    break;
  default:
    break;
  }
}
