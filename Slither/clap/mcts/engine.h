#pragma once

#include <memory>
#include <thread>
#include <vector>

#include "clap/game/game.h"
#include "clap/mcts/job.h"
#include "clap/mcts/model_manager.h"
#include "third_party/concurrentqueue/blockingconcurrentqueue.h"

namespace clap::mcts {

class Engine {
 public:
  Engine(const std::vector<int>& gpus, int models = 1);

  void load_game(const std::string& name);
  void load_model(const std::string& path, int version = 0);
  void start(int cpu_workers, int gpu_workers, int num_envs = -1);
  std::string get_trajectory();
  void stop();

  void add_job(int num = 1, const std::string& serialize_string = "");

  void cpu_worker(uint32_t seed);
  void gpu_worker(uint32_t seed);

  ~Engine() = default;

  bool running;

  const std::vector<int> gpus;

  static int batch_size;
  static int max_simulations;

  static float c_puct;
  static float dirichlet_alpha;
  static float dirichlet_epsilon;
  static float float_error;

  static int num_sampled_transformations;
  static int batch_per_job;

  static int temperature_drop;
  static int inference_wait_ms;

  static bool play_until_terminal;
  static bool auto_reset_job;
  static bool play_until_turn_player;
  static bool mcts_until_turn_player;
  static bool verbose;
  static int top_n_childs;
  static bool states_value_using_childs;
  static bool dump_tree;
  static bool save_observation;
  static bool report_serialize_string;

  game::GamePtr game;
  ModelManager model_manager;
  int num_models;

  std::vector<std::thread> cpu_threads;
  std::vector<std::thread> gpu_threads;

  moodycamel::BlockingConcurrentQueue<std::unique_ptr<Job>> cpu_jobs;
  std::vector<moodycamel::BlockingConcurrentQueue<std::unique_ptr<Job>>>
      gpu_jobs;
  moodycamel::BlockingConcurrentQueue<std::string> trajectories;
};

}  // namespace clap::mcts
