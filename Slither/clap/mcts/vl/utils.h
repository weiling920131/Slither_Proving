#pragma once

#include <atomic>

namespace clap::mcts::vl {

template <class T>
inline void atomic_add(std::atomic<T> &f, const T d) {
  T old = f.load();
  while (!f.compare_exchange_weak(old, old + d)) {
  }
}

}  // namespace clap::mcts::vl