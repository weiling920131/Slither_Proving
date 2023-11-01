add_library(clap.mcts OBJECT
  node.cc
  tree.cc
  job.cc
  job_expand.cc
  model_manager.cc
  engine.cc
  vl/node.cc
  vl/tree.cc
  vl/job.cc
  vl/job_expand.cc
  vl/engine.cc
)

clap_pybind11_add_module(mcts
  module.cc
  $<TARGET_OBJECTS:clap.mcts>
  $<TARGET_OBJECTS:clap.game>
  $<TARGET_OBJECTS:clap.proto>
)

target_link_libraries(mcts PRIVATE ${Protobuf_LIBRARIES} ${TORCH_LIBRARIES})
