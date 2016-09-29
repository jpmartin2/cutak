#include "tak/tak.hpp"
#include "tak/net/message.hpp"
#include "tak/ptn.hpp"
#include "tak/tps.hpp"
#include "alphabeta.hpp"
#include "eval.hpp"

int main(int argc, char** argv) {
  alphabeta<3, Eval> ab;

  Move<3> move(4, Piece::FLAT);
  Board<3> board;
  board.execute(move);
  //int i = 12;
  int i = 9;
  while(i > 1) {
    auto score = ab.search(board, move, i--);
    std::cout << "Best move: " << ptn::to_str(move) << " with score " << score << std::endl;
    board.execute(move);
    std::cout << tps::to_str(board) << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
  }
}
