#include <iostream>
#include <thread>
#include <cstdlib>
#include <regex>
#include <fstream>
#include <queue>
#include "asio.hpp"
#include "tak/tak.hpp"
#include "tak/ptn.hpp"
#include "tak/tps.hpp"
#include "alphabeta.hpp"

const uint8_t SIZE = 5;

using asio::ip::tcp;
using err_t = std::error_code;

std::regex game_start_rgx("Game Start ([0-9]+) ([3-8]) ([^ ]+) vs ([^ ]+) (white|black)");
std::regex game_add_rgx("GameList Add Game\\#([0-9]+) ([^ ]+) vs ([^ ]+), ([0-9]+)x([0-9]+), ([0-9]+), ([0-9]+) half-moves played, ([^ ]+) to move");
std::regex game_rem_rgx("GameList Remove Game\\#([0-9]+) ([^ ]+) vs ([^ ]+), ([0-9]+)x([0-9]+), ([0-9]+), ([0-9]+) half-moves played, ([^ ]+) to move");
std::regex seek_new_rgx("Seek new ([0-9]+) ([^ ]+) ([3-8]) ([0-9]+)( [WB])?");
std::regex seek_rem_rgx("Seek remove ([0-9]+) ([^ ]+) ([3-8]) ([0-9]+)( [WB])?");
std::regex move_rgx("Game#([0-9]+) M ([A-F][1-8]) ([A-F][1-8])((?: [1-8])+)");
std::regex place_rgx("Game#([0-9]+) P ([A-F][1-8])(?: ([CW]))?");
std::regex shout_rgx("Shout <([^>]+)> (.*)");
std::regex welcome_rgx("Welcome (.*)");
std::regex game_over_rgx("Game#([0-9]+) Over (.*)");

struct login_t { std::string username, password; };

struct seek_t { std::string game_no; std::string player; int size; };

struct eval_t {
  enum {
    MIN = -65535,
    MAX = 65535,
    WIN = 500,
    LOSS = -500,
  };

  template<uint8_t SIZE>
  int operator()(Board<SIZE>& state, uint8_t player) {
    int score = 0;
    for(Stack s : state.board) {
      if(s.top == Piece::FLAT && s.height) {
        if(s.owner() == player) {
          score++;
        } else {
          score--;
        }
      }
    }
    return score;
  }
};

class client {
public:
  client(asio::io_service& io, tcp::resolver::iterator endpoints, login_t login, std::vector<std::string> whitelist) : io(io), sock(io), auth(login), whitelist(whitelist) {
    connect(endpoints);
  }

  void quit() {
    send_msg("quit");
  }
private:
  void send_msg(std::string msg) {
    io.post([this, msg]() {
      send_msg_io(msg);
    });
  }

  void send_msg_io(std::string msg) {
    bool send_in_progress = msg_queue.size() > 0;
    msg_queue.push(msg);
    if(!send_in_progress) {
      std::cout << "Starting send...." << std::endl;
      do_send_msg();
    }
  }

  void connect(tcp::resolver::iterator endpoints) {
    asio::async_connect(sock, endpoints, [this](err_t err, tcp::resolver::iterator) {
      if(!err) {
        std::thread([this]() {
          while(true) {
            send_msg("PING\n");
            std::this_thread::sleep_for(std::chrono::seconds(30));
          }
        }).detach();
        readline();
      } else {
        std::cout << "Error connecting socket: " << err.message() << std::endl;
        std::exit(-1);
      }
    });
  }

  void doMove() {
    Board<SIZE> g = game;
    uint8_t pl = player;
    std::string cg = cur_game;
    std::thread([g,pl,cg,this]() mutable {
      alphabeta ab(pl);
      Move<SIZE> move;
      int score = ab.search(g, move, eval_t(), 5);
      std::cout << "Best move: " << ptn::to_str(move) << " with score " << score << std::endl;
      std::string m;
      switch(move.type()) {
      case Move<SIZE>::Type::MOVE: {
        char x1 = (move.idx()%SIZE) + 'A';
        char y1 = (move.idx()/SIZE) + '1';
        uint8_t end_idx = move.idx()+move.range()*move.dir();
        char x2 = (end_idx%SIZE) + 'A';
        char y2 = (end_idx/SIZE) + '1';
        m = "Game#" + cur_game + " M " + x1 + y1 + " " + x2 + y2;
        for(int i = 1; i <= move.range(); i++) {
          m += " ";
          m += (char)('0'+move.slides(i));
        }
      }
      break;
      case Move<SIZE>::Type::PLACE: {
        char x1 = (move.idx()%SIZE) + 'A';
        char y1 = (move.idx()/SIZE) + '1';
        m = "Game#" + cg + " P " + x1 + y1;
        if(move.pieceType() == Piece::CAP) {
          m += " C";
        } else if(move.pieceType() == Piece::WALL) {
          m += " W";
        }
      }
      break;
      }
      m += "\n";

      io.post([cg,m,move,this]() mutable {
        if(cg == cur_game) {
          this->game.execute(move);
          std::cout << "Current board state: " << tps::to_str(this->game) << std::endl;
          std::cout << "Sending move `" << m << "' to server..." << std::endl;
          send_msg_io(m);
        }
      });
    }).detach();
  }

  void readline() {
    asio::async_read_until(sock, buf, '\n', [this](err_t err, std::size_t) {
      if(!err) {
        std::istream is(&buf);
        std::string line;
        std::getline(is, line);

        std::smatch match;

        if(line == "Login or Register") {
          seeks.clear();
          send_msg_io("Login " + auth.username + " " + auth.password + "\n");
        } else if(std::regex_match(line, match, welcome_rgx)) {
          std::cout << "Connected..." << std::endl;
          send_msg_io("Seek "+std::to_string(SIZE)+" 600\n");
        } else if(std::regex_search(line, match, game_add_rgx)) {
        } else if(std::regex_search(line, match, game_rem_rgx)) {
        } else if(std::regex_search(line, match, seek_new_rgx)) {
          seeks.push_back(seek_t { match[1], match[2], std::stoi(match[3]) });
        } else if(std::regex_search(line, match, seek_rem_rgx)) {
          std::remove_if(seeks.begin(), seeks.end(), [&](seek_t s) { return s.game_no == match[1]; });
        } else if(std::regex_search(line, match, shout_rgx) && (std::find(whitelist.begin(), whitelist.end(), match[1]) != whitelist.end())) {
          std::cout << "Got message from <" << match[1] << ">: " << match[2] << std::endl;
          if(match[2] == "cutak_bot: play") {
            auto seek = std::find_if(seeks.begin(), seeks.end(), [&](seek_t s) { return s.player == match[1]; });
            if(seek != seeks.end()) {
              if(seek->size != SIZE) {
                send_msg_io("Shout I can only play 5x5 games right now.");
              } else {
                send_msg_io("Accept "+seek->game_no+"\n");
              }
            } else {
              send_msg_io("Shout I couldn't find a game of yours to join. Please create one.\n");
            }
          }
        } else if(std::regex_search(line, match, game_start_rgx)) {
          std::cout << "Playing game as " << match[5] << std::endl;
          player = match[5] == "white" ? WHITE : BLACK;
          cur_game = match[1];
          game = Board<SIZE>();
          if(player == WHITE) {
            doMove();
          }
        } else if(std::regex_search(line, match, move_rgx)) {
          std::cout << "Got move from server: " << line << std::endl;
          uint8_t idx = parse_square_idx<SIZE>(match[2].str());
          uint8_t end_idx = parse_square_idx<SIZE>(match[3].str());
          int range = 0;
          using Dir = typename Move<SIZE>::Dir;
          Dir dir;
          if(idx%SIZE == end_idx%SIZE) {
            range = end_idx/SIZE - idx/SIZE;
            dir = Dir::NORTH;
            if(range < 0) {
              range = -range;
              dir = Dir::SOUTH;
            }
          } else if(idx/SIZE == end_idx/SIZE) {
            range = end_idx - idx;
            dir = Dir::EAST;
            if(range < 0) {
              range = -range;
              dir = Dir::WEST;
            }
          } else {
            std::cout << "Invalid move `" << line << "' from server" << std::endl;
          }
          std::istringstream slides_str(match[4].str());
          std::string slide;
          uint8_t slides[SIZE-1] = { 0 };
          int i = 0;
          while(std::getline(slides_str, slide, ' ')) {
            if(slide.size()) {
              if(i >= sizeof(slides)) break;
              slides[i++] = std::stoi(slide);
            }
          }
          Move<SIZE> m(idx, dir, range, slides);
          std::cout << "Got move from server: " << ptn::to_str(m) << std::endl;
          if(match[1] == cur_game) {
            if(!game.valid(m)) {
              std::cout << "Invalid move `" << line << "' from server" << std::endl;
            }
            game.execute(m);
            std::cout << "Current board state: " << tps::to_str(this->game) << std::endl;
            std::cout << "Move is for current game" << std::endl;
            doMove();
          }
        } else if(std::regex_search(line, match, place_rgx)) {
          std::cout << "Got move from server: " << line << std::endl;
          Piece type = Piece::FLAT;
          if(match[3] == "C") type = Piece::CAP;
          else if(match[3] == "W") type = Piece::WALL;
          uint8_t idx = match[2].str()[0]-'A' + (match[2].str()[1]-'1')*SIZE;
          Move<SIZE> m(idx, type);
          std::cout << "Got move from server: " << ptn::to_str(m) << std::endl;
          if(match[1] == cur_game) {
            if(!game.valid(m)) {
              std::cout << "Invalid move `" << line << "' from server" << std::endl;
            }
            game.execute(m);
            std::cout << "Current board state: " << tps::to_str(this->game) << std::endl;
            std::cout << "Move is for current game" << std::endl;
            doMove();
          }
        } else if(std::regex_search(line, match, game_over_rgx)) {
          send_msg_io("Seek "+std::to_string(SIZE)+" 600\n");
        } else {
          std::cout << "Got unrecognized message `" << line << "'" << std::endl;
        }

        readline();
      } else {
        std::cout << "Error reading socket: " << err.message() << std::endl;
        std::exit(-1);
        //readline();
      }
    });
  }

  template<uint8_t SIZE>
  uint8_t parse_square_idx(std::string sq) {
    if(sq.size() == 2) {
      uint8_t x = std::toupper(sq[0])-'A';
      uint8_t y = sq[1]-'1';
      if(x >= SIZE) std::cout << "x of " << x << " is too big" << std::endl;
      if(y >= SIZE) std::cout << "y of " << y << " is too big" << std::endl;
      return x + y*SIZE;
    } else {
      return -1;
    }
  }

  void do_send_msg() {
    asio::async_write(sock, asio::buffer(msg_queue.front()), [this](err_t err, std::size_t) {
      if(!err) {
        std::cout << "Send message `" << msg_queue.front() << "' to server" << std::endl;
        msg_queue.pop();
        if(msg_queue.size()) {
          std::cout << "Sending another" << std::endl;
          do_send_msg();
        }
      } else {
        std::cout << "Error sending msg: " << err.message() << std::endl;
        std::exit(-1);
        //do_send_msg();
      }
    });
  }

  Board<SIZE> game;
  uint8_t player;
  std::string cur_game;
  asio::streambuf buf;
  std::queue<std::string> msg_queue;
  std::vector<std::string> whitelist;
  std::vector<seek_t> seeks;
  asio::io_service& io;
  tcp::socket sock;
  login_t auth;
};

int main(int argc, char** argv) {
  if(argc < 3) {
    std::cout << "please specify a server to connect to" << std::endl;
    return -1;
  }

  std::ifstream auth("auth.txt");
  if(!auth.is_open()) {
    std::cout << "Failed to open auth.txt" << std::endl;
    return -1;
  }

  std::ifstream f("whitelist.txt");
  if(!auth.is_open()) {
    std::cout << "Failed to open whitelist.txt" << std::endl;
    return -1;
  }
  std::string name;
  std::vector<std::string> whitelist;
  while(std::getline(f, name)) {
    whitelist.push_back(name);
  }

  login_t login;
  std::getline(auth, login.username);
  std::getline(auth, login.password);

  asio::io_service io;
  tcp::resolver resolver(io);
  tcp::socket socket(io);
  tcp::resolver::iterator endpoints = resolver.resolve({argv[1], argv[2]});
  client c(io, endpoints, login, whitelist);
  std::thread io_thread([&io](){ io.run(); });

  while(true) {
    std::string line;
    std::getline(std::cin, line);
    if(line == "quit") {
      c.quit();
      break;
    }
  }

  io_thread.join();
}
