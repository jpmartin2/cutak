#include "tak/dynamic.hpp"

/*
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
*/

DynamicBoard::DynamicBoard(int size) : size(size) {//, board(size) {}
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

void DynamicBoard::execute(DynamicMove& move) {
  switch(size) {
  case 3: {
    Move<3> m = move;
    three.execute(m);
    break;
          }
  case 4: {
    Move<4> m = move;
    four.execute(m);
    break;
          }
  case 5: {
    Move<5> m = move;
    five.execute(m);
    break;
          }
  case 6: {
    Move<6> m = move;
    six.execute(m);
    break;
          }
  case 7: {
    Move<7> m = move;
    seven.execute(m);
    break;
          }
  case 8: {
    Move<8> m = move;
    eight.execute(m);
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
    three.undo(m);
    break;
          }
  case 4: {
    Move<4> m = move;
    four.undo(m);
    break;
          }
  case 5: {
    Move<5> m = move;
    five.undo(m);
    break;
          }
  case 6: {
    Move<6> m = move;
    six.undo(m);
    break;
          }
  case 7: {
    Move<7> m = move;
    seven.undo(m);
    break;
          }
  case 8: {
    Move<8> m = move;
    eight.undo(m);
    break;
          }
  default:
    break;
  }
}

void DynamicBoard::accept(DynamicBoard::Visitor& v) {
  switch(size) {
  case 3:
    v.visit(three);
    break;
  case 4:
    v.visit(four);
    break;
  case 5:
    v.visit(five);
    break;
  case 6:
    v.visit(six);
    break;
  case 7:
    v.visit(seven);
    break;
  case 8:
    v.visit(eight);
    break;
  default:
    break;
  }
}
