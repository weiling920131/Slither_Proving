#include "clap/mcts/job.h"

#include "clap/mcts/engine.h"
#include "clap/mcts/node.h"
#include "clap/mcts/tree.h"

namespace clap::mcts {

Job::Job(Engine* engine, const std::string& serialize_string)
    : engine(engine),
      next_step(Step::SELECT),
      root_state(engine->game->new_initial_state()) {
  if (serialize_string != "") {
    root_state = engine->game->deserialize_state(serialize_string);
  }
}

void Job::select(std::mt19937& rng) {
  auto* leaf_node = &tree.root_node;
  leaf_state = root_state->clone();

  game::Player previous_player = leaf_state->current_player();

  selection_path.clear();
  selection_path.emplace_back(previous_player, previous_player, leaf_node);

  game::Action action;
  while (!leaf_node->children.empty()) {
    if(previous_player == 0 && leaf_state->current_player() == 1) {
      if(!leaf_state->check_can_block()){
        leaf_node->label = 0; // black win
      };
    }

    std::tie(action, leaf_node) = leaf_node->select(rng);
    leaf_state->apply_action(action);
    game::Player current_player = leaf_state->current_player();
    selection_path.emplace_back(previous_player, current_player, leaf_node);
    previous_player = current_player;
  }

  if (leaf_state->is_terminal()) {
    leaf_policy.clear();
    leaf_returns = leaf_state->returns();
    next_step = Step::UPDATE;
  } else {
    leaf_observation = leaf_state->observation_tensor();
    next_step = Step::EVALUATE;
  }
}

void Job::evaluate() {
  const auto& observation_tensor_shape =
      engine->game->observation_tensor_shape();
  std::vector<int64_t> input_shape(observation_tensor_shape.begin(),
                                   observation_tensor_shape.end());
  // add batch dimension (CHW -> BCHW)
  input_shape.insert(input_shape.begin(), 1);

  // calculate input shape & size
  const int input_size = std::accumulate(input_shape.begin(), input_shape.end(),
                                         1, std::multiplies<>());

  // construct input tensor
  std::vector<float> input_vector;
  input_vector.reserve(input_size);
  input_vector.insert(input_vector.end(),
                      std::make_move_iterator(leaf_observation.begin()),
                      std::make_move_iterator(leaf_observation.end()));
  auto [device, model_ptr] = engine->model_manager.get();
  auto input_tensor =
      torch::from_blob(input_vector.data(), input_shape).to(device);

  // inference
  auto results = model_ptr->forward({input_tensor}).toTuple()->elements();
  auto batch_policy = results[0].toTensor().cpu();
  auto batch_value = results[1].toTensor().cpu();

  const auto& policy_ptr = batch_policy[0].data_ptr<float>();
  const auto& policy_size = batch_policy[0].numel();
  leaf_policy.assign(policy_ptr, policy_ptr + policy_size);

  const auto& value_ptr = batch_value[0].data_ptr<float>();
  const auto& value_size = batch_value[0].numel();
  leaf_returns.assign(value_ptr, value_ptr + value_size);

  next_step = Job::Step::UPDATE;
}

void Job::update(std::mt19937& rng) {
  for (const auto& [parent_player, current_player, node] : selection_path) {
    ++node->num_visits;
    node->parent_player_value_sum += leaf_returns[parent_player];
    node->current_player_value_sum += leaf_returns[current_player];
  }

  if (!leaf_policy.empty()) {
    auto& [parent_player, current_player, leaf_node] = selection_path.back();
    const auto legal_actions = leaf_state->legal_actions();
    leaf_node->expand(legal_actions);
    // extract legal action policy and normalize
    float policy_sum = 0.0F;
    for (auto& [p, action, child] : leaf_node->children) {
      p = leaf_policy[action];
      policy_sum += p;
    }
    for (auto& [p, action, child] : leaf_node->children) p /= policy_sum;
    // first simulation -> add dirichlet noise to root policy
    if (tree.num_simulations() == 1) tree.add_dirichlet_noise(rng);
  }

  if (tree.root_node.num_visits >= Engine::max_simulations) {
    next_step = Step::PLAY;
  } else {
    next_step = Step::SELECT;
  }
}

void Job::play(std::mt19937& rng) {
  const clap::game::Player player = root_state->current_player();
  clap::mcts::Node* parent_node_ptr = &tree.root_node;

//  if (Engine::dump_tree) {
//    dump_mcts();
//  }

  while (player == root_state->current_player() && !root_state->is_terminal()) {
    if (Engine::verbose) {
      print_mcts_results(Engine::top_n_childs, *parent_node_ptr);
    }

    // extract the policy of legal actions
    std::vector<float> legal_policy;
    legal_policy.reserve(parent_node_ptr->children.size());
    // calculate mcts policy with visit count
    std::vector<float> mcts_policy(engine->game->num_distinct_actions(), 0.0F);
    for (const auto& [p, action, child] : parent_node_ptr->children) {
      mcts_policy[action] = static_cast<float>(child.num_visits) /
                            (parent_node_ptr->num_visits - 1);
      legal_policy.push_back(mcts_policy[action]);
    }

    const bool greedy = trajectory.states_size() >= Engine::temperature_drop;
    const int action_index =
        greedy ? std::max_element(legal_policy.begin(), legal_policy.end()) -
                     legal_policy.begin()
               : std::discrete_distribution<int>{legal_policy.begin(),
                                                 legal_policy.end()}(rng);
    const auto chosen_action =
        std::get<1>(parent_node_ptr->children[action_index]);

    // save root state into trajectory
    auto* pb_state = trajectory.add_states();
    auto* pb_state_evaluation = pb_state->mutable_evaluation();
    auto* pb_state_transition = pb_state->mutable_transition();
    if (Engine::save_observation) {
      for (auto observation : root_state->observation_tensor())
        pb_state->add_observation_tensor(observation);
    }
    pb_state_transition->set_action(chosen_action);
    if (Engine::states_value_using_childs) {
      const clap::mcts::Node& child_node =
          std::get<2>(parent_node_ptr->children[action_index]);
      pb_state_evaluation->set_value(child_node.parent_player_value_sum /
                                     child_node.num_visits);
    } else {
      pb_state_evaluation->set_value(parent_node_ptr->current_player_value_sum /
                                     parent_node_ptr->num_visits);
    }
    for (const auto& p : mcts_policy) pb_state_evaluation->add_policy(p);

    // apply action to root state
    root_state->apply_action(chosen_action);
    parent_node_ptr = &std::get<2>(parent_node_ptr->children[action_index]);

    if (parent_node_ptr->num_visits <= 1 || !Engine::play_until_turn_player) {
      break;
    }
  }

  bool search_again = (Engine::mcts_until_turn_player &&
                       root_state->current_player() == player);
  if (!Engine::play_until_terminal && !search_again) {
    next_step = Step::REPORT;
  } else if (root_state->is_terminal()) {
    auto* pb_statistics = trajectory.mutable_statistics();
    for (const auto& value : root_state->returns())
      pb_statistics->add_rewards(value);
    next_step = Step::REPORT;
  } else {
    tree.reset();
    next_step = Step::SELECT;
  }
}

void Job::report() {
  if (Engine::report_serialize_string) {
    trajectory.set_appendix(root_state->serialize());
  }
  std::string raw;
  trajectory.SerializeToString(&raw);
  engine->trajectories.enqueue(std::move(raw));

  // reset the job
  if (Engine::auto_reset_job) {
    next_step = Step::SELECT;
    tree.reset();
    root_state = engine->game->new_initial_state();
    trajectory.Clear();
  } else {
    next_step = Step::DONE;
  }
}

}  // namespace clap::mcts
