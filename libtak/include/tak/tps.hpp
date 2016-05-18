#pragma once

#include "board.hpp"

namespace tps {
inline std::string to_str(Stack& stack) {
  if(stack.height) {
    std::string s;
    uint64_t owners = stack.owners;
    for(int i = stack.height-1; i >= 0; i--) {
      if(owners&1) {
        s.insert(0, "2");
      } else {
        s.insert(0, "1");
      }
      owners >>= 1;
    }

    if(stack.top == Piece::CAP) {
      s += "C";
    } else if(stack.top == Piece::WALL) {
      s += "S";
    }
    return s;
  } else {
    return "x";
  }
}

template<uint8_t SIZE>
std::string to_str(Board<SIZE>& board) {
  std::string b;
  b += "[TPS \"";

  for(int j = SIZE-1; j >= 0; j--) {
    for(int i = 0; i < SIZE; i++) {
      b += to_str(board.board[i+j*SIZE]);
      if(i < SIZE-1) b += ",";
    }
    if(j > 0) b += "/";
  }

  b += board.curPlayer == WHITE ? " 1 " : " 2 ";
  b += std::to_string(board.round);
  b += " \"]";

  return b;
}

} // namespace tps
