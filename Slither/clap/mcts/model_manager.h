#pragma once

#include <torch/script.h>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include "clap/game/game.h"

namespace clap::mcts {

class ModelManager {
 public:
  using Model = torch::jit::script::Module;
  using ModelPtr = std::shared_ptr<Model>;

  ModelManager(const std::vector<int>& gpus, int num_versions = 1);
  void load(const std::string& path, int version = 0);
  std::tuple<torch::Device, ModelPtr> get(int version = 0);
  ~ModelManager() = default;

 private:
  static std::vector<torch::Device> torch_devices(const std::vector<int>& gpus);

  // inference devices
  const std::vector<torch::Device> devices;
  // toggle[version] 0 <-> 1
  std::vector<std::atomic_int> toggle;
  // device_switch[version]
  std::vector<std::atomic_int> device_switch;
  // models[device][version][toggle]
  std::vector<std::vector<std::array<ModelPtr, 2>>> models;
};

}  // namespace clap::mcts
