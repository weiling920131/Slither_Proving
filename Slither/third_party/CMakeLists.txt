add_subdirectory(pybind11)

function(clap_pybind11_add_module target_name)
  pybind11_add_module(${target_name} ${ARGN})

  set(output_directory ${CMAKE_CURRENT_BINARY_DIR}/..)

  if(DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    file(RELATIVE_PATH relative_dir ${CMAKE_BINARY_DIR} ${output_directory})
    set(output_directory ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${relative_dir})
  endif()

  set_target_properties(${target_name} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${output_directory}
  )
endfunction()