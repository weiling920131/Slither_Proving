#include "clap/mcts/model_manager.h"

#include <torch/cuda.h>
#include <torch/script.h>

namespace clap::mcts {

ModelManager::ModelManager(const std::vector<int>& gpus, int num_versions)
    : devices(ModelManager::torch_devices(gpus)),
      toggle(num_versions),
      device_switch(num_versions),
      models(devices.size()) {
  for (auto& model_d : models) model_d.resize(num_versions);
}

void ModelManager::load(const std::string& path, int version) {
  const auto new_toggle = toggle[version] ^ 1;
  for (int d_i = 0; d_i < devices.size(); ++d_i) {
    const auto& device = devices[d_i];
    models[d_i][version][new_toggle] =
        std::make_shared<Model>(torch::jit::load(path, device));
  }
  toggle[version] ^= 1;
}

std::tuple<torch::Device, ModelManager::ModelPtr> ModelManager::get(
    int version) {
  const int device_index = (++device_switch[version]) % devices.size();
  if (device_index == 0) device_switch[version] -= devices.size();
  return {devices[device_index],
          models[device_index][version][toggle[version]]};
}

std::vector<torch::Device> ModelManager::torch_devices(
    const std::vector<int>& gpus) {
  std::vector<torch::Device> cuda_devices;
  cuda_devices.reserve(gpus.size());
  for (const auto& gpu : gpus) cuda_devices.emplace_back(torch::kCUDA, gpu);
  return cuda_devices;
}

}  // namespace clap::mcts