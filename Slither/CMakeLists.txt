cmake_minimum_required(VERSION 3.14)
project(clap)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Option
option(ENABLE_CLANG_TIDY "enable clang-tidy" ON)
option(ENABLE_CLANG_FORMAT "enable clang-format" ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(ENABLE_CLANG_TIDY OFF)
  # set(ENABLE_CLANG_FORMAT OFF)
endif()

# clang-tidy
if(ENABLE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy" "-config=" "-extra-arg"
                           "-fsized-deallocation")
endif()

add_subdirectory(third_party)
add_subdirectory(clap)

# clang-format
if(ENABLE_CLANG_FORMAT)
  include(ClangFormat)
  file(GLOB_RECURSE CLAP_SRC "clap/*.cc" "clap/*.h")
  list(FILTER CLAP_SRC EXCLUDE REGEX "pb/*")
  clangformat_setup(${CLAP_SRC})
endif()