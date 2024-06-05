#include "clap/mcts/vl/tree.h"

#include "clap/mcts/vl/engine.h"
#include "clap/mcts/vl/node.h"

namespace clap::mcts::vl {

int Tree::num_simulations() const { return root_node->num_visits; }

void Tree::add_dirichlet_noise(std::mt19937& rng) {
  auto& children = root_node->children;
  const auto& ALPHA = Engine::dirichlet_alpha;
  const auto& EPSILON = Engine::dirichlet_epsilon;
  std::gamma_distribution<float> gamma(ALPHA, 1.0F);

  float noise_sum = 0.0F;
  std::vector<float> noise(children.size());
  for (int i = 0; i < noise.size(); ++i) {
    noise[i] = gamma(rng);
    noise_sum += noise[i];
  }

  if (noise_sum < std::numeric_limits<float>::min()) return;

  for (int i = 0; i < children.size(); ++i) {
    noise[i] /= noise_sum;
    auto& [p, action, child] = children[i];
    p = p * (1.0F - EPSILON) + noise[i] * EPSILON;
  }
}

void Tree::reset() { 
  root_node->reset(); 
  TT.clear();
}

void Tree::store_TT(uint64_t board, int label){
    TT[board] = {label};
	return;
}

bool Tree::lookup_TT(uint64_t board){
    auto it = TT.find(board);
    if (it != TT.end()) {
        return true;
    } else {
        return false;
    }
}


}  // namespace clap::mcts::vl