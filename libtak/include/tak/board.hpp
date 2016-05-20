#pragma once

#include "util.hpp"
#include "table.hpp"
#include "move.hpp"
#include "game.hpp"
#include <string>
#include <algorithm>
#include <iostream>

enum ForContinue {
  CONTINUE, BREAK
};

struct Stack {
  uint64_t owners; // Who owns which piece, LSB is top piece
  uint8_t height; // Height of the stack
  Piece top; // Type of the top piece

  CUDA_CALLABLE inline Stack() : owners(0), height(0), top(Piece::FLAT) {}

  CUDA_CALLABLE inline uint8_t pop(int n) {
    uint8_t o = owners&~((-1)<<n);
    owners >>= n;
    height -= n;
    return o;
  }

  CUDA_CALLABLE inline void push(int n, uint8_t pieces) {
    owners = (owners << n) | pieces;
    height += n;
  }

  CUDA_CALLABLE inline uint8_t owner() const { return owners&1; }
};

template<uint8_t SIZE>
class Board {
public:
  Stack board[SIZE*SIZE];

  struct {
    uint8_t flats, caps;
  } white, black;

  uint8_t curPlayer;
  uint32_t round;

  class Map {
  private:
    uint8_t left[SIZE*SIZE];
    uint8_t right[SIZE*SIZE];
    uint8_t up[SIZE*SIZE];
    uint8_t down[SIZE*SIZE];
  public:
    friend class Board;
    CUDA_CALLABLE Map(Board& board) {
      int c, w;

      auto update = [&board, &c, &w](uint8_t target[], int i, int j) {
        int idx = i+j;
        Piece type = board.board[idx].height ? board.board[idx].top : Piece::INVALID;
        target[idx] = (((type==Piece::CAP)&&w)<<7)|c;
        c++;
        if(type == Piece::WALL || type == Piece::CAP) {
          c = 0;
          w = type==Piece::WALL;
        }
      };

      for(int j = 0; j < SIZE*SIZE; j+=SIZE) {
        c = w = 0;
        for(int i = 0; i < SIZE; i++) {
          update(left, i, j);
        }
      }

      for(int j = 0; j < SIZE*SIZE; j+=SIZE) {
        c = w = 0;
        for(int i = SIZE-1; i >= 0; i--) {
          update(right, i, j);
        }
      }

      for(int i = 0; i < SIZE; i++) {
        c = w = 0;
        for(int j = 0; j < SIZE*SIZE; j+=SIZE) {
          update(down, i, j);
        }
      }

      for(int i = 0; i < SIZE; i++) {
        c = w = 0;
        for(int j = SIZE*(SIZE-1); j >= 0; j-=SIZE) {
          update(up, i, j);
        }
      }
    }
  };

  CUDA_CALLABLE Board() :
    white({ num_flats<SIZE>::value, num_caps<SIZE>::value }),
    black({ num_flats<SIZE>::value, num_caps<SIZE>::value }),
    curPlayer(WHITE),
    round(1)
  {}

  // Check if the given player has a road on board
  CUDA_CALLABLE bool playerHasRoad(uint8_t player) const {
    // Just for convenience...
    const typename Move<SIZE>::Dir NORTH = Move<SIZE>::Dir::NORTH;
    const typename Move<SIZE>::Dir SOUTH = Move<SIZE>::Dir::SOUTH;
    const typename Move<SIZE>::Dir EAST = Move<SIZE>::Dir::EAST;
    const typename Move<SIZE>::Dir WEST = Move<SIZE>::Dir::WEST;

    // Bit vector of visited nodes
    uint64_t visited = 0;
    // Queue of currently waiting nodes
    uint8_t queue[SIZE*SIZE];
    uint8_t queueSize;

    auto visit = [this, player, &queue, &queueSize, &visited](uint8_t n) {
      if(board[n].height && (board[n].owner() == player) && (board[n].top != Piece::WALL)) {
        uint64_t nm = ((uint64_t)1)<<n;
        if((visited&nm) == 0) {
          visited |= nm;
          queue[queueSize++] = n;
        }
      }
    };

    // Search for a top<->bottom road
    visited = queueSize = 0;
    for(int i = 0; i < SIZE; i++) {
      if(board[i].height && (board[i].owner() == player) && board[i].top != Piece::WALL) {
        visited |= 1<<i;
        queue[queueSize++] = i;
      }
    }

    while(queueSize > 0) {
      uint8_t node = queue[--queueSize];

      if(node/SIZE == SIZE-1) {
        return true;
      }

      if((uint8_t)(node+NORTH) < SIZE*SIZE) visit(node+NORTH);
      if(((uint8_t)(node+EAST))/SIZE == node/SIZE) visit(node+EAST);
      if(((uint8_t)(node+WEST))/SIZE == node/SIZE) visit(node+WEST);
      if((uint8_t)(node+SOUTH) < SIZE*SIZE) visit(node+SOUTH);
    }

    // Search for a left<->right road
    visited = queueSize = 0;
    for(int i = 0; i < SIZE*SIZE; i += SIZE) {
      if(board[i].height && board[i].owner() == player && board[i].top != Piece::WALL) {
        visited |= 1<<i;
        queue[queueSize++] = i;
      }
    }

    while(queueSize > 0) {
      uint8_t node = queue[--queueSize];

      if(node%SIZE == SIZE-1) {
        return true;
      }

      if(((uint8_t)(node+EAST))/SIZE == node/SIZE) visit(node+EAST);
      if((uint8_t)((node+NORTH)) < SIZE*SIZE) visit(node+NORTH);
      if((uint8_t)((node+SOUTH)) < SIZE*SIZE) visit(node+SOUTH);
      if(((uint8_t)(node+WEST))/SIZE == node/SIZE) visit(node+WEST);
    }

    return false;
  }

  // Check if the board is full
  CUDA_CALLABLE bool checkBoardFull() const {
    for(int i = 0; i < SIZE*SIZE; i++) {
      if(!board[i].height) {
        return false;
      }
    }
    return true;
  }

  // Check if a game is over (and if so, who won and why)
  CUDA_CALLABLE GameStatus status() const {
    GameStatus s;

    // Check for roads
    bool whiteRoad = playerHasRoad(WHITE);
    bool blackRoad = playerHasRoad(BLACK);

    // If either player has a road
    if(whiteRoad || blackRoad) {
      // The game is a road victory of some kind
      s.over = true;
      s.condition = ROAD_VICTORY;
      // If there's two roads,
      if(whiteRoad && blackRoad) {
        // The player who just went (i.e. the one not about to go) wins
        s.winner = !curPlayer;
      } else {
        // Otherwise whoever had a road wins
        s.winner = blackRoad ? BLACK : WHITE;
      }
      return s;
    }

    // Check if the board is full
    bool boardFull = checkBoardFull();
    // If either player runs out of flats or the board is full,
    if((white.flats == 0 && white.caps == 0) ||
       (black.flats == 0 && black.caps == 0) ||
       boardFull) {
      // Game is a Flat victory
      s.over = true;
      s.condition = FLAT_VICTORY;
      // Count both players flats
      int w = 0, b = 0;
      for(int i = 0; i < SIZE*SIZE; i++) {
        if(board[i].height && board[i].top == Piece::FLAT) {
          if(board[i].owner() == WHITE) {
            w++;
          } else {
            b++;
          }
        }
      }

      // Winner is whoever had more flats on top
      if(w > b) { s.winner = WHITE; }
      else if(b > w) { s.winner = BLACK; }
      else { s.winner = TIE; }
    }

    return s;
  }

  template<typename Func>
  CUDA_CALLABLE void forEachMove(Map map, Func func) const {
    if(round == 1) {
      // On the first round you can only place flats
      for(int i = 0; i < SIZE*SIZE; i++) {
        if(board[i].height == 0) {
          if(func(Move<SIZE>(i, Piece::FLAT)) == BREAK) return;
        }
      }
    } else {
      for(int i = 0; i < SIZE*SIZE; i++) {
        if(board[i].height == 0) {
          int flats = curPlayer == WHITE ? white.flats : black.flats;
          int caps = curPlayer == WHITE ? white.caps : black.caps;
          if(flats > 0) {
            if(func(Move<SIZE>(i, Piece::FLAT)) == BREAK) return;
            if(func(Move<SIZE>(i, Piece::WALL)) == BREAK) return;
          }
          if(caps > 0) {
            if(func(Move<SIZE>(i, Piece::CAP)) == BREAK) return;
          }
        } else if(board[i].owner() == curPlayer) {
          int stack_size = util::min(SIZE,board[i].height);
          for(auto move : Table::moves(stack_size, map.left[i])) {
            //std::cout << "Stack size: " << stack_size << ", left[i] = "<< (int)map.left[i] << std::endl;
            if(func(Move<SIZE>(i, Move<SIZE>::Dir::WEST, move)) == BREAK) return;
          }
          for(auto move : Table::moves(stack_size, map.right[i])) {
            //std::cout << "Stack size: " << stack_size << ", left[i] = "<< (int)map.left[i] << std::endl;
            if(func(Move<SIZE>(i, Move<SIZE>::Dir::EAST, move)) == BREAK) return;
          }
          for(auto move : Table::moves(stack_size, map.up[i])) {
            //std::cout << "Stack size: " << stack_size << ", left[i] = "<< (int)map.left[i] << std::endl;
            if(func(Move<SIZE>(i, Move<SIZE>::Dir::NORTH, move)) == BREAK) return;
          }
          for(auto move : Table::moves(stack_size, map.down[i])) {
            //std::cout << "Stack size: " << stack_size << ", left[i] = "<< (int)map.left[i] << std::endl;
            if(func(Move<SIZE>(i, Move<SIZE>::Dir::SOUTH, move)) == BREAK) return;
          }
        }
      }
    }
  }

  CUDA_CALLABLE bool valid(Move<SIZE> m) const {
    switch(m.type()) {
    case Move<SIZE>::Type::MOVE: {
      if(m.idx() > SIZE*SIZE) {
        std::cout << "Invalid move: index out of range" << std::endl;
        return false;
      }
      if(board[m.idx()].owner() != curPlayer) {
        std::cout << "Inalid move: trying to move stack of wrong player" << std::endl;
        return false;
      }
      if(m.pieceType() == Piece::INVALID) return false;
      int slide_sum = 0;
      for(int n = m.range(); n > 0; n--) {
        uint8_t i = m.idx()+n*m.dir();
        if(board[i].height > 0) {
          if(n == m.range() && board[i].top == Piece::WALL) {
            if(board[m.idx()].top != Piece::CAP || m.slides(n) > 1) {
              std::cout << "Invalid move: only a cap alone can flatten a wall" << std::endl;
              return false;
            }
          } else if(board[i].top == Piece::WALL || board[i].top == Piece::CAP) {
            std::cout << "Invalid move: cannot move on top of wall or cap" << std::endl;
            return false;
          }
        }
        slide_sum += m.slides(n);
      }
      if(slide_sum > board[m.idx()].height) {
        std::cout << "Invalid move: trying to move too many pieces" << std::endl;
        return false;
      }
      return true; }
    case Move<SIZE>::Type::PLACE:
      return board[m.idx()].height == 0;
    default:
      return false;
    }
  }

  CUDA_CALLABLE void execute(Move<SIZE>& m) {
    switch(m.type()) {
    case Move<SIZE>::Type::MOVE:
      for(int n = m.range(); n > 0; n--) {
        int nDropped = m.slides(n);
        uint8_t dropped = board[m.idx()].pop(nDropped);
        board[(uint8_t)(m.idx()+n*m.dir())].push(nDropped, dropped);
        board[(uint8_t)(m.idx()+n*m.dir())].top = Piece::FLAT;
      }
      m.undo() = board[(uint8_t)(m.idx()+m.range()*m.dir())].top;
      board[(uint8_t)(m.idx()+m.range()*m.dir())].top = board[m.idx()].top;
      board[m.idx()].top = Piece::FLAT;
      break;
    case Move<SIZE>::Type::PLACE:
      switch(m.pieceType()) {
      case Piece::FLAT:
      case Piece::WALL:
        if(curPlayer == WHITE) white.flats--;
        else black.flats--;
        break;
      case Piece::CAP:
        if(curPlayer == WHITE) white.caps--;
        else black.caps--;
        break;
      default:
        break;
      }
      // Place for opposite player if round 1, otherwise place for current player
      board[m.idx()].owners = round == 1 ? !curPlayer : curPlayer;
      board[m.idx()].top = m.pieceType();
      board[m.idx()].height = 1;
      break;
    }

    // Increment the round counter if black just went
    round += curPlayer == BLACK;
    curPlayer = !curPlayer;
  }

  CUDA_CALLABLE void undo(Move<SIZE>& m) {
    curPlayer = !curPlayer;
    round -= curPlayer == BLACK;
    switch(m.type()) {
    case Move<SIZE>::Type::MOVE:
      board[m.idx()].top = board[(uint8_t)(m.idx()+m.range()*m.dir())].top;
      board[(uint8_t)(m.idx()+m.range()*m.dir())].top = m.undo();
      for(int n = 1; n <= m.range(); n++) {
        int nDropped = m.slides(n);
        uint8_t dropped = board[(uint8_t)(m.idx()+n*m.dir())].pop(nDropped);
        board[m.idx()].push(nDropped, dropped);
      }
      break;
    case Move<SIZE>::Type::PLACE:
      switch(m.pieceType()) {
      case Piece::FLAT:
      case Piece::WALL:
        if(curPlayer == WHITE) white.flats++;
        else black.flats++;
        break;
      case Piece::CAP:
        if(curPlayer == WHITE) white.caps++;
        else black.caps++;
        break;
      default:
        break;
      }
      board[m.idx()].height = 0;
      break;
    }
  }
};

template<uint8_t SIZE>
CUDA_CALLABLE bool operator==(Board<SIZE>& left, Board<SIZE>& right) {
  for(int i = 0; i < SIZE*SIZE; i++) {
    if(left.board[i] != right.board[i]) return false;
  }
  return left.white.flats == right.white.flats &&
         left.white.caps == right.white.caps &&
         left.black.flats == right.black.flats &&
         left.black.caps == right.black.caps &&
         left.curPlayer == right.curPlayer &&
         left.round == right.round;
}

template<uint8_t SIZE>
CUDA_CALLABLE bool operator!=(Board<SIZE>& left, Board<SIZE>& right) { return !(left == right); }

CUDA_CALLABLE bool operator==(Stack left, Stack right) {
  if(left.height != right.height) return false;
  if(left.height == 0) return true;
  uint64_t mask = ~((-1)<<left.height);

  return (left.owners&mask) == (right.owners&mask) &&
         left.top == right.top;
}

CUDA_CALLABLE bool operator!=(Stack& left, Stack& right) { return !(left == right); }
