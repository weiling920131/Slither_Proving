#pragma once

#include "clap/mcts/node.h"

namespace clap::mcts {

class Job;

class Tree {
 public:
  Tree() = default;
  int num_simulations() const;
  void add_dirichlet_noise(std::mt19937& rng);
  void reset();
  ~Tree() = default;

  Node root_node;
};

}  // namespace clap::mcts