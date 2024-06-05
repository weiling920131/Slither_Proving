#pragma once

#include <random>
#include <tuple>
#include <vector>
#include <unordered_map>


#include "clap/game/game.h"
#include "clap/mcts/vl/tree.h"
#include "clap/pb/clap.pb.h"

namespace clap::mcts::vl {



class Node;
class Engine;

class Job {
 public:
  enum Step { SELECT, EVALUATE, UPDATE, PLAY, REPORT, DONE };

  Job(Engine* engine, const std::string& serialize_string = "");
  void select(std::mt19937& rng);
  void evaluate();
  void update(std::mt19937& rng);
  void play(std::mt19937& rng);
  void report();
  void PreOrderTraversalDump(std::ofstream& sgf_file_, const Node& current_node,
                             const game::StatePtr& current_state,
                             const float& prior, const int& parent_num_visits,
                             const game::Action& last_action) const;
  void print_mcts_results(const int top_n) const;
  void dump_mcts() const;

  Engine* engine;
  Step next_step;

  // select
  game::StatePtr leaf_state;
  std::vector<float> leaf_observation;
  // previous player, current player, node ptr
  std::vector<std::tuple<game::Player, game::Player, Node*, game::Action>> selection_path;
  // evaluate
  std::vector<float> leaf_policy;
  std::vector<float> leaf_returns;
  // update
  Tree tree;
  bool tree_owner;
  // play
  game::StatePtr root_state;
  // report
  clap::pb::Trajectory trajectory;
};

}  // namespace clap::mcts::vl
