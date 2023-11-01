file(GLOB GAMES_SRC "*/*.cc")

add_library(clap.game OBJECT game.cc ${GAMES_SRC})

clap_pybind11_add_module(game
  module.cc
  $<TARGET_OBJECTS:clap.game>
)