#include <sstream>
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

ClientMsg::ClientMsg(std::string msg) : msg(msg) {}

ClientMsg ClientMsg::client(std::string client_name) {
  return ClientMsg("Client "+client_name+"\n");
}

ClientMsg ClientMsg::login(std::string username, std::string password) {
  return ClientMsg("Login "+username+" "+password+"\n");
}

ClientMsg ClientMsg::login_guest() {
  return ClientMsg("Login Guest\n");
}

ClientMsg ClientMsg::seek(int size, int time, int incr, util::option<Player> player) {
  std::string msg = "Seek "+std::to_string(size)+" "+std::to_string(time)+" "+std::to_string(incr);
  if(player) {
    msg += *player == WHITE ? " W" : " B";
  }
  msg += "\n";
  return ClientMsg(msg);
}

ClientMsg ClientMsg::accept(int seek_id) {
  return ClientMsg("Accept "+std::to_string(seek_id)+"\n");
}

ClientMsg ClientMsg::move(int game_id, DynamicMove move) {
  std::string m;
  switch(move.type()) {
  case DynamicMove::Type::MOVE: {
    char x1 = move.x() + 'A';
    char y1 = move.y() + '1';
    char x2 = x1;
    char y2 = y1;
    switch(move.dir()) {
    case DynamicMove::Dir::NORTH:
      y2 += move.range();
      break;
    case DynamicMove::Dir::SOUTH:
      y2 -= move.range();
      break;
    case DynamicMove::Dir::EAST:
      x2 += move.range();
      break;
    case DynamicMove::Dir::WEST:
      x2 -= move.range();
      break;
    }
    m = "Game#" + std::to_string(game_id) + " M " + x1 + y1 + " " + x2 + y2;
    for(int i = 1; i <= move.range(); i++) {
      m += " ";
      m += (char)('0'+move.slides(i));
    }
  }
  break;
  case DynamicMove::Type::PLACE: {
    char x1 = move.x() + 'A';
    char y1 = move.y() + '1';
    m = "Game#" + std::to_string(game_id) + " P " + x1 + y1;
    if(move.pieceType() == Piece::CAP) {
      m += " C";
    } else if(move.pieceType() == Piece::WALL) {
      m += " W";
    }
  }
  break;
  }
  m += "\n";
  return ClientMsg(m);
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

const std::string ClientMsg::text() { return msg; }

// ----------------------------------------------------- //
//                   Server Messages                     //
// ----------------------------------------------------- //

// There's probably better ways to implement this, but this is easy
std::regex welcome_rgx("^Welcome!");
std::regex login_prompt_rgx("^Login or Register");
std::regex login_succes_rgx("^Welcome ([^!]+)!");
std::regex game_add_rgx("^GameList Add Game#([0-9]+) ([^ ]+) vs ([^ ]+), ([0-9]+)x([0-9]+), ([0-9]+), ([0-9]+), ([0-9]+) half-moves played, ([^ ]+) to move");
std::regex game_remove_rgx("^GameList Remove Game#([0-9]+) ([^ ]+) vs ([^ ]+), ([0-9]+)x([0-9]+), ([0-9]+), ([0-9]+), ([0-9]+) half-moves played, ([^ ]+) to move");
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

ServerMsg::ServerMsg(std::string msg) : m(msg) {}

void ServerMsg::handle(ServerMsg::Visitor& handler) {
  std::smatch match;
  if(std::regex_search(m, match, welcome_rgx)) {
    handler.welcome_msg();
  } else if(std::regex_search(m, match, login_prompt_rgx)) {
    handler.login_prompt_msg();
  } else if(std::regex_search(m, match, login_succes_rgx)) {
    handler.login_success_msg(match[1].str());
  } else if(std::regex_search(m, match, game_add_rgx)) {
    // Verify that the board is a square size
    if(std::stoi(match[4].str()) != std::stoi(match[5].str())) {
      handler.unknown_msg(m);
    } else {
      handler.game_add_msg(
        std::stoi(match[1].str()), match[2].str(), match[3].str(),
        std::stoi(match[4].str()), std::stoi(match[6].str()), std::stoi(match[7].str()),
        std::stoi(match[8].str()), match[9].str()
      );
    }
  } else if(std::regex_search(m, match, game_remove_rgx)) {
    handler.game_remove_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, game_start_rgx)) {
    Player player = match[5].str() == "white" ? WHITE : BLACK;
    handler.game_start_msg(std::stoi(match[1].str()), std::stoi(match[2].str()), match[3].str(), match[4].str(), player);
  } else if(std::regex_search(m, match, place_rgx)) {
    Piece type = Piece::FLAT;
    if(match[3] == "C") type = Piece::CAP;
    else if(match[3] == "W") type = Piece::WALL;
    uint8_t x = match[2].str()[0]-'A';
    uint8_t y = match[2].str()[1]-'1';
    DynamicMove move(x, y, type);
    handler.move_msg(std::stoi(match[1].str()), move);
  } else if(std::regex_search(m, match, move_rgx)) {
    int x1 = match[2].str()[0]-'A';
    int y1 = match[2].str()[1]-'1';
    int x2 = match[3].str()[0]-'A';
    int y2 = match[3].str()[1]-'1';
    int range = 0;
    DynamicMove::Dir dir;
    if(x1 == x2) {
      range = y2 - y1;
      dir = DynamicMove::Dir::NORTH;
      if(range < 0) {
        range = -range;
        dir = DynamicMove::Dir::SOUTH;
      }
    } else if(y1 == y2) {
      range = x2 - x1;
      dir = DynamicMove::Dir::EAST;
      if(range < 0) {
        range = -range;
        dir = DynamicMove::Dir::WEST;
      }
    }
    std::istringstream slides_str(match[4].str());
    std::string slide;
    std::vector<uint8_t> slides;
    int i = 0;
    while(std::getline(slides_str, slide, ' ')) {
      if(slide.size()) {
        if(i >= sizeof(slides)) break;
        slides.push_back(std::stoi(slide));
      }
    }
    DynamicMove move(x1, y1, dir, range, slides);
    handler.move_msg(std::stoi(match[1].str()), move);
  } else if(std::regex_search(m, match, time_rgx)) {
    handler.time_msg(std::stoi(match[1].str()), std::stoi(match[2].str()), std::stoi(match[3].str()));
  } else if(std::regex_search(m, match, game_over_rgx)) {
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
      status.winner = WHITE;
    } else if(match[2].str() == "0-R" || match[2].str() == "0-F") {
      status.winner = BLACK;
    } else {
      status.winner = NEITHER;
    }

    handler.game_over_msg(std::stoi(match[1].str()), status);
  } else if(std::regex_search(m, match, draw_offer_rgx)) {
    handler.draw_offer_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, draw_cancel_rgx)) {
    handler.draw_cancel_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, undo_req_rgx)) {
    handler.undo_req_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, undo_cancel_rgx)) {
    handler.undo_cancel_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, undo_rgx)) {
    handler.undo_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, game_abandon_rgx)) {
    handler.game_abandon_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, seek_new_rgx)) {
    util::option<Player> player;
    if(match[5].str() == "W") {
      player = WHITE;
    } else if(match[5].str() == "B") {
      player = BLACK;
    }

    Seek seek(
      std::stoi(match[1].str()), match[2].str(),
      std::stoi(match[3].str()), std::stoi(match[4].str()), player
    );
    handler.seek_new_msg(seek);
  } else if(std::regex_search(m, match, seek_rem_rgx)) {
    util::option<Player> player;
    if(match[5].str() == "W") {
      player = WHITE;
    } else if(match[5].str() == "B") {
      player = BLACK;
    }

    Seek seek(
      std::stoi(match[1].str()), match[2].str(),
      std::stoi(match[3].str()), std::stoi(match[4].str()), player
    );
    handler.seek_remove_msg(seek);
  } else if(std::regex_search(m, match, observe_game_rgx)) {
    // Assert that board is square
    if(std::stoi(match[4].str()) != std::stoi(match[5].str())) {
      handler.unknown_msg(m);
    } else {
      handler.observe_game_msg(
        std::stoi(match[1].str()), match[2].str(), match[3].str(),
        std::stoi(match[4].str()), std::stoi(match[6].str()),
        std::stoi(match[7].str()), match[8].str()
      );
    }
  } else if(std::regex_search(m, match, shout_rgx)) {
    handler.shout_msg(match[1].str(), match[2].str());
  } else if(std::regex_search(m, match, server_rgx)) {
    handler.server_msg(match[1].str());
  } else if(std::regex_search(m, match, error_rgx)) {
    handler.error_msg(match[1].str());
  } else if(std::regex_search(m, match, online_rgx)) {
    handler.online_msg(std::stoi(match[1].str()));
  } else if(std::regex_search(m, match, nok_rgx)) {
    handler.nok_msg();
  } else if(std::regex_search(m, match, ok_rgx)) {
    handler.ok_msg();
  } else {
    handler.unknown_msg(m);
  }
}

}
}
