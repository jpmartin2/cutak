#pragma once

#include <cstdint>
#include <cstdio>

#include "util.hpp"

template<int size>
struct num_flats;

template<int size>
struct num_caps;

#define PIECES \
  GAME(3, 10, 0) \
  GAME(4, 15, 0) \
  GAME(5, 21, 1) \
  GAME(6, 30, 1) \
  GAME(7, 40, 2) \
  GAME(8, 50, 2)

#define GAME(size, flats, caps) \
  template<> struct num_flats<size> { enum { value = flats }; }; \
  template<> struct num_caps<size> { enum { value = caps }; };

PIECES

#undef GAME
#undef PIECES

enum Player {
  WHITE = 0,
  BLACK = 1,
  NEITHER = 2,
  TIE = 2,
};

enum VictoryCondition {
  ROAD_VICTORY = 0,
  FLAT_VICTORY = 1,
  RESIGNATION  = 2,
  OFFERED_DRAW = 3,
};

struct GameStatus {
  bool over;
  uint8_t winner;
  uint8_t condition;

  CUDA_CALLABLE inline GameStatus() : over(false) {}

  void print() {
    if(over) {
      const char* color = winner == WHITE ? "white" : (winner == BLACK ? "black" : "neither player");
      const char* reason = condition == ROAD_VICTORY ? " road victory" :
                           condition == FLAT_VICTORY ? " flat victory" :
                           condition == RESIGNATION  ? " resignation" : "n offered draw";
      printf("Game over:\n");
      printf("  %s has won via\n", color);
      printf("  a%s.\n", reason);
    } else {
      printf("Game in progress.\n");
    }
  }
};

enum class Piece : uint8_t {
  FLAT = 0,
  WALL = 1,
  CAP  = 2,
  INVALID = 3,
};
