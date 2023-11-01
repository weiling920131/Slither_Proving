#include "clap/game/slither9/slither9.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stack>
#include <iostream>

namespace clap::game::slither9 {

Slither9State::Slither9State(GamePtr game_ptr)
    : State(std::move(game_ptr)),
      turn_(0),
      skip_(0),
      winner_(-1) {
  std::fill(std::begin(board_), std::end(board_), EMPTY);
  history_.clear();
}

StatePtr Slither9State::clone() const {
  return std::make_unique<Slither9State>(*this);
}

void Slither9State::apply_action(const Action &action) {
  //std::cout << "current turn:" << turn_ << "\n"; 
  //std::cout << "action:" << action << '\n';
  if (turn_ % 3 == 0) {  // choose
		if (action == empty_index) skip_ = 1;
		++turn_;
		history_.push_back(action);
		//std::cout << "success\n";
	} 
	else if (turn_ % 3 == 1) { // move
		Player player = current_player();
		const int src = history_[history_.size() - 1];
		if(src != empty_index && action != empty_index){
			std::swap(board_[src], board_[action]);
			board_[src] = EMPTY;
		}
		++turn_;
		history_.push_back(action);
	} 
	else {  
		// place new piece
		Player player = current_player();
		board_[action] = player;
		++turn_;
		history_.push_back(action);
		// check if valid
		if (skip_ == 1) { // only play a new piece
			skip_ = 0;
			if (!is_valid(action)) {
				if(player == BLACK)
					winner_ = WHITE;
				else 
					winner_ = BLACK;
			}
			// check if win
			if(winner_ == -1 && have_win(action))
				winner_ = player;
		}
		else {
			const int src_empty = history_[history_.size() - 3]; // piece -> empty
			const int src_new_piece = history_[history_.size() - 2]; // empty -> piece
			if (!is_valid(action) || !is_valid(src_new_piece) || (action != src_empty && !is_empty_valid(src_empty) ) ) {
				if(player == BLACK)
					winner_ = WHITE;
				else 
					winner_ = BLACK;
			}
			// check if win
			if(winner_ == -1){
				if(have_win(action) || have_win(src_new_piece))
					winner_ = player;
			}
		}
	}
}

std::vector<Action> Slither9State::legal_actions() const {
	std::vector<Action> actions;
	if(turn_ % 3 == 0) // empty
		actions.push_back(empty_index);
	else if(turn_ % 3 == 1 && skip_ == 1){
		actions.push_back(empty_index);
		return actions;
	}
	for (uint32_t i = 0; i < kNumOfGrids; i++) {
		if (is_legal_action(i)) {
			actions.push_back(i);
		}
	}
	/*
	for(int i = 0; i < actions.size(); i++){
		std::cout << actions[i] << '\n';
	}
	*/
	return actions;
}

bool Slither9State::is_legal_action(const Action &action) const {
	if (action < 0 || action >= kNumOfGrids) return false;
	const Player player = current_player();
	if (turn_ % 3 == 0) {  // choose
		if (board_[action] == player && have_move(action)) {
			return true;
		}
	}
	else if (turn_ % 3 == 1) {  // move
		const Action &src = history_[history_.size() - 1];
		if (board_[action] == EMPTY) {
			if ((std::abs(action / kBoardSize - src / kBoardSize) <= 1) &&
				(std::abs(action % kBoardSize - src % kBoardSize) <= 1))
				return true;
		}
		return false;
	} else {  // new chess
		if(board_[action] == EMPTY)
			return true;
	}
	return false;
}

bool Slither9State::have_move(const Action &action) const {
  for (int const &diff : MoveDirection) {
    int dest = action + diff;
    if ((dest >= 0 && dest < kNumOfGrids && board_[dest] == EMPTY) &&
        (std::abs(dest / kBoardSize - action / kBoardSize) <= 1) &&
        (std::abs(dest % kBoardSize - action % kBoardSize) <= 1)) {
      return true;
    }
  }
  return false;
}

bool Slither9State::have_win(const Action &action) const {
	if (action < 0 || action >= kNumOfGrids) return false;
	const Player player = board_[action];
	std::stack<int> pieces;
	pieces.push(int(action));
	int traversed[81] = {0}; //const
	traversed[action] = 1;
	int head = 0, tail = 0; 
	while(!pieces.empty()){
		int cur = pieces.top();
		pieces.pop();
		int u = cur + MoveDirection[1];
		int l = cur + MoveDirection[3];
		int r = cur + MoveDirection[4];
		int b = cur + MoveDirection[6];
		int top_check = 1, bottom_check = 1, left_check = 1, right_check = 1;
		if(cur / kBoardSize == 0) top_check = 0;
		if(cur / kBoardSize == kBoardSize - 1) bottom_check = 0;
		if(cur % kBoardSize == 0) left_check = 0;
		if(cur % kBoardSize == kBoardSize - 1) right_check = 0;
		if(top_check && traversed[u] == 0 && board_[u] == player){
			traversed[u] = 1;
			pieces.push(u);
		}
		if(left_check && traversed[l] == 0 && board_[l] == player){
			traversed[l] = 1;
			pieces.push(l);
		}
		if(right_check && traversed[r] == 0 && board_[r] == player){
			traversed[r] = 1;
			pieces.push(r);
		}
		if(bottom_check && traversed[b] == 0 && board_[b] == player){
			traversed[b] = 1;
			pieces.push(b);
		}
	}
	if(player == BLACK){
		for(int i = 0; i < kBoardSize; i++){
			if(traversed[i] == 1)
				head = 1;
			if(traversed[kBoardSize * (kBoardSize - 1) + i] == 1)
				tail = 1;
		}
	}
	else{
		for(int i = 0; i < kBoardSize; i++){
			if(traversed[i*kBoardSize] == 1)
				head = 1;
			if(traversed[kBoardSize * (i+1) - 1] == 1)
				tail = 1;
		}
	}
	if(head && tail)
		return true;
	return false;
}

bool Slither9State::is_valid(const Action &action) const {
	if (action < 0 || action >= kNumOfGrids) return false;
	const Player player = board_[action];
	int top_check = 1, bottom_check = 1, left_check = 1, right_check = 1;
	if(action / kBoardSize == 0) top_check = 0;
	if(action / kBoardSize == kBoardSize - 1) bottom_check = 0;
	if(action % kBoardSize == 0) left_check = 0;
	if(action % kBoardSize == kBoardSize - 1) right_check = 0;
	int ul = action + MoveDirection[0];
	int u = action + MoveDirection[1];
	int ur = action + MoveDirection[2];
	int l = action + MoveDirection[3];
	int r = action + MoveDirection[4];
	int bl = action + MoveDirection[5];
	int b = action + MoveDirection[6];
	int br = action + MoveDirection[7];
	// upper left check
	if(top_check && left_check && board_[ul] == player && board_[u] != player && board_[l] != player)
		return false;
	// upper right check
	if(top_check && right_check && board_[ur] == player && board_[u] != player && board_[r] != player)
		return false;
	// lower left check
	if(bottom_check && left_check && board_[bl] == player && board_[b] != player && board_[l] != player)
		return false;
	// lower right check
	if(bottom_check && right_check && board_[br] == player && board_[b] != player && board_[r] != player)
		return false;
	return true;
}

bool Slither9State::is_empty_valid(const Action &action) const {
	if (action < 0 || action >= kNumOfGrids) return false;
	if(board_[action] != EMPTY) return true;
	int top_check = 1, bottom_check = 1, left_check = 1, right_check = 1;
	if(action / kBoardSize == 0) top_check = 0;
	if(action / kBoardSize == kBoardSize - 1) bottom_check = 0;
	if(action % kBoardSize == 0) left_check = 0;
	if(action % kBoardSize == kBoardSize - 1) right_check = 0;
	int ul = action + MoveDirection[0];
	int u = action + MoveDirection[1];
	int ur = action + MoveDirection[2];
	int l = action + MoveDirection[3];
	int r = action + MoveDirection[4];
	int bl = action + MoveDirection[5];
	int b = action + MoveDirection[6];
	int br = action + MoveDirection[7];
	// upper left check
	if(top_check && left_check && board_[u] != EMPTY && board_[u] == board_[l] && board_[u] != board_[ul])
		return false;
	// upper right check
	if(top_check && right_check && board_[u] != EMPTY && board_[u] == board_[r] && board_[u] != board_[ur])
		return false;
	// lower left check
	if(bottom_check && left_check && board_[b] != EMPTY && board_[b] == board_[l] && board_[b] != board_[bl])
		return false;
	// lower right check
	if(bottom_check && right_check && board_[b] != EMPTY && board_[b] == board_[r] && board_[b] != board_[br])
		return false;
	return true;
}

std::string Slither9State::to_string() const {
  std::stringstream ss;
  const std::vector<std::string> chess{"o", "x", "·"};

  for (int i = 0; i < kBoardSize; ++i) {
    ss << std::setw(2) << std::setfill(' ') << kBoardSize - i;
    for (int j = 0; j < kBoardSize; ++j) {
        ss << ' ' << chess[board_[i * kBoardSize + j]];
    }
    ss << std::endl;
  }
  ss << "  ";
  for (int i = 0; i < kBoardSize; ++i) ss << ' ' << static_cast<char>('A' + i);
  return ss.str();
}

bool Slither9State::is_terminal() const {
  //return (winner_ != -1) || (legal_actions().empty());
  return (winner_ != -1);
}

Player Slither9State::current_player() const {
  return (Player)((turn_ % 6) / 3);
}

std::vector<float> Slither9State::observation_tensor() const {
  std::vector<float> tensor;
  tensor.reserve(4 * 9 * 9);
  Player player = current_player();
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == player));
  }
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>(board_[i] == 1 - player));
  }
  /*
  00: turn % 3 == 0
  01: turn % 3 == 1
  10: turn % 3 == 2
  11: unused
  */
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>((turn_ % 3 == 2)));
  }
  for (int i = 0; i < kNumOfGrids; ++i) {
    tensor.push_back(static_cast<float>((turn_ % 3 == 1)));
  }
  return tensor;
}

std::vector<float> Slither9State::returns() const {
  if (winner_ == BLACK) {
    return {1.0, -1.0};
  }
  if (winner_ == WHITE) {
    return {-1.0, 1.0};
  }
  return {0.0, 0.0};
}

std::string Slither9State::serialize() const {
  std::stringstream ss;
  for (const Action action : history_) {
    ss << action << " ";
  }
  return ss.str();
}

std::string Slither9Game::name() const {
  return "slither9";
}
int Slither9Game::num_players() const { return 2; }
int Slither9Game::num_distinct_actions() const {
  return kPolicyDim;
}
std::vector<int> Slither9Game::observation_tensor_shape() const {
  return {4, kBoardSize, kBoardSize};
}

StatePtr Slither9Game::new_initial_state() const {
  return std::make_unique<Slither9State>(shared_from_this());
}

int Slither9Game::num_transformations() const { return 4; }

std::vector<float> Slither9Game::transform_observation(
    const std::vector<float> &observation, int type) const {
  std::vector<float> data(observation);
  if (type != 0) {
    int feature_num = (int)observation.size() / kNumOfGrids;
    for (int i = 0; i < feature_num; i++) {
      for (int index = 0; index < kNumOfGrids; index++) {
        int new_index = transform_index(index, type);
        data[i * kNumOfGrids + new_index] =
            observation[i * kNumOfGrids + index];
      }
    }
  }
  return data;
}

std::vector<float> Slither9Game::transform_policy(
    const std::vector<float> &policy, int type) const {
  std::vector<float> data(policy);
  if (type != 0) {
    for (int index = 0; index < kNumOfGrids; index++) {
      int new_index = transform_index(index, type);
      data[new_index] = policy[index];
    }
  }
  return data;
}

std::vector<float> Slither9Game::restore_policy(
    const std::vector<float> &policy, int type) const {
  std::vector<float> data(policy);
  if (type != 0) {
    for (int index = 0; index < kNumOfGrids; index++) {
      int new_index = transform_index(index, type);
      data[new_index] = policy[index];
    }
  }
  return data;
}

int Slither9Game::transform_index(const int &index, const int type) const {
  if (type == 0) {
    return index;
  } else {
    int i = index / kBoardSize;
    int j = index % kBoardSize;
    if (type == 1) { // reflect horizontal
      j = (kBoardSize - 1) - j;
    }
    else if (type == 2) { // reflect vertical
      i = (kBoardSize - 1) - i;
    }
    else if (type == 3) { // reverse
      i = (kBoardSize - 1) - i;
      j = (kBoardSize - 1) - j;
    }
    return i * kBoardSize + j;
  }
}

std::string Slither9Game::action_to_string(
    const Action &action) const {
  using namespace std::string_literals;
  std::stringstream ss;
  if(action == empty_index)
      ss << "X";
  else
    ss << "ABCDEFGHIJKLM"s.at(action % kBoardSize) << kBoardSize - (action / kBoardSize);
  return ss.str();
}

std::vector<Action> Slither9Game::string_to_action(
    const std::string &str) const {
  std::string wopunc = str;
  wopunc.erase(std::remove_if(wopunc.begin(), wopunc.end(), ispunct),
               wopunc.end());
  std::stringstream ss(wopunc);
  std::string action;
  std::vector<Action> id;
  while (ss >> action) {
    if (action.at(0) == 'X' || action.at(0) == 'x') {
      id.push_back(empty_index);
    } else if (action.at(0) >= 'a' && action.at(0) <= 'z') {
      int tmp = (kBoardSize - std::stoi(action.substr(1))) * kBoardSize +
                (action.at(0) - 'a');
      id.push_back(tmp);
    } else {
      int tmp = (kBoardSize - std::stoi(action.substr(1))) * kBoardSize +
                (action.at(0) - 'A');
      id.push_back(tmp);
    }
  }
  return id;
}

StatePtr Slither9Game::deserialize_state(
    const std::string &str) const {
  std::stringstream ss(str);
  int action;
  StatePtr state = new_initial_state();
  while (ss >> action) {
    state->apply_action(action);
  }
  return state->clone();
}

}  // namespace clap::game::slither9
