#pragma once

#include "tak/tak.hpp"
#include <random>

struct Eval {
  using Score = int32_t;

  enum S : Score {
    MIN = -(1<<30),
    MAX = (1<<30),
    LOSS = -(1<<29),
    WIN = 1<<29,
  };

  static std::mt19937 generator;
  static std::uniform_int_distribution<int> distribution;

  // Evaluates the strength of one player
  template<uint8_t SIZE>
  static Score eval_player(const Board<SIZE>& state, uint8_t player) {
    int top_flats = 0;
    int adj_flats = 0;
    int flats = 0;
    int caps = 0;
    int influence = 0;
    int captured = 0;
    int captured_penalty = 0;
    for(int i = 0; i < SIZE*SIZE; i++) {
      Stack s = state.board[i];
      int cap_this_stack = 0;
      if(s.height && s.top == Piece::FLAT && s.owner() == player) {
        top_flats++;
        uint8_t o = i+Move<SIZE>::Dir::NORTH;
        if(o < SIZE*SIZE && state.board[o].height && state.board[o].owner() == player && state.board[o].top == Piece::FLAT) {
          adj_flats++;
        }
        o = i+Move<SIZE>::Dir::SOUTH;
        if(o < SIZE*SIZE && state.board[o].height && state.board[o].owner() == player && state.board[o].top == Piece::FLAT) {
          adj_flats++;
        }
        o = i+Move<SIZE>::Dir::EAST;
        if(o/SIZE == i/SIZE && state.board[o].height && state.board[o].owner() == player && state.board[o].top == Piece::FLAT) {
          adj_flats++;
        }
        o = i+Move<SIZE>::Dir::WEST;
        if(o/SIZE == i/SIZE && state.board[o].height && state.board[o].owner() == player && state.board[o].top == Piece::FLAT) {
          adj_flats++;
        }
        //influence += (0x7F&map.left[i]) + (0x7F&map.right[i]) + (0x7F&map.up[i]) + (0x7F&map.down[i]);
        uint64_t owners = state.board[i].owners;
        for(int i = 1; i < state.board[i].height; i++) {
          owners >>= 1;
          if((owners&1) == player) {
            flats += 1;
          } else {
            captured += 1;
            cap_this_stack += 1;
          }
        }
        if(cap_this_stack >= 3) {
          captured_penalty += cap_this_stack*cap_this_stack;
        }
      } else if(s.height && s.top == Piece::CAP && s.owner() == player) {
        caps++;
      }

      int adj_ally = 0;
      int adj_enemy = 0;
      uint8_t o = i+Move<SIZE>::Dir::NORTH;
      if(o < SIZE*SIZE && state.board[o].height) {
        if(state.board[o].owner() == player) {
          adj_ally++;
        } else {
          adj_enemy++;
        }
      }
      o = i+Move<SIZE>::Dir::SOUTH;
      if(o < SIZE*SIZE && state.board[o].height) {
        if(state.board[o].owner() == player) {
          adj_ally++;
        } else {
          adj_enemy++;
        }
      }
      o = i+Move<SIZE>::Dir::EAST;
      if(o/SIZE == i/SIZE && state.board[o].height) {
        if(state.board[o].owner() == player) {
          adj_ally++;
        } else {
          adj_enemy++;
        }
      }
      o = i+Move<SIZE>::Dir::WEST;
      if(o/SIZE == i/SIZE && state.board[o].height) {
        if(state.board[o].owner() == player) {
          adj_ally++;
        } else {
          adj_enemy++;
        }
      }

      influence += adj_ally-adj_enemy;
    }

    return influence*25 + (top_flats+adj_flats/2)*400 + flats*100 + caps*50 - captured_penalty*100;
  }

  template<uint8_t SIZE>
  static Score eval(const Board<SIZE>& state, uint8_t player) {
    return eval_player(state, player) - eval_player(state, !player) + distribution(generator);
  }
};

