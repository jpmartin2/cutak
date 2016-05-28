#include <iostream>
#include <thread>
#include <cstdlib>
#include <regex>
#include <fstream>
#include <sstream>
#include <queue>
#include <memory>
#include <unordered_set>
#include <cstdint>
#define ASIO_STANDALONE
#include "asio.hpp"
#include "tak/tak.hpp"
#include "tak/net/message.hpp"
#include "tak/ptn.hpp"
#include "tak/tps.hpp"
#include "alphabeta.hpp"
#include "eval.hpp"

using asio::ip::tcp;
using err_t = std::error_code;

struct Login { std::string username, password; };

using namespace tak::net;

class client : public ServerMsg::Visitor, DynamicBoard::Visitor {
public:
  client(asio::io_service& io, tcp::resolver::iterator endpoints, Login login, std::vector<std::string> whitelist) : io(io), sock(io), login(login), whitelist(whitelist), game_id(-1), max_depth(DEFAULT_MAX_DEPTH) {
    connect(endpoints);
  }
private:

  void seek() {
    send_msg_io(ClientMsg::seek(5, 1800, 0, WHITE));
  }

  virtual void error_msg(std::string msg) {
    std::cout << "ERROR: " << msg << std::endl;
  }

  virtual void nok_msg() {
    std::cout << "NOK" << std::endl;
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
    otherPlayer = player == WHITE ? player2 : player1;
    std::cout << "Starting game against " << otherPlayer << std::endl;
    my_color = player;

    game = std::unique_ptr<DynamicBoard>(new DynamicBoard(size));
    game_id = id;

    //send_msg_io(ClientMsg::shout("Good luck, "+otherPlayer+"!"));

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
      //send_msg_io(ClientMsg::shout("gg, "+otherPlayer+"!"));
      game_id = -1;
      game.reset();
      max_depth = DEFAULT_MAX_DEPTH;
      seek();
    }
  }

  virtual void seek_new_msg(Seek seek) {
    /*
    static bool played = false;
    if(!played && seek.player == "TakticianBot") {
      send_msg_io(ClientMsg::accept(seek.id));
      played = true;
    }
    */
    seeks.insert(seek);
  }

  virtual void seek_remove_msg(Seek seek) {
    seeks.erase(seek);
  }

  virtual void shout_msg(std::string name, std::string msg) {
    std::regex command_rgx("^cutak_bot: (.*)");
    std::smatch match;
    if(std::regex_search(msg, match, command_rgx)) {
      std::istringstream buffer(match[1].str());
      std::vector<std::string> words((std::istream_iterator<std::string>(buffer)),
                                      std::istream_iterator<std::string>());

      if(words.size() && words[0] == "play") {
        if(game) {
          send_msg_io(ClientMsg::shout("Sorry "+name+", I'm currently playing a game against "+otherPlayer+". Please try again once this game is over."));
        } else {
          if(words.size() == 2) {
            try {
              max_depth = std::stoi(words[1]);
            } catch(std::exception e) {
              send_msg(ClientMsg::shout("Sorry "+name+", I failed to parse the depth `"+words[1]+"'"));
            }
          } else if(words.size() > 2) {
            send_msg_io(ClientMsg::shout("Sorry "+name+", I don't understand that command."));
            send_msg_io(ClientMsg::shout("I currently support the command `cutak_bot: play <depth>'"));
            return;
          }
          play(name);
        }
      }
    }
  }

  bool authorized(std::string player) {
    return std::find(whitelist.begin(), whitelist.end(), player) != whitelist.end();
  }

  void play(std::string player) {
    auto seek = std::find_if(seeks.begin(), seeks.end(), [&](Seek s) { return s.player == player; });
    if(seek != seeks.end()) {
      send_msg_io(ClientMsg::accept(seek->id));
    } else {
      //send_msg_io(ClientMsg::shout("Sorry "+player+", I couldn't find a game of yours to join. Please create one and try again."));
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
      alphabeta<N, Eval> ab(my_color); \
      Move<N> move; \
      alphabeta<N, Eval>::Score score = ab.search(board, move, max_depth); \
      std::cout << "Best move: " << ptn::to_str(move) << " with score " << score << std::endl; \
      std::cout << "Move sequence: "; \
      std::cout << std::endl; \
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
  std::string otherPlayer;
  int game_id;
  int max_depth;
  static const int DEFAULT_MAX_DEPTH = 6;

  asio::streambuf buf;
  std::queue<std::string> msg_queue;

  // Login information
  Login login;
  std::vector<std::string> whitelist;

  // Hash Seeks on their id
  struct SeekHash { size_t operator()(const Seek& seek) const { return seek.id; } };
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
