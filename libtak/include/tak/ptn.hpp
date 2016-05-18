#pragma once

#include "game.hpp"
#include "move.hpp"
#include <regex>
#include <string>
#include <algorithm>

namespace ptn {

template<uint8_t SIZE>
std::string to_str(typename Move<SIZE>::Dir dir) {
  switch(dir) {
  case Move<SIZE>::Dir::NORTH: return "+";
  case Move<SIZE>::Dir::SOUTH: return "-";
  case Move<SIZE>::Dir::EAST: return ">";
  case Move<SIZE>::Dir::WEST: return "<";
  default: return "?";
  }
}

inline std::string to_str(Piece piece) {
  switch(piece) {
  case Piece::FLAT: return "F";
  case Piece::WALL: return "S";
  case Piece::CAP: return "C";
  default: return "?";
  }
}

template<uint8_t SIZE>
std::string to_str(Move<SIZE> move) {
  std::string m;
  switch(move.type()) {
  case Move<SIZE>::Type::MOVE: {
    int x = 0;
    for(int i = 1; i <= move.range(); i++) { 
      x += move.slides(i);
    }
    if(x > 1) {
      m.push_back('0'+x);
    }
    m.push_back('a'+move.idx()%SIZE);
    m.push_back('1'+move.idx()/SIZE);
    m += to_str<SIZE>(move.dir());
    if(move.range() != 1) {
      for(int i = 1; i <= move.range(); i++) { 
        m.push_back('0'+move.slides(i));
      }
    }
    return m;
  }
  case Move<SIZE>::Type::PLACE:
    m += to_str(move.pieceType());
    m.push_back('a'+move.idx()%SIZE);
    m.push_back('1'+move.idx()/SIZE);
    return m;
  default:
    return "?";
  }
}

template<uint8_t SIZE>
bool from_str(std::string ptn, Move<SIZE>& move) {
  std::transform(ptn.begin(), ptn.end(), ptn.begin(), ::tolower);
  std::regex place_rgx("([fsc]?)([a-f][1-8])");
  std::regex move_rgx("([1-8]?)([a-f][1-8])([-+<>])([1-8]*)");
  std::smatch match;

  if(std::regex_match(ptn, match, place_rgx)) {
    Piece type = Piece::FLAT;
    if(match[1].str().size()) {
      switch(match[1].str()[0]) {
      case 'f': type = Piece::FLAT; break;
      case 's': type = Piece::WALL; break;
      case 'c': type = Piece::CAP; break;
      }
    }

    uint8_t file = match[2].str()[0] - 'a';
    uint8_t rank = match[2].str()[1] - '1';
    if(file > SIZE-1 || rank > SIZE-1) return false;
    uint8_t idx = file+rank*SIZE;
    move = Move<SIZE>(idx, type);

    return true;
  } else if(std::regex_match(ptn, match, move_rgx)) {
    typedef typename Move<SIZE>::Dir Dir;
    uint8_t num_pieces = 1;
    if(match[1].str().size()) {
      num_pieces = match[1].str()[0]-'0';
    }

    uint8_t file = match[2].str()[0] - 'a';
    uint8_t rank = match[2].str()[1] - '1';
    if(file > SIZE-1 || rank > SIZE-1) return false;
    uint8_t idx = file+rank*SIZE;

    Dir dir;
    switch(match[3].str()[0]) {
    case '+': dir = Dir::NORTH; break;
    case '-': dir = Dir::SOUTH; break;
    case '>': dir = Dir::EAST; break;
    case '<': dir = Dir::WEST; break;
    default: return false;
    }

    uint8_t slides[SIZE-1] = { 0 };
    uint8_t slides_sum = 0;
    uint8_t range = match[4].str().size();
    if(range > SIZE-1) return false;
    for(int i = 0; i < range; i++) {
      slides[i] = match[4].str()[i]-'0';
      slides_sum += slides[i];
    }

    if(range == 0) {
      range = 1;
      slides[0] = num_pieces;
      slides_sum = num_pieces;
    }

    if(slides_sum != num_pieces) return false;

    move = Move<SIZE>(idx, dir, range, slides);

    return true;
  }

  return false;
}

} // namespace ptn
