#pragma once

#include <sstream>
#include <regex>
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
  for(int j = SIZE-1; j >= 0; j--) {
    int num_prev_empty = 0;
    for(int i = 0; i < SIZE; i++) {
      if(board.board[i+j*SIZE].height == 0) {
        num_prev_empty++;
        if(i == SIZE-1) {
          b += "x"+(num_prev_empty > 1 ? std::to_string(num_prev_empty) : "");
        }
      } else {
        if(num_prev_empty > 0) {
          b += "x"+(num_prev_empty > 1 ? std::to_string(num_prev_empty) : "")+",";
          num_prev_empty = 0;
        }
        b += to_str(board.board[i+j*SIZE]);
        if(i < SIZE-1) b += ",";
      }
    }
    if(j > 0) b += "/";
  }

  b += board.curPlayer == WHITE ? " 1 " : " 2 ";
  b += std::to_string(board.round);

  return b;
}

template<uint8_t SIZE>
bool from_str(std::string tps, Board<SIZE>& board) {
  std::string sq_reg = "(?:x[1-9]?|[12]+[SC]?)";
  std::string row_reg = "(?:"+sq_reg+",){0,"+std::to_string(SIZE-1)+"}(?:"+sq_reg+")";
  std::string board_reg = "(?:"+row_reg+"\\/){"+std::to_string(SIZE-1)+"}(?:"+row_reg+")";
  std::string tps_str = "("+board_reg+") ([12]) ([0-9]+)";
  std::regex tps_rgx(tps_str);
  std::cout << tps_str << std::endl;

  std::smatch match;
  if(std::regex_search(tps, match, tps_rgx)) {
    board.curPlayer = std::stoi(match[2].str()) - 1;
    board.round = std::stoi(match[3].str());

    std::istringstream b(match[1].str());
    std::string row;
    int y = SIZE-1;
    while(std::getline(b, row, '/')) {
      std::istringstream r(row);
      std::string square;
      int x = 0;
      while(std::getline(r, square, ',')) {
        if(square[0] == 'x') {
          int num = 1;
          if(square.size() > 1) {
            num = square[1]-'0';
          }
          int end = x+num;
          if((end-1)/SIZE != x/SIZE) return false;
          for(; x < end; x++) {
            int idx = x+y*SIZE;
            board.board[idx].height = 0;
            board.board[idx].owners = 0;
            board.board[idx].top = Piece::FLAT;
          }
        } else {
          int idx = x+y*SIZE;
          for(char c : square) {
            board.board[idx].top = Piece::FLAT;
            if(c == '1') {
              board.board[idx].owners <<= 1;
              board.board[idx].height += 1;
            } else if(c == '2') {
              board.board[idx].owners <<= 1;
              board.board[idx].owners |= 1;
              board.board[idx].height += 1;
            } else if(c == 'S') {
              board.board[idx].top = Piece::WALL;
            } else if(c == 'C') {
              board.board[idx].top = Piece::CAP;
            }
          }
          x++;
        }
      }

      y--;
    }

    return true;
  } else {
    return false;
  }
}

} // namespace tps
