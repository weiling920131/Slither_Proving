#pragma once

#include <array>

#include "clap/game/game.h"

namespace clap::game::connect6 {

const int BOARD_SIZE = 19;
const int NUM_GRIDS = BOARD_SIZE * BOARD_SIZE;
const int CONNECT = 6;

class Connect6State final : public State {
 public:
  Connect6State(GamePtr);
  Connect6State(const Connect6State&) = default;
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
  // add by Wei-Yuan Hsu
  void modify(const std::vector<Action>);

 private:
  // 0 -> ●
  // 1 -> ○
  // 2 -> empty
  std::array<short, NUM_GRIDS> board_;
  int turn_;
  Player winner_;

  // add by Wei-Yuan Hsu
  int turn_to_ignore_stones_;
  std::vector<bool> ignored_stones_table;
  std::vector<std::vector<bool> > ignored_pairs_table;
  std::vector<Action> history_;
};

class Connect6Game final : public Game {
 public:
  std::string name() const override;
  int num_players() const override;
  int num_distinct_actions() const override;
  StatePtr new_initial_state() const override;
  std::vector<int> observation_tensor_shape() const override;
  int num_transformations() const override;
  std::vector<float> transform_observation(const std::vector<float>&,
                                           int) const override;
  std::vector<float> transform_policy(const std::vector<float>&,
                                      int) const override;
  std::vector<float> restore_policy(const std::vector<float>&,
                                    int) const override;

  StatePtr deserialize_state(const std::string& str = "") const override;

 private:
  void transform(float*, int) const;
  void rotate_right(float*) const;
  void rotate_left(float*) const;
  void reverse(float*) const;
  void transpose(float*) const;
  void reflect_horizontal(float*) const;
  void reflect_vertical(float*) const;
};

// *IMPORTANT* Register this game to the factory
namespace {
Registration<Connect6Game> registration;
}

}  // namespace clap::game::connect6
