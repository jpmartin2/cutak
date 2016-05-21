#include "eval.hpp"

std::mt19937 Eval::generator;
std::uniform_int_distribution<int> Eval::distribution = std::uniform_int_distribution<int>(-500,500);
