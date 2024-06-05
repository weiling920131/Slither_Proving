#include "clap/mcts/vl/node.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "clap/mcts/vl/engine.h"
#include "clap/mcts/vl/tree.h"

namespace clap::mcts::vl {

Node::Node()
    : num_visits(0),
      parent_player_value_sum(0.0F),
      current_player_value_sum(0.0F),
      label(2),
      boardInt(0),
      expand_state(State::UNEXPANDED) {}

std::tuple<game::Action, Node*> Node::select(std::mt19937& rng) const {
  wait_expand();

  const auto& C_PUCT = Engine::c_puct;
  const auto& EPS = Engine::float_error;

  int num_candidates = 0;
  game::Action selected_action = -1; // default action
  Node* selected_node = nullptr;
  float best_score = std::numeric_limits<float>::lowest();

  const float DEFAULT_Q = current_player_value_sum / num_visits;

  for (const auto& [p, action, child] : children) {
    if(child->label != 2) continue;
                        
    const float q = child->num_visits != 0
                        ? child->parent_player_value_sum / child->num_visits
                        : DEFAULT_Q;
    const float u =
        C_PUCT * p * std::sqrt(num_visits.load()) / (1 + child->num_visits);
    const float score = q + u;

    if (score >= best_score + EPS) {
      num_candidates = 1;
      best_score = score;
      selected_action = action;
      selected_node = child.get();
    } else if (score > best_score - EPS) {
      ++num_candidates;
      if (std::uniform_int_distribution<int>{0, num_candidates - 1}(rng) == 0) {
        selected_action = action;
        selected_node = child.get();
      }
    }
  }

  return {selected_action, selected_node};
}

void Node::expand(Tree& tree, game::State* state, const std::vector<game::Action>& legal_actions) {
  children.reserve(legal_actions.size());
  for (const auto& action : legal_actions) {
    auto cur_state = state;
    auto node = std::make_unique<Node>();
    cur_state->apply_action(action);
    node->boardInt = cur_state->convert_to_uint64_t(cur_state->getboard());
    if(tree.lookup_TT(node->boardInt)) {
      std::cout<< "Found in transposition table"<< std::endl;
      node.get()->label = 0;
    }else{
      // std::cout<< "not Found in transposition table"<< std::endl;
      node.get()->label = 2;
    }
    children.emplace_back(0.0F, action, std::move(node));
  }
  expand_done();
}

void Node::wait_expand() const {
  while (expand_state.load() == State::EXPANDING) {
  }
}

bool Node::acquire_expand() {
  auto old = State::UNEXPANDED;
  auto val = State::EXPANDING;
  return expand_state.compare_exchange_strong(old, val);
}

void Node::expand_done() { expand_state.exchange(State::EXPANDED); }

void Node::reset() {
  label = 2;
  expand_state.exchange(State::UNEXPANDED);
  num_visits = 0;
  parent_player_value_sum = 0.0F;
  current_player_value_sum = 0.0F;
  children.clear();
}

}  // namespace clap::mcts::vl
