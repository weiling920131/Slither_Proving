#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace clap::game {

using Player = int;
using Action = int;

class State;
using StatePtr = std::unique_ptr<State>;

class Game;
using GamePtr = std::shared_ptr<const Game>;

class State {
 public:
  State(GamePtr game) : game_(game) {}
  virtual ~State() = default;
  virtual Player current_player() const = 0;
  virtual Player get_winner() const { return -1; }
  virtual std::vector<Action> legal_actions() const = 0;
  virtual std::vector<float> observation_tensor() const = 0;
  virtual void apply_action(const Action&) = 0;
  virtual void manual_action(const Action&, Player) { return; }
  // 11/7 modified
  virtual void reset_path() { return; }
  // virtual void test_action(std::vector<Action>) { return; }
  virtual int test_action(std::vector<Action>, Player) { return 0; }
  virtual int test_generate(std::vector<Action>, int, int) { return 0; }
  // 11/7 modified
  //whp
  virtual bool check(std::vector<int>) { return 0; };
  virtual void DFS(std::vector<std::vector<int>> &, std::vector<int> &, int , int) { return; };
  virtual std::vector<std::vector<int>> generate(int cnt) { return {}; };
  //whp
  virtual bool is_terminal() const = 0;
  virtual std::vector<float> returns() const = 0;
  virtual StatePtr clone() const = 0;
  virtual std::string to_string() const { return ""; }
  GamePtr game() const { return game_; }

  virtual std::string serialize() const { return ""; }
  virtual std::string serialize_num_to_char() const { return ""; }

 protected:
  GamePtr game_;
};

class Game : public std::enable_shared_from_this<Game> {
 public:
  virtual ~Game() = default;
  virtual std::string name() const = 0;
  virtual int num_players() const = 0;
  virtual int num_distinct_actions() const = 0;
  virtual StatePtr new_initial_state() const = 0;
  virtual std::vector<int> observation_tensor_shape() const = 0;

  virtual int num_transformations() const;
  virtual std::vector<float> transform_observation(const std::vector<float>&,
                                                   int) const;
  virtual std::vector<float> transform_policy(const std::vector<float>&,
                                              int) const;
  virtual std::vector<float> restore_policy(const std::vector<float>&,
                                            int) const;

  virtual std::string action_to_string(const Action& action) const {
    return std::to_string(action);
  }
  virtual std::vector<Action> string_to_action(const std::string& str) const {
    return {std::stoi(str)};
  }
  virtual StatePtr deserialize_state(const std::string& str = "") const {
    return new_initial_state();
  }

  virtual int getBoardSize() const { return 0; }
  virtual bool save_manual(const std::vector<Action>, const std::string) const { return false; }

};

class Factory {
 public:
  using CreateFunc = std::function<GamePtr()>;
  static Factory& instance();
  void add(const CreateFunc&);
  std::vector<std::string> games() const;
  GamePtr create(const std::string& name);

 private:
  std::unordered_map<std::string, CreateFunc> factory_;
};

template <class GameType>
struct Registration {
  Registration() {
    Factory::instance().add([]() { return std::make_shared<GameType>(); });
  }
};

std::vector<std::string> list();
GamePtr load(const std::string& name);

}  // namespace clap::game