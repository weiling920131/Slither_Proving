#include "clap/game/connect6/connect6.h"

#include <iomanip>
#include <numeric>
#include <sstream>

namespace clap::game::connect6 {

/** Construct a new state; the initial board is empty,
 * filled with "2"
 * ( 0: o     )
 * ( 1: x     )
 * ( 2: empty ) */
Connect6State::Connect6State(GamePtr game_ptr)
    : State(std::move(game_ptr)), turn_(0), winner_(-1) {
  board_.fill(2);
  // board_[9*19+9] = 0;
  // turn_ ++;

  // added by Wei-Yuan Hsu
  turn_to_ignore_stones_ = -1;
  ignored_stones_table = std::vector<bool>(NUM_GRIDS, false);
  ignored_pairs_table =
      std::vector<std::vector<bool> >(NUM_GRIDS, ignored_stones_table);
}

StatePtr Connect6State::clone() const {
  return std::make_unique<Connect6State>(*this);
}

/** Apply an action. It's about where you place a chess. */
void Connect6State::apply_action(const Action& action) {
  Player player = current_player();
  board_[action] = player;
  ++turn_;
  /** See if there's person who wins this game,
   * based on its last action,
   * doing this save time compared with checking the entire board.
   * ( There are 2 has_line functions:
   *   1. has_line(const Player&)
   *   2. has_line(const Player &, const Action & )
   * ) */
  if (has_line(player, action)) {
    winner_ = player;
  }

  // added by Wei-Yuan Hsu
  if (turn_ == turn_to_ignore_stones_) {
    ignored_stones_table = ignored_pairs_table[action];
  }
  history_.push_back(action);
}

/** Get the empty places which you can place your chess. */
std::vector<Action> Connect6State::legal_actions() const {
  // added by Wei-Yuan Hsu
  if (turn_ == turn_to_ignore_stones_) {
    std::vector<Action> actions;

    actions.reserve(NUM_GRIDS - turn_);
    for (int i = 0; i < NUM_GRIDS; ++i) {
      if (board_[i] == 2 && !ignored_stones_table[i]) actions.push_back(i);
    }
    return actions;
  }

  std::vector<Action> actions;

  actions.reserve(NUM_GRIDS - turn_);
  for (int i = 0; i < NUM_GRIDS; ++i)
    if (board_[i] == 2) actions.push_back(i);
  return actions;
}

/*   return board state now.*/
std::string Connect6State::to_string() const {
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

/** The game is terminal when one of the following situation occurs:
 *   - The board is full of chesses.
 *   - There's a person who wins this game. */
bool Connect6State::is_terminal() const {
  return turn_ == NUM_GRIDS || winner_ != -1;
}

/** Get the current player using the turn number.
 * the player can only put one chess.
 * From the second turn, players have to put two chesses.
 * Therefore,
 *   turn_ == 0: first player put 1 chess (+1 then %4 equals 1)
 *   turn_ == 1: second player put 1 chess (+1 then %4 equals 2)
 *   turn_ == 2: second player put 1 chess (+1 then %4 equals 3)
 *   turn_ == 3: first player put 1 chess (+1 then %4 equals 0)
 *   turn_ == 4: first player put 1 chess (+1 then %4 equals 1)
 *   From the above, we can see when (turn_+1)%4 equals 0 or 1,
 *   it's the first player's turn. */
Player Connect6State::current_player() const {
  // return ((turn_ + 1) % 4) >= 2 ? 1 : 0;
  return ((turn_ + 1) % 4) >= 2 ? 1 : 0;
}

/** {6,19,19}
 * (channel,heigt,width) */
std::vector<float> Connect6State::observation_tensor() const {
  std::vector<float> tensor;
  auto shape = game()->observation_tensor_shape();
  auto size =
      std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>());
  tensor.reserve(size);

  /** layer 1: current black side */
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == 0));
  }
  /** layer 2: current white side */
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == 1));
  }
  /** layer 3: who's turn */
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(turn_ % 4 == 0));
  }
  /** layer 4: who's turn */
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(turn_ % 4 == 1));
  }
  /** layer 5: who's turn */
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(turn_ % 4 == 2));
  }
  /** layer 6: who's turn */
  for (int i = 0; i < NUM_GRIDS; ++i) {
    tensor.push_back(static_cast<float>(turn_ % 4 == 3));
  }

  return tensor;
}

/** Judge if there's a completed line.
 * There are 2 has_line functions.
 * This is type 1 */
bool Connect6State::has_line(const Player& player, const Action& action) const {
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

/*return values state*/
std::vector<float> Connect6State::returns() const {
  if (winner_ == -1) {
    return {0.0F, 0.0F};
  }
  if (winner_ == 0) {
    return {1.0F, -1.0F};
  }
  // if (winner_ == 1)
  return {-1.0F, 1.0F};
}

std::string Connect6State::serialize() const {
  std::stringstream ss;
  for (const Action action : history_) {
    ss << action << " ";
  }
  return ss.str();
}

// added by Wei-Yuan Hsu
void Connect6State::modify(const std::vector<Action> actions) {
  turn_to_ignore_stones_ = turn_ + 1;
  ignored_pairs_table[actions[0]][actions[1]] = true;
  ignored_pairs_table[actions[1]][actions[0]] = true;
}

/** Get the name of this game. (connect6) */
std::string Connect6Game::name() const { return "connect6"; }

/** Return the number of players.
 * There are 2 players in connect6. */
int Connect6Game::num_players() const { return 2; }

/** All grids are posible actions. */
int Connect6Game::num_distinct_actions() const { return NUM_GRIDS; }

/** Format of this refers the comments in czf_env.h
 * (channel, height, width) */
std::vector<int> Connect6Game::observation_tensor_shape() const {
  return {6, BOARD_SIZE, BOARD_SIZE};
}
/** Get an initial state. */
StatePtr Connect6Game::new_initial_state() const {
  return std::make_unique<Connect6State>(shared_from_this());
}

int Connect6Game::num_transformations() const { return 8; }

std::vector<float> Connect6Game::transform_observation(
    const std::vector<float>& observation, int type) const {
  std::vector<float> transformed_observation(observation);
  transform(&transformed_observation[0], type);
  transform(&transformed_observation[NUM_GRIDS], type);
  return transformed_observation;
}

std::vector<float> Connect6Game::transform_policy(
    const std::vector<float>& policy, int type) const {
  std::vector<float> transformed_policy(policy);
  transform(&transformed_policy[0], type);
  return transformed_policy;
}

std::vector<float> Connect6Game::restore_policy(
    const std::vector<float>& policy, int type) const {
  std::vector<float> restored_policy(policy);
  int restored_type;
  switch (type) {
    default:
    case 0:
      restored_type = 0;
      break;
    case 1:
      restored_type = 3;
      break;
    case 2:
      restored_type = 2;
      break;
    case 3:
      restored_type = 1;
      break;
    case 4:
      restored_type = 4;
      break;
    case 5:
      restored_type = 5;
      break;
    case 6:
      restored_type = 6;
      break;
    case 7:
      restored_type = 7;
      break;
  }
  transform(&restored_policy[0], restored_type);
  return restored_policy;
}

void Connect6Game::transform(float* tensor, int type) const {
  switch (type) {
    default:
    case 0:
      break;
    case 1:
      rotate_right(tensor);
      break;
    case 2:
      reverse(tensor);
      break;
    case 3:
      rotate_left(tensor);
      break;
    case 4:
      transpose(tensor);
      break;
    case 5:
      rotate_right(tensor);
      transpose(tensor);
      break;
    case 6:
      reverse(tensor);
      transpose(tensor);
      break;
    case 7:
      rotate_left(tensor);
      transpose(tensor);
      break;
  }
}

void Connect6Game::rotate_right(float* tensor) const {
  transpose(tensor);
  reflect_horizontal(tensor);
}

void Connect6Game::rotate_left(float* tensor) const {
  transpose(tensor);
  reflect_vertical(tensor);
}

void Connect6Game::reverse(float* tensor) const {
  reflect_horizontal(tensor);
  reflect_vertical(tensor);
}

void Connect6Game::transpose(float* tensor) const {
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = i; j < BOARD_SIZE; j++) {
      int positionA = i * BOARD_SIZE + j;
      int positionB = j * BOARD_SIZE + i;
      std::swap(tensor[positionA], tensor[positionB]);
    }
  }
}

void Connect6Game::reflect_horizontal(float* tensor) const {
  for (int i = 0; i < BOARD_SIZE; i++) {
    for (int j = 0, j2 = BOARD_SIZE - 1; j < j2; j++, j2--) {
      int positionA = i * BOARD_SIZE + j;
      int positionB = i * BOARD_SIZE + j2;
      std::swap(tensor[positionA], tensor[positionB]);
    }
  }
}

void Connect6Game::reflect_vertical(float* tensor) const {
  for (int i = 0, i2 = BOARD_SIZE - 1; i < i2; i++, i2--) {
    for (int j = 0; j < BOARD_SIZE; j++) {
      int positionA = i * BOARD_SIZE + j;
      int positionB = i2 * BOARD_SIZE + j;
      std::swap(tensor[positionA], tensor[positionB]);
    }
  }
}

StatePtr Connect6Game::deserialize_state(const std::string& str) const {
  std::stringstream ss(str);
  int action;
  StatePtr state = new_initial_state();
  while (ss >> action) {
    state->apply_action(action);
  }
  return state->clone();
}

}  // namespace clap::game::connect6
