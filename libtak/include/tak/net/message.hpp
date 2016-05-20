#pragma once

namespace tak {
namespace net {

struct Seek {
  const int id;
  const std::string player;
  const int size;
  const int time;
  const option<Color> color;
};

bool operator==(Seek left, Seek right);

class ClientMsg {
public:
  static ClientMsg client(std::string client_name);
  static ClientMsg login(std::string username, std::string password);
  static ClientMsg login_guest();
  static ClientMsg seek(int size, int time, option<Color> color = option<Color>::None);
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
  private:
    friend class ServerMsg;
    void visit(ServerMsg& m);
  public:
    inline virtual void welcome_msg() {}
    inline virtual void login_promt_msg() {}
    inline virtual void login_success_msg(std::string name) {}
    inline virtual void game_add_msg(
      int id, std::string player1, std::string player2,
      int size, int time, int moves, std::string cur_player
    ) {}
    // There's more information in this message, but it seems useless
    inline virtual void game_remove_msg(int id) {}
    inline virtual void game_start_msg(int id, std::string player1, std::string player2, util::option<Color> color) {}
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

  void accept(visitor& v) {
    v.visit(*this);
  }
private:
  std::string m;
};


} // namespace net
} // namespace tak
