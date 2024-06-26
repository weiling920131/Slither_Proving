#pragma once

#include <random>
#include <tuple>
#include <vector>

#include "clap/game/game.h"

namespace clap::mcts {

class Node {
 public:
  Node();
  std::tuple<game::Action, Node*> select(std::mt19937& rng) const;
  void expand(const std::vector<game::Action>& legal_actions);
  void reset();

  ~Node() = default;

  int label;
  // 2 no win
  // 0 black win
  // 1 white win

  int num_visits;
  float parent_player_value_sum;
  float current_player_value_sum;
  // prior, action, child
  std::vector<std::tuple<float, game::Action, Node>> children;
};

}  // namespace clap::mcts
