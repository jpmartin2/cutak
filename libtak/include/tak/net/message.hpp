#pragma once

#include "util.hpp"
#include "tak/tak.hpp"

namespace tak {
namespace net {

struct Seek {
  const int id;
  const std::string player;
  const int size;
  const int time;
  const util::option<Player> color;

  inline Seek(int id, std::string player, int size, int time, util::option<Player> color) :
    id(id), player(player), size(size), time(time), color(color) {}
};

bool operator==(Seek left, Seek right);

class ClientMsg {
public:
  static ClientMsg client(std::string client_name);
  static ClientMsg login(std::string username, std::string password);
  static ClientMsg login_guest();
  static ClientMsg seek(int size, int time, int incr, util::option<Player> player = util::option<Player>::None);
  static ClientMsg accept(int seek_id);
  static ClientMsg move(int game_id, DynamicMove move);
  static ClientMsg offer_draw(int game_id);
  static ClientMsg cancel_draw(int game_id);
  static ClientMsg resign(int game_id);
  static ClientMsg request_undo(int game_id);
  static ClientMsg cancel_undo(int game_id);
  static ClientMsg list_seeks();
  static ClientMsg list_games();
  static ClientMsg observe(int game_id);
  static ClientMsg unobserve(int game_id);
  static ClientMsg shout(std::string msg);
  static ClientMsg ping();
  static ClientMsg quit();

  const std::string text();
private:
  ClientMsg(std::string msg);
  const std::string msg;
};

class ServerMsg {
public:
  class Visitor {
  public:
    inline virtual void welcome_msg() {}
    inline virtual void login_prompt_msg() {}
    inline virtual void login_success_msg(std::string name) {}
    inline virtual void game_add_msg(
      int id, std::string player1, std::string player2,
      int size, int time, int incr, int moves, std::string cur_player
    ) {}
    // There's more information in this message, but it seems useless
    inline virtual void game_remove_msg(int id) {}
    inline virtual void game_start_msg(int id, int size, std::string player1, std::string player2, Player player) {}
    inline virtual void move_msg(int id, DynamicMove move) {}
    inline virtual void time_msg(int id, int white_time, int black_time) {}
    inline virtual void game_over_msg(int id, GameStatus status) {}
    inline virtual void draw_offer_msg(int id) {}
    inline virtual void draw_cancel_msg(int id) {}
    inline virtual void undo_req_msg(int id) {}
    inline virtual void undo_cancel_msg(int id) {}
    inline virtual void undo_msg(int id) {}
    inline virtual void game_abandon_msg(int id) {}
    inline virtual void seek_new_msg(Seek seek) {}
    inline virtual void seek_remove_msg(Seek seek) {}
    inline virtual void observe_game_msg(
      int id, std::string player1, std::string player2,
      int size, int time, int moves, std::string cur_player
    ) {}
    inline virtual void shout_msg(std::string player, std::string msg) {}
    inline virtual void server_msg(std::string msg) {}
    inline virtual void error_msg(std::string what) {}
    inline virtual void online_msg(int num_players) {}
    inline virtual void nok_msg() {}
    inline virtual void ok_msg() {}
    // Unable to parse message
    inline virtual void unknown_msg(std::string msg) {}
  };

  ServerMsg(std::string msg);

  void handle(Visitor& v);
private:
  std::string m;
};


} // namespace net
} // namespace tak
