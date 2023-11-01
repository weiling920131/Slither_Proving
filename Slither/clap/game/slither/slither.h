#pragma once
#include <array>
#include <map>

#include "clap/game/game.h"

namespace clap::game::slither {

constexpr int kBoardSize = 5;
constexpr int kNumOfGrids = kBoardSize * kBoardSize;
constexpr int kPolicyDim = kNumOfGrids + 1;
constexpr int empty_index = kNumOfGrids; // 25 in 5 * 5 board

/** Maximum number of steps */
constexpr int kMaxTurn = INT16_MAX;
/** Maximum number of repetitions */
//constexpr int kMaxRepeatBoard = 2;
/** Maximum number of consecutive steps without capture */
//constexpr int kMaxNoCaptureTurn = 50;

class SlitherState final : public State {
 public:
  SlitherState(GamePtr);
  SlitherState(const SlitherState &) = default;
  StatePtr clone() const override;
  void apply_action(const Action &) override;
  std::vector<Action> legal_actions() const override;
  bool is_terminal() const override;

  Player current_player() const override;
  Player get_winner() const override;
  std::vector<float> returns() const override;
  std::vector<float> observation_tensor() const override;
  std::string to_string() const override;

  std::string serialize() const override;
  std::string serialize_num_to_char() const override;

  //bool has_piece(const Player &) const;

 private:
  std::vector<int> get_restrictions(const Action src, const Action action, const Player player, std::array<short, kNumOfGrids>* bptr = nullptr) const;
  bool is_moving_valid(const Action src, const Action action, const Player player, std::array<short, kNumOfGrids>* bptr = nullptr) const;
  bool is_selecting_valid(const Action action, const Player player) const;
  bool is_placing_valid(const Action action, const Player player, std::array<short, kNumOfGrids>* bptr = nullptr) const;
  bool is_placing_valid(const Player player, std::array<short, kNumOfGrids>* bptr = nullptr) const;
  bool is_legal_action(const Action &action) const;
  //bool is_legal_capture_this_circle(const Action &action, const Action &src,
  //                                  const std::array<int, 56> &circle) const;
  bool have_move(const Action &action) const;
  bool have_win(const Action &action) const;
  bool is_valid(const Action &action) const;
  bool is_empty_valid(const Action &action) const;
  //bool have_capture(const Action &action) const;
  //bool have_capture_this_circle(const Action &action,
  //                              const std::array<int, 56> &circle) const;
  /** 0 -> B (black)
   * 1 -> W (white)
   * 2 -> E (empty)*/
  enum Piece { BLACK, WHITE, EMPTY };
  /** index
   * 0  1  2  3  4  
   * 5  6  7  8  9
   * 10 11 12 13 14
   * 15 16 17 18 19
   * 20 21 22 23 24*/
  std::array<short, kNumOfGrids> board_;
  /** Game rule for terminal: the board_ repeat 3 times */
  //std::map<std::array<short, kNumOfGrids>, short> repeat_;
  std::vector<Action> history_;
  int turn_;
  int skip_;
  //int without_capture_turn_;
  //int black_piece_;
  //int white_piece_;

  Player winner_;
};

class SlitherGame final : public Game {
 public:
  std::string name() const override;
  int num_players() const override;
  int num_distinct_actions() const override;
  StatePtr new_initial_state() const override;
  std::vector<int> observation_tensor_shape() const override;

  int num_transformations() const override;
  std::vector<float> transform_observation(
      const std::vector<float> &observation, int) const override;
  std::vector<float> transform_policy(const std::vector<float> &policy,
                                      int) const override;
  std::vector<float> restore_policy(const std::vector<float> &policy,
                                    int) const override;

  std::string action_to_string(const Action &action) const override;
  std::vector<Action> string_to_action(const std::string &str) const override;
  StatePtr deserialize_state(const std::string &) const override;

  int getBoardSize() const override; 
  bool save_manual(const std::vector<Action> actions, const std::string savepath) const override;

 private:
  int transform_index(const int &index, int) const;
};

// *IMPORTANT* Register this game to the factory
namespace {
Registration<SlitherGame> registration;
}

constexpr std::array<int, 8> MoveDirection = {-kBoardSize-1, -kBoardSize, -kBoardSize+1, -1, +1, +kBoardSize-1, +kBoardSize, +kBoardSize+1}; //ul, u, ur, l, r, bl, b, br
}  // namespace clap::game::slither
