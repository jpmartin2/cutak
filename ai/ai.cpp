#include <iostream>
#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include "tak/tps.hpp"
#include "alphabeta.hpp"

struct eval {
  enum {
    MIN = -501,
    MAX = 501,
    WIN = 500,
    LOSS = -500,
  };

  template<uint8_t SIZE>
  int operator()(Board<SIZE>& state) {
    return 0;
  }
};

int main() {
  Board<3> b;
  alphabeta ab(WHITE);

  int depth;

  std::cout << "Depth: " << std::flush;

  std::cin >> depth;

  std::vector<Move<3>> moves;

  std::string input;
  while(true) {
    Move<3> move;
    Board<3> b2 = b;
    int best = ab.search(b2, move, eval{}, depth);
    if(!(b2 == b)) {
      std::cout << "Error in undoing of moves:" << std::endl;
      std::cout << "original: " << tps::to_str(b) << std::endl;
      std::cout << "ai:       " << tps::to_str(b2) << std::endl;
      return -1;
    }
    b.execute(move);
    moves.push_back(move);

    std::cout << "Best move " << ptn::to_str(move) << " with score of " << best << std::endl;
    std::cout << tps::to_str(b) << std::endl;

    GameStatus s = b.status();

    if(s.over) {
      s.print();
      return 0;
    }
    std::cout << "> " << std::flush;
    std::cin >> input;
    if(input == "exit") break;
    else if(input == "status") {
      s = b.status();
      s.print();
      std::cout << std::endl;
    } else {

      if(ptn::from_str(input, move)) {
        b.execute(move);
        moves.push_back(move);

        s = b.status();

        if(s.over) {
          s.print();
          return 0;
        }

      } else {
        std::cout << "Invalid move!" << std::endl;
      }

      std::cout << tps::to_str(b) << std::endl;
    }
  }
}
