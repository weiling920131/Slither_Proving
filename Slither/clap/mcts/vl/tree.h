#pragma once

#include "clap/mcts/vl/node.h"

namespace clap::mcts::vl {

class Tree {
 public:
  Tree() = default;
  int num_simulations() const;
  void add_dirichlet_noise(std::mt19937& rng);
  void reset();
  ~Tree() = default;

  std::shared_ptr<Node> root_node = std::make_shared<Node>();
};

}  // namespace clap::mcts::vl