#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "clap/game/game.h"

namespace clap::game {
namespace {

namespace py = ::pybind11;
using py::literals::operator""_a;

PYBIND11_MODULE(game, m) {  // NOLINT
  m.def("list", &list).def("load", &load, "name"_a);

  py::class_<State>(m, "State")
      .def_property_readonly("game", &State::game)
      .def("current_player", &State::current_player)
      .def("legal_actions", &State::legal_actions)
      .def("observation_tensor", &State::observation_tensor)
      .def("apply_action", &State::apply_action)
      .def("manual_action", &State::manual_action)
    // 11/7 modified
      // .def("return_path", &State::return_path)
      .def("test_action", &State::test_action)
    // 11/7 modified
    //whp
      .def("check", &State::check)
      .def("DFS", &State::DFS)
      .def("generate", &State::generate)
      .def("test_generate", &State::test_generate)
      .def("getboard", &State::getboard)
    //whp
      .def("is_terminal", &State::is_terminal)
      .def("get_winner", &State::get_winner)
      .def("returns", &State::returns)
      .def("clone", &State::clone)
      .def("serialize", &State::serialize)
      .def("__repr__", &State::to_string);

  py::class_<Game, std::shared_ptr<Game>>(m, "Game")
      .def_property_readonly("name", &Game::name)
      .def_property_readonly("num_players", &Game::num_players)
      .def_property_readonly("num_distinct_actions",
                             &Game::num_distinct_actions)
      .def_property_readonly("observation_tensor_shape",
                             &Game::observation_tensor_shape)
      .def("new_initial_state", &Game::new_initial_state)
      .def_property_readonly("num_transformations", &Game::num_transformations)
      .def("transform_observation", &Game::transform_observation)
      .def("transform_policy", &Game::transform_policy)
      .def("restore_policy", &Game::restore_policy)
      .def("action_to_string", &Game::action_to_string)
      .def("string_to_action", &Game::string_to_action)
      .def_property_readonly("getBoardSize", &Game::getBoardSize)
      .def("save_manual", &Game::save_manual)
      .def("deserialize_state", &Game::deserialize_state);
}

}  // namespace
}  // namespace clap::game
