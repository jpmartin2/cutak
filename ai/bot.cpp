#include <iostream>
#include <thread>
#include <cstdlib>
#include <regex>
#include <fstream>
#include <queue>
#include <memory>
#include <unordered_set>
#include "asio.hpp"
#include "tak/tak.hpp"
#include "tak/net/message.hpp"
#include "tak/ptn.hpp"
#include "tak/tps.hpp"
#include "alphabeta.hpp"

const uint8_t SIZE = 5;

using asio::ip::tcp;
using err_t = std::error_code;

struct Login { std::string username, password; };

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

using namespace tak::net;

class client : public ServerMsg::Visitor, DynamicBoard::Visitor {
public:
  client(asio::io_service& io, tcp::resolver::iterator endpoints, Login login, std::vector<std::string> whitelist) : io(io), sock(io), login(login), whitelist(whitelist), game_id(-1) {
    connect(endpoints);
  }
private:

  void seek() {
    send_msg_io(ClientMsg::seek(5, 1800));
  }

  virtual void login_prompt_msg() {
    send_msg_io(ClientMsg::login(login.username, login.password));
  }

  virtual void login_success_msg(std::string name) {
    seek();
  }

  virtual void game_start_msg(
    int id, int size, std::string player1, std::string player2, Player player
  ) {
    std::cout << "Starting game against " << (player == WHITE ? player2 : player1) << std::endl;
    my_color = player;

    game = std::unique_ptr<DynamicBoard>(new DynamicBoard(size));
    game_id = id;

    if(my_color == WHITE) {
      game->accept(*this);
    }
  }

  virtual void move_msg(int id, DynamicMove move) {
    if(id == game_id && game) {
      std::cout << "Recieved move from opponent: " << ptn::to_str(move) << std::endl;
      game->execute(move);
      game->accept(*this);
    } else {
      std::cout << "Recieved move for unknown game: " << ptn::to_str(move) << std::endl;
    }
  }

  virtual void game_over_msg(int id, GameStatus status) {
    game_done(id);
  }

  virtual void game_abandon_msg(int id) {
    game_done(id);
  }

  // Called whichever way a game ends
  void game_done(int id) {
    if(id == game_id) {
      game_id = -1;
      game.reset();
      seek();
    }
  }

  virtual void seek_new_msg(Seek seek) {
    seeks.insert(seek);
  }

  virtual void seek_remove_msg(Seek seek) {
    seeks.erase(seek);
  }

  virtual void shout_msg(std::string name, std::string msg) {
    std::regex command_rgx("^cutak_bot: (.*)");
    if(std::find(whitelist.begin(), whitelist.end(), name) != whitelist.end()) {
      std::smatch match;
      if(std::regex_search(msg, match, command_rgx)) {
        if(match[1].str() == "play") {
          auto seek = std::find_if(seeks.begin(), seeks.end(), [&](Seek s) { return s.player == name; });
          if(seek != seeks.end()) {
            send_msg_io(ClientMsg::accept(seek->id));
          }
        }
      }
    }
  }

  void send_msg(ClientMsg msg) {
    io.post([this, msg]() {
      send_msg_io(msg);
    });
  }

  void send_msg_io(ClientMsg msg) {
    bool send_in_progress = msg_queue.size() > 0;
    msg_queue.push(msg.text());
    if(!send_in_progress) {
      do_send_msg();
    }
  }

  void connect(tcp::resolver::iterator endpoints) {
    asio::async_connect(sock, endpoints, [this](err_t err, tcp::resolver::iterator) {
      if(!err) {
        std::thread([this]() {
          while(true) {
            std::cout << "Sending ping..."<< std::endl;
            send_msg(ClientMsg::ping());
            std::this_thread::sleep_for(std::chrono::seconds(5));
          }
        }).detach();
        readline();
      } else {
        std::cout << "Error connecting socket: " << err.message() << std::endl;
        std::exit(-1);
      }
    });
  }

  void readline() {
    asio::async_read_until(sock, buf, '\n', [this](err_t err, std::size_t) {
      if(!err) {
        std::istream is(&buf);
        std::string line;
        std::getline(is, line);

        // Handle incoming messages
        ServerMsg(line).handle(*this);

        readline();

      } else {
        std::cout << "Error reading socket: " << err.message() << std::endl;
        std::exit(-1);
      }
    });
  }

  void do_send_msg() {
    asio::async_write(sock, asio::buffer(msg_queue.front()), [this](err_t err, std::size_t) {
      if(!err) {
        std::cout << "Send message `" << msg_queue.front() << "' to server" << std::endl;
        msg_queue.pop();
        if(msg_queue.size()) {
          do_send_msg();
        }
      } else {
        std::cout << "Error sending msg: " << err.message() << std::endl;
        std::exit(-1);
      }
    });
  }

// I'd like to find a way to get rid of this macro...
#define VISIT(N) \
  virtual void visit(Board<N>& board) { \
    int id = game_id; \
    std::thread([this, board, id] () mutable { \
      alphabeta<N> ab(my_color); \
      Move<N> move; \
      int score = ab.search(board, move, eval_t(), max_depth); \
      std::cout << "Best move: " << ptn::to_str(move) << " with score " << score << std::endl; \
      io.post([this, move, id]() mutable { \
        if(id == game_id && game) { \
          DynamicMove m = move;\
          game->execute(m); \
          send_msg_io(ClientMsg::move(game_id, m)); \
        } \
      }); \
    }).detach(); \
  }

  VISIT(3)
  VISIT(4)
  VISIT(5)
  VISIT(6)
  VISIT(7)
  VISIT(8)

  std::unique_ptr<DynamicBoard> game;
  Player my_color;
  int game_id;
  static const int max_depth = 6;

  asio::streambuf buf;
  std::queue<std::string> msg_queue;

  // Login information
  Login login;
  std::vector<std::string> whitelist;

  // Hash Seeks on their id
  struct SeekHash { size_t operator()(const Seek& seek) { return seek.id; } };
  std::unordered_set<Seek, SeekHash> seeks;

  asio::io_service& io;
  tcp::socket sock;
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

  Login login;
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
    std::cout << line << std::endl;
  }

  io_thread.join();
}
