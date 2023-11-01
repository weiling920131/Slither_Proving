#include "clap/game/gomoku/gomoku.h"

#include <iomanip>
#include <numeric>
#include <sstream>

namespace clap::game::gomoku {

GomokuState::GomokuState(GamePtr game_ptr)
    : State(std::move(game_ptr)), turn_(0), winner_(-1) {
  board_.fill(2);
}

StatePtr GomokuState::clone() const {
  return std::make_unique<GomokuState>(*this);
}

void GomokuState::apply_action(const Action& action) {
  Player player = current_player();
  board_[action] = player;
  ++turn_;
  if (has_line(player, action)) {
    winner_ = player;
  }
  history_.push_back(action);
}

std::vector<Action> GomokuState::legal_actions() const {
  std::vector<Action> actions;
  actions.reserve(NUM_GRIDS - turn_);
  for (int i = 0; i < NUM_GRIDS; ++i)
    if (board_[i] == 2) actions.push_back(i);
  return actions;
}

std::string GomokuState::to_string() const {
  std::stringstream ss;
  const std::vector<std::string> chess{"●", "○", "·"};

  for (int i = 0; i < BOARD_SIZE; ++i) {
    ss << std::setw(2) << std::setfill(' ') << BOARD_SIZE - i;
    for (int j = 0; j < BOARD_SIZE; ++j)
      ss << ' ' << chess[board_[i * BOARD_SIZE + j]];
    ss << std::endl;
  }
  ss << "  ";
  for (int i = 0; i < BOARD_SIZE; ++i) ss << ' ' << static_cast<char>('A' + i);
  return ss.str();
}

bool GomokuState::is_terminal() const {
  return turn_ == NUM_GRIDS || winner_ != -1;
}

Player GomokuState::current_player() const { return turn_ % 2; }

std::vector<float> GomokuState::observation_tensor() const {
  std::vector<float> tensor;
  auto shape = game()->observation_tensor_shape();
  auto size =
      std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
  tensor.reserve(size);

  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == 0));
  }
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == 1));
  }
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == 2));
  }
  Player player = current_player();
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(player));
  }

  return tensor;
}

bool GomokuState::has_line(const Player& player, const Action& action) const {
  int ai = action / BOARD_SIZE, aj = action % BOARD_SIZE;
  // right, down-right, down, down-left
  int vi[] = {0, 1, 1, 1};
  int vj[] = {1, 1, 0, -1};

  for (int d = 0; d < 4; ++d) {
    int count = 0;
    for (int ci = ai, cj = aj;
         ci >= 0 && ci < BOARD_SIZE && cj >= 0 && cj < BOARD_SIZE &&
         board_[ci * BOARD_SIZE + cj] == player;
         ci -= vi[d], cj -= vj[d])
      ++count;
    for (int ci = ai, cj = aj;
         ci >= 0 && ci < BOARD_SIZE && cj >= 0 && cj < BOARD_SIZE &&
         board_[ci * BOARD_SIZE + cj] == player;
         ci += vi[d], cj += vj[d])
      ++count;
    // +1 since board_[action] will be counted twice
    if (count >= CONNECT + 1) return true;
  }
  return false;
}

std::string GomokuState::serialize() const {
  std::stringstream ss;
  for (const Action action : history_) {
    ss << action << " ";
  }
  return ss.str();
}

std::vector<float> GomokuState::returns() const {
  if (winner_ == -1) {
    return {0.0F, 0.0F};
  }
  if (winner_ == 0) {
    return {1.0F, -1.0F};
  }
  // if (winner_ == 1)
  return {-1.0F, 1.0F};
}

std::string GomokuGame::name() const { return "gomoku"; }
int GomokuGame::num_players() const { return 2; }
int GomokuGame::num_distinct_actions() const { return NUM_GRIDS; }
std::vector<int> GomokuGame::observation_tensor_shape() const {
  return {4, BOARD_SIZE, BOARD_SIZE};
}

StatePtr GomokuGame::new_initial_state() const {
  return std::make_unique<GomokuState>(shared_from_this());
}

StatePtr GomokuGame::deserialize_state(const std::string& str) const {
  std::stringstream ss(str);
  int action;
  StatePtr state = new_initial_state();
  while (ss >> action) {
    state->apply_action(action);
  }
  return state->clone();
}

}  // namespace clap::game::gomoku
