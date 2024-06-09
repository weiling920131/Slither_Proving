#include "clap/mcts/vl/engine.h"

#include <torch/torch.h>

#include <algorithm>
#include <random>

namespace clap::mcts::vl {

int Engine::batch_size;
int Engine::max_simulations;

float Engine::c_puct;
float Engine::dirichlet_alpha;
float Engine::dirichlet_epsilon;
float Engine::float_error = 1e-3;

int Engine::temperature_drop = std::numeric_limits<int>::max();
int Engine::num_sampled_transformations = 0;
int Engine::batch_per_job = 1;
int Engine::inference_wait_ms = 1;
int Engine::virtual_loss = 3;

bool Engine::play_until_terminal = true;
bool Engine::auto_reset_job = true;
bool Engine::play_until_turn_player = false;
bool Engine::mcts_until_turn_player = false;
bool Engine::verbose = false;
int Engine::top_n_childs = 10;
bool Engine::states_value_using_childs = false;
bool Engine::dump_tree = true;

Engine::Engine(const std::vector<int>& gpus, int models)
    : running(false),
      gpus(gpus),
      model_manager(gpus, models),
      num_models(models) {
  gpu_jobs.clear();
  gpu_jobs.reserve(models);
  for (int i = 0; i < models; i++) {
    gpu_jobs.push_back(
        moodycamel::BlockingConcurrentQueue<std::unique_ptr<Job>>());
  }
}

void Engine::load_game(const std::string& name) {
  game = clap::game::load(name);
}

void Engine::load_model(const std::string& path, int version) {
  model_manager.load(path, version);
}

void Engine::start(int cpu_workers, int gpu_workers, int num_envs) {
  if (running) return;
  running = true;

  std::random_device rd;
  cpu_threads.reserve(cpu_workers);
  for (int i = 0; i < cpu_workers; ++i) {
    auto seed = rd();
    cpu_threads.emplace_back(&Engine::cpu_worker, this, seed);
  }

  gpu_threads.reserve(gpu_workers);
  for (int i = 0; i < gpu_workers; ++i) {
    auto seed = rd();
    gpu_threads.emplace_back(&Engine::gpu_worker, this, seed);
  }

  if (num_envs < 0) num_envs = 2 * Engine::batch_size * gpu_workers;
  for (int i = 0; i < num_envs; ++i)
    cpu_jobs.enqueue(std::make_unique<Job>(this));
}

void Engine::stop() {
  if (!running) return;
  running = false;

  for (const auto& thread : cpu_threads) cpu_jobs.enqueue(nullptr);

  for (auto& thread : cpu_threads) thread.join();
  for (auto& thread : gpu_threads) thread.join();

  // enqueue a empty string to unblock Engine::get_trajectory
  trajectories.enqueue("");
}

void Engine::add_job(int num, const std::string& serialize_string) {
  if (num < 1) return;
  auto first_job = std::make_unique<Job>(this, serialize_string);
  auto root = first_job->tree.root_node;
  cpu_jobs.enqueue(std::move(first_job));
  for (int i = 1; i < num; ++i) {
    auto job = std::make_unique<Job>(this, serialize_string);
    job->tree.root_node = root;
    job->tree_owner = false;
    cpu_jobs.enqueue(std::move(job));
  }
}

std::string Engine::get_trajectory() {
  std::string raw;
  trajectories.wait_dequeue(raw);
  return raw;
}

void Engine::cpu_worker(uint32_t seed) {
  std::mt19937 rng{seed};
  while (running) {
    std::unique_ptr<Job> job;
    cpu_jobs.wait_dequeue(job);
    if (job == nullptr) return;
    // std::cout<<job->tree.root_node.use_count()<<std::endl;

    while (true) {
      switch (job->next_step) {
        case Job::Step::SELECT:
          job->select(rng);
          break;
        case Job::Step::UPDATE:
          job->update(rng);
          break;
        case Job::Step::PLAY:
          job->play(rng);
          break;
        case Job::Step::REPORT:
          job->report();
          break;
        default:
          goto done;
      }
    }

  done:
    if (job->next_step == Job::Step::EVALUATE) {
      gpu_jobs[job->root_state->current_player() % num_models].enqueue(
          std::move(job));
    }
  }
}

void Engine::gpu_worker(uint32_t seed) {
  std::mt19937 rng{seed};
  const auto wait_time = std::chrono::milliseconds(Engine::inference_wait_ms);

  const auto& observation_tensor_shape = game->observation_tensor_shape();
  std::vector<int64_t> input_shape(observation_tensor_shape.begin(),
                                   observation_tensor_shape.end());
  // add batch dimension (CHW -> BCHW)
  input_shape.insert(input_shape.begin(), 1);

  // create a vector containing all transformations
  int num_transformations = game->num_transformations();
  std::vector<int> transformations;
  transformations.reserve(num_transformations);
  for (int i = 0; i < num_transformations; ++i) transformations.push_back(i);

  int gpu_job_toggle = 0;
  while (running) {
    // collect jobs
    std::chrono::steady_clock::time_point deadline;
    std::chrono::system_clock::duration timeout = wait_time;
    std::vector<std::unique_ptr<Job>> jobs;
    jobs.reserve(Engine::batch_size);

    while (jobs.size() < jobs.capacity() &&
           timeout > std::chrono::steady_clock::duration::zero() && running) {
      const int count = gpu_jobs[gpu_job_toggle].wait_dequeue_bulk_timed(
          std::back_inserter(jobs), jobs.capacity() - jobs.size(), timeout);
      // first dequeue or didn't get any job
      if (count == jobs.size())
        deadline = std::chrono::steady_clock::now() + wait_time;
      if (jobs.empty()) gpu_job_toggle = (gpu_job_toggle + 1) % num_models;
      timeout = deadline - std::chrono::steady_clock::now();
    }
    if (jobs.empty()) continue;

    // calculate input shape & size
    const int batch_size = jobs.size() * Engine::batch_per_job;
    input_shape[0] = batch_size;
    const int input_size = std::accumulate(
        input_shape.begin(), input_shape.end(), 1, std::multiplies<>());

    std::vector<float> input_vector;
    input_vector.reserve(input_size);
    std::vector<std::vector<int>> batch_transformations;
    batch_transformations.reserve(jobs.size());

    // construct input tensor, record sampled transformations for each job
    for (int i = 0; i < jobs.size(); ++i) {
      std::vector<int> sampled_transformations;
      if (Engine::num_sampled_transformations == 0) {
        sampled_transformations.push_back(0);
      } else {
        std::sample(transformations.begin(), transformations.end(),
                    std::back_inserter(sampled_transformations),
                    Engine::num_sampled_transformations, rng);
      }
      batch_transformations.push_back(sampled_transformations);

      for (int type : sampled_transformations) {
        std::vector<float> transformed_observation =
            game->transform_observation(jobs[i]->leaf_observation, type);
        input_vector.insert(
            input_vector.end(),
            std::make_move_iterator(transformed_observation.begin()),
            std::make_move_iterator(transformed_observation.end()));
      }
    }
    auto [device, model_ptr] = model_manager.get(gpu_job_toggle);
    auto input_tensor =
        torch::from_blob(input_vector.data(), input_shape).to(device);

    // inference
    const auto results =
        model_ptr->forward({input_tensor}).toTuple()->elements();
    const auto batch_policy = results[0].toTensor().cpu().contiguous();
    const auto batch_value = results[1].toTensor().cpu().contiguous();

    const auto policy_size = batch_policy[0].numel();
    const auto value_size = batch_value[0].numel();

    auto policy_ptr_begin = batch_policy.data_ptr<float>();
    auto value_ptr_begin = batch_value.data_ptr<float>();

    std::vector<float> average_policy(policy_size);
    std::vector<float> average_value(value_size);
    // copy results back to jobs
    for (int i = 0; i < jobs.size(); ++i) {
      std::fill(average_policy.begin(), average_policy.end(), 0);
      std::fill(average_value.begin(), average_value.end(), 0);

      for (int type : batch_transformations[i]) {
        const auto policy_ptr_end = policy_ptr_begin + policy_size;
        const auto value_ptr_end = value_ptr_begin + value_size;

        std::vector<float> policy(policy_ptr_begin, policy_ptr_end);
        std::vector<float> restored_policy = game->restore_policy(policy, type);

        // sum all policies and values
        std::transform(restored_policy.begin(), restored_policy.end(),
                       average_policy.begin(), average_policy.begin(),
                       std::plus<float>());
        std::transform(value_ptr_begin, value_ptr_end, average_value.begin(),
                       average_value.begin(), std::plus<float>());

        policy_ptr_begin = std::move(policy_ptr_end);
        value_ptr_begin = std::move(value_ptr_end);
      }

      for (auto& p : average_policy) p /= Engine::batch_per_job;
      for (auto& v : average_value) v /= Engine::batch_per_job;
      jobs[i]->leaf_policy.assign(average_policy.begin(), average_policy.end());
      jobs[i]->leaf_returns.assign(average_value.begin(), average_value.end());
      jobs[i]->next_step = Job::Step::UPDATE;
    }

    cpu_jobs.enqueue_bulk(std::make_move_iterator(jobs.begin()), jobs.size());
    gpu_job_toggle = (gpu_job_toggle + 1) % num_models;
  }
}

}  // namespace clap::mcts::vl
