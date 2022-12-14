cmake_minimum_required(VERSION 3.14)

project(SketchFetchTests LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(SketchFetch REQUIRED)
  enable_testing()
endif()

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(SketchFetch_test source/SketchFetch_test.cpp)
target_link_libraries(
    SketchFetch_test PRIVATE
    SketchFetch::SketchFetch
    Catch2::Catch2WithMain
)
target_compile_features(SketchFetch_test PRIVATE cxx_std_20)

catch_discover_tests(SketchFetch_test)

# ---- End-of-file commands ----

add_folders(Test)
