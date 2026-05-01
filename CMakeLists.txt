cmake_minimum_required(VERSION 3.22)
project(osearch LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(EXPAT REQUIRED)
find_path(GSL_INCLUDE_DIR gsl/gsl PATHS /home/uabpath/GSL/include REQUIRED)

add_executable(osearch
  main.cc my_rand.cc obj.cc point.cc compile.cc
  execute.cc getopts.cc measure.cc print.cc
  read_conf.cc search.cc steps.cc)

target_include_directories(osearch PRIVATE ${GSL_INCLUDE_DIR})
target_link_libraries(osearch PRIVATE EXPAT::EXPAT)
