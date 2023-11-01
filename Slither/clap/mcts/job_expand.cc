#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>

#include "clap/game/game.h"
#include "clap/mcts/engine.h"
#include "clap/mcts/job.h"
#include "clap/mcts/node.h"
#include "clap/mcts/tree.h"

namespace clap::mcts {

void Job::print_mcts_results(const int top_n, const Node& parent_node) const {
  std::vector<int> children_index;
  for (int i = 0; i < parent_node.children.size(); i++) {
    children_index.push_back(i);
  }
  std::sort(children_index.begin(), children_index.end(),
            [&parent_node](int a, int b) {
              return std::get<2>(parent_node.children[a]).num_visits >
                     std::get<2>(parent_node.children[b]).num_visits;
            });
  int rank = std::min(top_n, (int)parent_node.children.size());
  std::cerr << std::right << std::setw(6) << "ID" << std::setw(10) << "Action"
            << std::setw(14) << "Prior Policy" << std::setw(12) << "MCTS Count"
            << std::setw(13) << "MCTS Policy" << std::setw(20)
            << "MCTS Winrate[-1~1]" << std::endl;
  for (int i = 0; i < rank; i++) {
    std::cerr
        << std::right << std::fixed << std::setprecision(6) << std::setw(6)
        << std::get<1>(parent_node.children[children_index[i]]) << std::setw(10)
        << engine->game->action_to_string(
               std::get<1>(parent_node.children[children_index[i]]))
        << std::setw(14) << std::get<0>(parent_node.children[children_index[i]])
        << std::setw(12)
        << std::get<2>(parent_node.children[children_index[i]]).num_visits
        << std::setw(13)
        << (double)std::get<2>(parent_node.children[children_index[i]])
                   .num_visits /
               (parent_node.num_visits - 1)
        << std::setw(20)
        << std::get<2>(parent_node.children[children_index[i]])
                   .parent_player_value_sum /
               std::get<2>(parent_node.children[children_index[i]]).num_visits
        << std::endl;
  }
}

const std::vector<std::string> color{"B", "W", "UNKNOWN"};

void Job::PreOrderTraversalDump(std::ofstream& sgf_file_,
                                const Node& current_node,
                                const game::StatePtr& parent_state,
                                const float& prior,
                                const int& parent_num_visits,
                                const game::Action& last_action) const {
  game::StatePtr current_state = parent_state->clone();
  if (&current_node == &tree.root_node) {
    sgf_file_ << ";GM[CLAP]";
    // GM[CLAP]
    // sgf_file_ << parent_state->serialize();
    sgf_file_ << parent_state->serialize_num_to_char();
    // ;B[KJ];W[KI];W[LJ];B[MK];B[ML];W[LK];W[LI];B[MI];B[MJ];W[MH];W[NH];B[MM];B[MN]
  } else {
    sgf_file_ << color[parent_state->current_player()] << "["
              << engine->game->action_to_string(last_action) << "]";
    sgf_file_ << "\r\nC[Simulation Count: " << current_node.num_visits << "\r\n";
    sgf_file_ << "MCTS Value: "
              << current_node.parent_player_value_sum / current_node.num_visits
              << "\r\n";
    sgf_file_ << "Prior Probability: " << prior << "\r\n";
    sgf_file_ << "MCTS Policy: "
              << (float)current_node.num_visits / (parent_num_visits - 1)
              << "\r\n]";
    current_state->apply_action(last_action);
  }
  bool hasBranch = 0;
  for(auto child: current_node.children){
    if (std::get<2>(child).num_visits != 0){
      hasBranch = 1;
      break;
    }
  }
  for (auto child : current_node.children) {
    if (std::get<2>(child).num_visits != 0) {
      float prior = std::get<0>(child);
      game::Action last_action = std::get<1>(child);
      if (hasBranch){
        sgf_file_<<"(";
      }
      PreOrderTraversalDump(sgf_file_, std::get<2>(child), current_state, prior,
                            current_node.num_visits, last_action);
      if (hasBranch){
        sgf_file_<<")";
      }
              
    }
  }

}

void Job::dump_mcts() const {
  const game::GamePtr& game = engine->game;
  std::ofstream sgf_file_;
  std::time_t now = std::time(0);
  std::tm* ltm = localtime(&now);
  std::stringstream dt;
  std::string folder="MCTS_sgf/";
  system(("mkdir -p "+folder).c_str());
  dt << 1900 + ltm->tm_year << 'Y' << 1 + ltm->tm_mon << 'M' << ltm->tm_mday
     << 'D' << ltm->tm_hour << ':' << ltm->tm_min << ':' << ltm->tm_sec;
  std::string filename = folder + "MCTS_" + dt.str() + ".sgf";
  sgf_file_.open(filename);
  PreOrderTraversalDump(sgf_file_, tree.root_node, root_state, 1.0,
                        tree.root_node.num_visits, 0);
  sgf_file_.close();
  system(("./parser "+filename).c_str());
  // std::cerr << game->name() << std::endl;
  // (;GM[7]FF[4];B[KJ];W[KI];W[LJ];B[MK];B[ML];W[LK];W[LI];B[MI];B[MJ];W[MH];W[NH];B[MM];B[MN])
  // (;GM[511];B[JJ](;W[JH];W[MK])(;W[JH];W[JN])(;W[JH];W[FK];B[FF])(;W[JH];W[PO](;B[NH];B[PJ];W[MM];W[KL])(;B[NH];B[MK])))
}
}  // namespace clap::mcts
