#pragma once

#include <array>

#include "clap/game/game.h"

namespace clap::game::gomoku {

const int BOARD_SIZE = 9;
const int NUM_GRIDS = BOARD_SIZE * BOARD_SIZE;
const int CONNECT = 5;

class GomokuState final : public State {
 public:
  GomokuState(GamePtr);
  GomokuState(const GomokuState&) = default;
  StatePtr clone() const override;
  void apply_action(const Action&) override;
  std::vector<Action> legal_actions() const override;
  bool is_terminal() const override;

  Player current_player() const override;
  std::vector<float> returns() const override;
  std::vector<float> observation_tensor() const override;
  std::string to_string() const override;

  bool has_line(const Player&, const Action&) const;

  std::string serialize() const override;

 private:
  // 0 -> ●
  // 1 -> ○
  // 2 -> empty
  std::array<short, NUM_GRIDS> board_;
  std::vector<Action> history_;
  int turn_;
  Player winner_;
};

class GomokuGame final : public Game {
 public:
  std::string name() const override;
  int num_players() const override;
  int num_distinct_actions() const override;
  StatePtr new_initial_state() const override;
  std::vector<int> observation_tensor_shape() const override;

  StatePtr deserialize_state(const std::string& str = "") const override;
};

// *IMPORTANT* Register this game to the factory
namespace {
Registration<GomokuGame> registration;
}

}  // namespace clap::game::gomoku
