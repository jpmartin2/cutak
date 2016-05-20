#include <regex>
#include "tak/net/message.hpp"

namespace tak {
namespace net {

bool operator==(Seek left, Seek right) {
  return left.id == right.id;
}

// ----------------------------------------------------- //
//                   Client Messages                     //
// ----------------------------------------------------- //

ClientMsg ClientMsg::client(std::string client_name) {
  return ClientMsg("Client "+client_name+"\n");
}

ClientMsg ClientMsg::login(std::string username, std::string password) {
  return ClientMsg("Login "+username+" "+password+"\n");
}

ClientMsg ClientMsg::login_guest() {
  return ClientMsg("Login Guest\n");
}

ClientMsg ClientMsg::seek(int size, int time, option<Color> color = util::option<Color>::None) {
  std::string msg = "Seek "+std::to_string(size)+" "+std::to_string(time);
  if(color) {
    msg += *color == WHITE ? " W" : " B";
  }
  msg += "\n";
  return ClientMsg(msg);
}

ClientMsg ClientMsg::accept(int seek_id) {
  return ClientMsg("Accept "+std::to_string(seek_id)+"\n");
}

ClientMsg ClientMsg::move(int game_id, DynamicMove move) {
}

ClientMsg ClientMsg::offer_draw(int game_id) {
  return ClientMsg("Game#"+std::to_string(game_id)+" OfferDraw\n");
}

ClientMsg ClientMsg::cancel_draw(int game_id) {
  return ClientMsg("Game#"+std::to_string(game_id)+" RemoveDraw\n");
}

ClientMsg ClientMsg::resign(int game_id) {
  return ClientMsg("Game#"+std::to_string(game_id)+" Resign\n");
}

ClientMsg ClientMsg::request_undo(int game_id) {
  return ClientMsg("Game#"+std::to_string(game_id)+" RequestUndo\n");
}

ClientMsg ClientMsg::cancel_undo(int game_id) {
  return ClientMsg("Game#"+std::to_string(game_id)+" RemoveUndo\n");
}

ClientMsg ClientMsg::list_seeks() {
  return ClientMsg("List\n");
}

ClientMsg ClientMsg::list_games() {
  return ClientMsg("GameList\n");
}

ClientMsg ClientMsg::observe(int game_id) {
  return ClientMsg("Observe "+std::to_string(game_id)+"\n");
}

ClientMsg ClientMsg::unobserve(int game_id) {
  return ClientMsg("Unobserve "+std::to_string(game_id)+"\n");
}

ClientMsg ClientMsg::shout(std::string msg) {
  return ClientMsg("Shout "+msg+"\n");
}

ClientMsg ClientMsg::ping() {
  return ClientMsg("PING\n");
}

ClientMsg ClientMsg::quit() {
  return ClientMsg("quit\n");
}

// ----------------------------------------------------- //
//                   Server Messages                     //
// ----------------------------------------------------- //

// There's probably better ways to implement this, but this is easy
std::regex welcome_rgx("^Welcome!")
std::regex login_prompt_rgx("^Login or Register");
std::regex login_succes_rgx("^Welcome ([^!]+)!");
std::regex game_add_rgx("^GameList Add Game#([0-9]+) ([^ ]+) vs ([^ ]+), ([0-9]+)x([0-9]+), ([0-9]+), ([0-9]+) half-moves played, ([^ ]+) to move");
std::regex game_remove_rgx("^GameList Remove Game#([0-9]+) ([^ ]+) vs ([^ ]+), ([0-9]+)x([0-9]+), ([0-9]+), ([0-9]+) half-moves played, ([^ ]+) to move");
std::regex game_start_rgx("^Game Start ([0-9]+) ([3-8]) ([^ ]+) vs ([^ ]+) (white|black)");
std::regex place_rgx("^Game#([0-9]+) P ([A-F][1-8])(?: ([CW]))?");
std::regex move_rgx("^Game#([0-9]+) M ([A-F][1-8]) ([A-F][1-8])((?: [1-8])+)");
std::regex time_rgx("^Game#([0-9]+) Time ([0-9]+) ([0-9]+)");
std::regex game_over_rgx("^Game#([0-9]+) Over (.*)");
std::regex draw_offer_rgx("^Game#([0-9]+) OfferDraw");
std::regex draw_cancel_rgx("^Game#([0-9]+) RemoveDraw");
std::regex undo_req_rgx("^Game#([0-9]+) RequestUndo");
std::regex undo_cancel_rgx("^Game#([0-9]+) RemoveUndo");
std::regex undo_rgx("^Game#([0-9]+) Undo");
std::regex game_abandon_rgx("^Game#([0-9]+) Abandoned");
std::regex seek_new_rgx("^Seek new ([0-9]+) ([^ ]+) ([3-8]) ([0-9]+)(?: ([WB]))?");
std::regex seek_rem_rgx("^Seek remove ([0-9]+) ([^ ]+) ([3-8]) ([0-9]+)(?: ([WB]))?");
std::regex observe_game_rgx("^Observe Game#([0-9]+) ([3-8]) ([^ ]+) vs ([^ ]+), ([0-9]+)x([0-9]+), ([0-9]+), ([0-9]+) half-moves played, ([^ ]+) to move");
std::regex shout_rgx("^Shout <([^>]+)> (.*)");
std::regex server_rgx("^Message (.*)");
std::regex error_rgx("^Error (.*)");
std::regex online_rgx("^Online ([0-9]+)");
std::regex nok_rgx("^NOK");
std::regex ok_rgx("^OK");

ServerMsg::ServerMsg(std::string msg) {
}

void ServerMsg::Visitor::visit(ServerMsg msg) {
  std::smatch match;
  if(std::regex_search(msg.m, match, welcome_rgx)) {
    welcome_msg();
  } else if(std::regex_search(msg.m, match, login_prompt_rgx)) {
    login_prompt_msg();
  } else if(std::regex_search(msg.m, match, login_succes_rgx)) {
    login_success_msg(match[1].str());
  } else if(std::regex_search(msg.m, match, game_add_rgx)) {
    // Verify that the board is a square size
    if(std::stoi(match[4].str()) != std::stoi(match[5].str())) {
      unknown_msg();
    } else {
      game_add_msg(
        std::stoi(match[1].str()), match[2].str(), match[3].str(),
        std::stoi(match[4].str()), std::stoi(match[6].str()),
        std::stoi(match[7].str()), match[8].str()
      );
    }
  } else if(std::regex_search(msg.m, match, game_remove_rgx)) {
    game_remove_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, game_start_rgx)) {
    Color color = match[4].str() == "white" ? WHITE : BLACK;
    game_start_msg(std::stoi(match[1].str()), match[2].str(), match[3].str(), color);
  } else if(std::regex_search(msg.m, match, place_rgx)) {
    // Todo: extract move
    move_msg(std::stoi(match[1].str(), move));
  } else if(std::regex_search(msg.m, match, move_rgx)) {
    // Todo: extract move
    move_msg(std::stoi(match[1].str(), move));
  } else if(std::regex_search(msg.m, match, time_rgx)) {
    time_msg(std::stoi(match[1].str()), std::stoi(match[2].str()), std::stoi(match[3].str()));
  } else if(std::regex_search(msg.m, match, game_over_rgx)) {
    GameStatus status;
    status.over = true;

    if(match[2].str() == "R-0" || match[2].str() == "0-R") {
      status.condition = ROAD_VICTORY;
    } else if(match[2].str() == "F-0" || match[2].str() == "0-F") {
      status.condition = FLAT_VICTORY;
    } else {
      status.condition = FLAT_VICTORY;
    }

    if(match[2].str() == "R-0" || match[2].str() == "F-0") {
      status.winer = WHITE;
    } else if(match[2].str() == "0-R" || match[2].str() == "0-F") {
      status.winner = BLACK;
    } else {
      status.winner = NEITHER;
    }

    game_over_msg(std::stoi(match[1].str()), status);
  } else if(std::regex_search(msg.m, match, draw_offer_rgx)) {
    draw_offer_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, draw_cancel_rgx)) {
    draw_cancel_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, undo_req_rgx)) {
    undo_req_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, undo_cancel_rgx)) {
    undo_cancel_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, undo_rgx)) {
    undo_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, game_abandon_rgx)) {
    game_abandon_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, seek_new_rgx)) {
    util::option<Color> color;
    if(match[5].str() == "W") {
      color = WHITE;
    } else if(match[5].str() == "B") {
      color = BLACK;
    }

    Seek seek(
      std::stoi(match[1].str()), match[2].str(),
      std::stoi(match[3].str()), std::stoi(match[4]).str(), color
    );
    seek_new_msg(seek);
  } else if(std::regex_search(msg.m, match, seek_rem_rgx)) {
    util::option<Color> color;
    if(match[5].str() == "W") {
      color = WHITE;
    } else if(match[5].str() == "B") {
      color = BLACK;
    }

    Seek seek(
      std::stoi(match[1].str()), match[2].str(),
      std::stoi(match[3].str()), std::stoi(match[4]).str(), color
    );
    seek_remove_msg(seek);
  } else if(std::regex_search(msg.m, match, observe_game_rgx)) {
    // Assert that board is square
    if(std::stoi(match[4].str()) != std::stoi(match[5].str())) {
      unknown_msg();
    } else {
      observe_game_msg(
        std::stoi(match[1].str()), match[2].str(), match[3].str(),
        std::stoi(match[4].str()), std::stoi(match[6].str()),
        std::stoi(match[7].str()), match[8].str()
      );
    }
  } else if(std::regex_search(msg.m, match, shout_rgx)) {
    shout_msg(match[1].str(), match[2].str());
  } else if(std::regex_search(msg.m, match, server_rgx)) {
    server_msg(match[1].str());
  } else if(std::regex_search(msg.m, match, error_rgx)) {
    error_msg(match[1].str());
  } else if(std::regex_search(msg.m, match, onmsg.m_rgx)) {
    online_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(msg.m, match, nok_rgx)) {
    nok_msg();
  } else if(std::regex_search(msg.m, match, ok_rgx)) {
    ok_msg();
  } else {
    unknown_msg(msg.m);
  }
}

}
}
