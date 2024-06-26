#pragma once

#include <atomic>
#include <memory>
#include <random>
#include <tuple>
#include <vector>

#ifndef NODE_H
#define NODE_H

#include "clap/game/game.h"
#include "clap/mcts/vl/tree.h"

namespace clap::mcts::vl {
class Tree;
class Node {
 public:
  Node();
  std::tuple<game::Action, Node*> select(std::mt19937& rng) const;
  // std::tuple<game::Action, Node*> select(std::mt19937& rng, game::StatePtr& leaf_state) const;
  void expand(Tree& tree, game::State* state, const std::vector<game::Action>& legal_actions);

  void wait_expand() const;
  bool acquire_expand();
  void expand_done();

  void reset();

  ~Node() = default;

  enum class State { UNEXPANDED, EXPANDING, EXPANDED };
  std::atomic<State> expand_state;

  // 2 no win
  // 0 black win
  // 1 white win
  int label;
  bool isTT;
  uint64_t boardInt;
  
  std::atomic <int> num_visits;
  std::atomic <float> parent_player_value_sum;
  std::atomic <float> current_player_value_sum;
  // prior, action, child
  std::vector<std::tuple<float, game::Action, std::unique_ptr<Node>>> children;
};

}  // namespace clap::mcts::vl
#endif