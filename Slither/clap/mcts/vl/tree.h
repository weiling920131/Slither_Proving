#pragma once

#ifndef TREE_H
#define TREE_H
#include "clap/mcts/vl/node.h"
#include <mutex>

namespace clap::mcts::vl {
class Node;
class Tree {
 public:
  Tree() = default;
  int num_simulations() const;
  void add_dirichlet_noise(std::mt19937& rng);
  void reset();
  ~Tree() = default;

  std::shared_ptr<Node> root_node = std::make_shared<Node>();

  std::mutex TTmutex;
  std::unordered_map<uint64_t, std::pair<int, int>> TT;
  void store_TT(uint64_t, int, int);
  bool lookup_TT(uint64_t, int);
};

}  // namespace clap::mcts::vl
#endif