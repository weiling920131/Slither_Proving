#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "clap/mcts/engine.h"
#include "clap/mcts/vl/engine.h"

namespace clap::mcts {
namespace {

namespace py = ::pybind11;
using py::literals::operator""_a;

PYBIND11_MODULE(mcts, m) {  // NOLINT
  py::class_<Engine>(m, "Engine")
      .def(py::init<const std::vector<int>&, int>(), "gpus"_a, "models"_a = 1)
      .def_readwrite_static("batch_size", &Engine::batch_size)
      .def_readwrite_static("max_simulations", &Engine::max_simulations)
      .def_readwrite_static("c_puct", &Engine::c_puct)
      .def_readwrite_static("dirichlet_alpha", &Engine::dirichlet_alpha)
      .def_readwrite_static("dirichlet_epsilon", &Engine::dirichlet_epsilon)
      .def_readwrite_static("float_error", &Engine::float_error)
      .def_readwrite_static("num_sampled_transformations",
                            &Engine::num_sampled_transformations)
      .def_readwrite_static("batch_per_job", &Engine::batch_per_job)
      .def_readwrite_static("temperature_drop", &Engine::temperature_drop)
      .def_readwrite_static("inference_wait_ms", &Engine::inference_wait_ms)
      .def_readwrite_static("play_until_terminal", &Engine::play_until_terminal)
      .def_readwrite_static("auto_reset_job", &Engine::auto_reset_job)
      .def_readwrite_static("play_until_turn_player",
                            &Engine::play_until_turn_player)
      .def_readwrite_static("mcts_until_turn_player",
                            &Engine::mcts_until_turn_player)
      .def_readwrite_static("verbose", &Engine::verbose)
      .def_readwrite_static("top_n_childs", &Engine::top_n_childs)
      .def_readwrite_static("states_value_using_childs",
                            &Engine::states_value_using_childs)
      .def_readwrite_static("dump_tree", &Engine::dump_tree)
      .def_readwrite_static("save_observation", &Engine::save_observation)
      .def_readwrite_static("report_serialize_string", &Engine::report_serialize_string)
      .def("load_game", &Engine::load_game, "name"_a)
      .def("load_model", &Engine::load_model, "path"_a, "version"_a = 0)
      .def("start", &Engine::start, "cpu_workers"_a, "gpu_workers"_a,
           "num_envs"_a = -1)
      .def("stop", &Engine::stop)

      .def("add_job", &Engine::add_job, "num"_a = 1, "serialize_string"_a = "")

      .def(
          "get_trajectory",
          [](Engine& engine) { return py::bytes(engine.get_trajectory()); },
          py::call_guard<py::gil_scoped_release>());

  py::class_<vl::Engine>(m, "VLEngine")
      .def(py::init<const std::vector<int>&, int>(), "gpus"_a, "models"_a = 1)
      .def_readwrite_static("batch_size", &vl::Engine::batch_size)
      .def_readwrite_static("max_simulations", &vl::Engine::max_simulations)
      .def_readwrite_static("c_puct", &vl::Engine::c_puct)
      .def_readwrite_static("dirichlet_alpha", &vl::Engine::dirichlet_alpha)
      .def_readwrite_static("dirichlet_epsilon", &vl::Engine::dirichlet_epsilon)
      .def_readwrite_static("float_error", &vl::Engine::float_error)
      .def_readwrite_static("num_sampled_transformations",
                            &vl::Engine::num_sampled_transformations)
      .def_readwrite_static("batch_per_job", &vl::Engine::batch_per_job)
      .def_readwrite_static("temperature_drop", &vl::Engine::temperature_drop)
      .def_readwrite_static("inference_wait_ms", &vl::Engine::inference_wait_ms)
      .def_readwrite_static("virtual_loss", &vl::Engine::virtual_loss)
      .def_readwrite_static("play_until_terminal", &vl::Engine::play_until_terminal)
      .def_readwrite_static("auto_reset_job", &vl::Engine::auto_reset_job)
      .def_readwrite_static("play_until_turn_player",
                            &vl::Engine::play_until_turn_player)
      .def_readwrite_static("mcts_until_turn_player",
                            &vl::Engine::mcts_until_turn_player)
      .def_readwrite_static("verbose", &vl::Engine::verbose)
      .def_readwrite_static("top_n_childs", &vl::Engine::top_n_childs)
      .def_readwrite_static("states_value_using_childs",
                            &vl::Engine::states_value_using_childs)
      .def("load_game", &vl::Engine::load_game, "name"_a)
      .def("load_model", &vl::Engine::load_model, "path"_a, "version"_a = 0)
      .def("start", &vl::Engine::start, "cpu_workers"_a, "gpu_workers"_a,
           "num_envs"_a = -1)
      .def("stop", &vl::Engine::stop)

      .def("add_job", &vl::Engine::add_job, "num"_a = 1, "serialize_string"_a = "")

      .def(
          "get_trajectory",
          [](vl::Engine& engine) { return py::bytes(engine.get_trajectory()); },
          py::call_guard<py::gil_scoped_release>());
}

}  // namespace
}  // namespace clap::mcts
