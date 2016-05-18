#include <iostream>
#include "tak/board.hpp"
#include "tak/ptn.hpp"
#include "tak/tps.hpp"

int main() {
  Board<3> b;

  int depth;

  std::cout << "Depth: " << std::flush;

  std::cin >> depth;

  std::vector<Move<3>> moves;

  std::string input;
  while(true) {
    GameStatus s = b.status();
    Move<3> move;

    std::cout << "> " << std::flush;
    std::cin >> input;
    if(input == "exit") break;
    else if(input == "undo") {
      if(moves.size() > 0) {
        b.undo(moves.back());
        moves.pop_back();
      } else {
        std::cout << "No moves to undo!" << std::endl;
      }
    } else if(input == "status") {
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
