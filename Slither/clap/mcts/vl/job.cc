#include "clap/mcts/vl/job.h"

#include "clap/mcts/vl/engine.h"
#include "clap/mcts/vl/node.h"
#include "clap/mcts/vl/tree.h"
#include "clap/mcts/vl/utils.h"

namespace clap::mcts::vl {

Job::Job(Engine* engine, const std::string& serialize_string)
    : engine(engine),
      next_step(Step::SELECT),
      tree_owner(true),
      root_state(engine->game->new_initial_state()) {
  if (serialize_string != "") {
    root_state = engine->game->deserialize_state(serialize_string);
  }
}

void Job::select(std::mt19937& rng) {
  // //std::cout<<"select start\n";
  auto leaf_node = tree.root_node.get();
  leaf_state = root_state->clone();
  
  game::Player previous_player = leaf_state->current_player();
  selection_path.clear();
  selection_path.emplace_back(1-previous_player, previous_player, leaf_node, -1);

  game::Action action;
  //std::cout<<"before select while\n";
  while (true) {
    std::cout<<".";
    leaf_node->num_visits += Engine::virtual_loss;

    if(tree.lookup_TT(leaf_node->boardInt)) {
      leaf_node->label = 0;
      leaf_policy.clear();
      leaf_returns = leaf_state->returns();
      next_step = Step::UPDATE;
      break;
    }

    if (leaf_node->children.empty()) {
      if (leaf_state->is_terminal()) {
        leaf_node->label = leaf_state->get_winner();
        leaf_policy.clear();
        leaf_returns = leaf_state->returns();
        next_step = Step::UPDATE;
        break;
      }
      auto success = leaf_node->acquire_expand();
      if (success) {
        leaf_observation = leaf_state->observation_tensor();
        next_step = Step::EVALUATE;
        break;
      }
    }
    auto tmp = leaf_node;
    // std::tie(action, leaf_node) = leaf_node->select(rng);
    std::tie(action, leaf_node) = leaf_node->select(rng);
    if (action == -1) {
        if(tmp == tree.root_node.get()) {
          std::cout<<"root: "<<tmp->label<<'\n';
        }
        std::cout<<"test1\n";
        leaf_policy.clear();
        leaf_returns = leaf_state->returns();
        if(tree_owner) {
          next_step = Step::PLAY;
        }else {
          next_step = Step::DONE;
        }
        std::cout<<"test2\n";
        break;
    }

    
    leaf_state->apply_action(action);
    //std::cout<<"job.cc line: 106\n";
    game::Player current_player = leaf_state->current_player();

    selection_path.emplace_back(previous_player, current_player, leaf_node, action);
    previous_player = current_player;
  }
  //std::cout<<"select end\n";
  
}

void Job::evaluate() {
  std::cout << "evaluate\n";
  // std::cout<<"job.cc line: 76\n";
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
  // std::cout<<"job update\n";
  for (auto& [parent_player, current_player, node, act] : selection_path) {
    node->num_visits -= Engine::virtual_loss - 1;
    atomic_add(node->parent_player_value_sum, leaf_returns[parent_player]);
    atomic_add(node->current_player_value_sum, leaf_returns[current_player]);
  }
  auto& [parent_player, current_player, leaf_node, act] = selection_path.back();
  if((parent_player == 0) && (current_player == 1)) {
    // std::cout<< "before check can block\n";
    if(!leaf_state->check_can_block()) {
      // std::cout<< "after check can block\n";
      // 
      leaf_node->label = 0; // black win
      tree.store_TT(leaf_node->boardInt, leaf_node->label);
    }
  }
  if (!leaf_policy.empty() && leaf_node->label == 2) {
    auto& [parent_player, current_player, leaf_node, act] = selection_path.back();
    const auto legal_actions = leaf_state->legal_actions();
    leaf_node->expand(tree, leaf_state.get(), legal_actions);
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

  for (auto& [p, action, child] : leaf_node->children) {
    if (child->label != 2) {
      auto pre_label = child->label;
      int i = selection_path.size() - 1;
      
      if (i >= 0) {
        auto& [p_player, c_player, node, act] = selection_path[i];
        if (node->label != 2) continue;

        bool needLabel = true;
        if((pre_label == 0) && (p_player == 0) && (c_player == 0)){ // black choose, black move
          node->label = pre_label;
          if(!tree.lookup_TT(node->boardInt)) {
            tree.store_TT(node->boardInt, pre_label);
          }
        }
        else if((pre_label == 1) && (p_player == 1) && (c_player == 1)){ // white choose, white move
          node->label = pre_label;
          if(!tree.lookup_TT(node->boardInt)) {
            tree.store_TT(node->boardInt, pre_label);
          }
        }
        else if((pre_label == 0) && (p_player == 1) && (c_player == 0)){ // label = 0, OR node (white place)
          node->label = pre_label;
          if(!tree.lookup_TT(node->boardInt)) {
            tree.store_TT(node->boardInt, pre_label);
          }
        }
        else if((pre_label == 1) && (p_player == 0) && (c_player == 1)){ // label = 1, AND node (black place)
          node->label = pre_label;
          if(!tree.lookup_TT(node->boardInt)) {
            tree.store_TT(node->boardInt, pre_label);
          }
        }
        else{
          bool needLabel = true;
          for(auto& [p, action, child] : node->children){
            if(child->label != pre_label){
              needLabel = false;
              break;
            }
          }
          if(needLabel){
            node->label = pre_label;
            if(!tree.lookup_TT(node->boardInt)) {
              tree.store_TT(node->boardInt, pre_label);
            }
          }
        }
      }
      
    }
  }

  auto pre_label = leaf_node->label;
  int i = selection_path.size() - 2;

  while((i >= 0) && (pre_label != 2)) {
    auto& [p_player, c_player, node, act] = selection_path[i];

    bool needLabel = true;
    if((pre_label == 0) && (p_player == 0) && (c_player == 0)){ // black choose, black move
      node->label = pre_label;
      if(!tree.lookup_TT(node->boardInt)) {
        tree.store_TT(node->boardInt, pre_label);
      }
    }
    else if((pre_label == 1) && (p_player == 1) && (c_player == 1)){ // white choose, white move
      node->label = pre_label;
      if(!tree.lookup_TT(node->boardInt)) {
        tree.store_TT(node->boardInt, pre_label);
      }
    }
    else if((pre_label == 0) && (p_player == 1) && (c_player == 0)){ // label = 0, OR node (white place)
      node->label = pre_label;
      if(!tree.lookup_TT(node->boardInt)) {
        tree.store_TT(node->boardInt, pre_label);
      }
    }
    else if((pre_label == 1) && (p_player == 0) && (c_player == 1)){ // label = 1, AND node (black place)
      node->label = pre_label;
      if(!tree.lookup_TT(node->boardInt)) {
        tree.store_TT(node->boardInt, pre_label);
      }
    }
    else{
      bool needLabel = true;
      for(auto& [p, action, child] : node->children){
        if(child->label != pre_label){
          needLabel = false;
          break;
        }
      }
      if(needLabel){
        node->label = pre_label;
        if(!tree.lookup_TT(node->boardInt)) {
          tree.store_TT(node->boardInt, pre_label);
        }
        //std::cout<<"成功update\n";
      }
    }
    // if (needLabel) {
    //   std::cout << leaf_state->printBoard({}, {}) << '\n';
    // }
    pre_label = node->label;
    i--;
  }
  
  // std::cout << "Update done\n";
  // if (tree.root_node->num_visits >= Engine::max_simulations) {
  //   if (tree_owner) {
  //     next_step = Step::PLAY;
  //   } else {
  //     next_step = Step::DONE;
  //   }
  // } else {
    next_step = Step::SELECT;
    std::cout<< tree.root_node->num_visits << '\n';
    if (tree.root_node->num_visits > 15000000) {
      next_step = Step::DONE;
    }
    // std::cout << "update\n";

}

void Job::play(std::mt19937& rng) {
  // std::cerr<<"test"<<std::endl;
  while (tree.root_node.use_count() != 1) {
    // break;
    // std::cout<<"play\n";
    // //std::cout<<"use_count: "<<tree.root_node.use_count()<<'\n';
  }
  const auto player = root_state->current_player();
  const auto root_node = tree.root_node.get();

  if (Engine::verbose) {
    print_mcts_results(Engine::top_n_childs);
  }
  if (Engine::dump_tree) {
//    //std::cout<<"dump tree"<<std::endl;
    dump_mcts();
  }
//  else
//  {
//    //std::cout<<"no dump tree"<<std::endl;

//  }
  

  // extract the policy of legal actions
  std::vector<float> legal_policy;
  legal_policy.reserve(root_node->children.size());
  // calculate mcts policy with visit count
  std::vector<float> mcts_policy(engine->game->num_distinct_actions(), 0.0F);
  for (const auto& [p, action, child] : root_node->children) {
    mcts_policy[action] =
        static_cast<float>(child->num_visits) / (root_node->num_visits - 1);
    legal_policy.push_back(mcts_policy[action]);
  }

  const bool greedy = trajectory.states_size() >= Engine::temperature_drop;
  const int action_index =
      greedy ? std::max_element(legal_policy.begin(), legal_policy.end()) -
                   legal_policy.begin()
             : std::discrete_distribution<int>{legal_policy.begin(),
                                               legal_policy.end()}(rng);
  const auto chosen_action = std::get<1>(root_node->children[action_index]);

  // save root state into trajectory
  auto* pb_state = trajectory.add_states();
  auto* pb_state_evaluation = pb_state->mutable_evaluation();
  auto* pb_state_transition = pb_state->mutable_transition();
  pb_state_transition->set_action(chosen_action);
  if (Engine::states_value_using_childs) {
    const auto child_node =
        std::get<2>(root_node->children[action_index]).get();
    pb_state_evaluation->set_value(child_node->parent_player_value_sum /
                                   child_node->num_visits);
  } else {
    pb_state_evaluation->set_value(root_node->current_player_value_sum /
                                   root_node->num_visits);
  }
  for (const auto& p : mcts_policy) pb_state_evaluation->add_policy(p);

  // apply action to root state
  root_state->apply_action(chosen_action);

  // play until turn player
  auto parent_node = std::get<2>(root_node->children[action_index]).get();
  while (Engine::play_until_turn_player &&
         root_state->current_player() == player && !root_state->is_terminal()) {
    // no children for this node
    // std::cout<<"while\n";
    if (parent_node->num_visits - 1 == 0) break;

    // reset policy
    legal_policy.clear();
    legal_policy.reserve(parent_node->children.size());
    std::fill(mcts_policy.begin(), mcts_policy.end(), 0.0F);

    for (const auto& [p, action, child] : parent_node->children) {
      mcts_policy[action] =
          static_cast<float>(child->num_visits) / (parent_node->num_visits - 1);
      legal_policy.push_back(mcts_policy[action]);
    }

    const int action_index =
        std::max_element(legal_policy.begin(), legal_policy.end()) -
        legal_policy.begin();
    const clap::game::Action chosen_action =
        std::get<1>(parent_node->children[action_index]);

    // save root state into trajectory
    auto* pb_state = trajectory.add_states();
    auto* pb_state_evaluation = pb_state->mutable_evaluation();
    auto* pb_state_transition = pb_state->mutable_transition();
    pb_state_transition->set_action(chosen_action);
    if (Engine::states_value_using_childs) {
      const auto child_node =
          std::get<2>(parent_node->children[action_index]).get();
      pb_state_evaluation->set_value(child_node->parent_player_value_sum /
                                     child_node->num_visits);
    } else {
      pb_state_evaluation->set_value(parent_node->current_player_value_sum /
                                     parent_node->num_visits);
    }
    for (const auto& p : mcts_policy) pb_state_evaluation->add_policy(p);

    // apply action to root state
    root_state->apply_action(chosen_action);
    parent_node = std::get<2>(parent_node->children[action_index]).get();
  }

  bool still_search = (Engine::mcts_until_turn_player &&
                       root_state->current_player() == player);
  if (!Engine::play_until_terminal && !still_search) {
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

}  // namespace clap::mcts::vl
