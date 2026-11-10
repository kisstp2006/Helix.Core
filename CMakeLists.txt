cmake_minimum_required(VERSION 3.28)
project(Helix.Core LANGUAGES CXX)

if(NOT COMMAND helix_add_library)
    include("${CMAKE_CURRENT_SOURCE_DIR}/External/Helix.CMake/Helix.cmake")
endif()

helix_add_library(helix_core ALIAS Core)

if(HELIX_PLATFORM STREQUAL "windows")
    target_link_libraries(helix_core PRIVATE bcrypt)
elseif(HELIX_PLATFORM STREQUAL "android")
    target_link_libraries(helix_core PUBLIC log)
endif()

helix_add_test_suite(helix_core_tests DEPS Helix::Core)
helix_add_benchmark_suite(helix_core_bench DEPS Helix::Core)
