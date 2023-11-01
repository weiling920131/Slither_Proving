#include "clap/game/game.h"

#include <algorithm>
#include <sstream>

namespace clap::game {

int Game::num_transformations() const { return 0; }

std::vector<float> Game::transform_observation(
    const std::vector<float>& observation, int type = 0) const {
  return std::vector<float>(observation);
}

std::vector<float> Game::transform_policy(const std::vector<float>& policy,
                                          int type = 0) const {
  return std::vector<float>(policy);
}

std::vector<float> Game::restore_policy(const std::vector<float>& policy,
                                        int type = 0) const {
  return std::vector<float>(policy);
}

Factory& Factory::instance() {
  static Factory impl;
  return impl;
}

void Factory::add(const CreateFunc& create_func) {
  factory_[create_func()->name()] = create_func;
}

std::vector<std::string> Factory::games() const {
  std::vector<std::string> names;
  names.reserve(factory_.size());
  std::transform(factory_.begin(), factory_.end(), std::back_inserter(names),
                 [](auto p) { return p.first; });
  return names;
}

GamePtr Factory::create(const std::string& name) { return factory_[name](); }

std::vector<std::string> list() { return Factory::instance().games(); }
GamePtr load(const std::string& name) {
  return Factory::instance().create(name);
}

}  // namespace clap::game