cmake_minimum_required(VERSION 3.21)
project(dHists)

set(CMAKE_CXX_STANDARD 14)
find_package(ROOT 6.18 CONFIG REQUIRED)

add_executable(dHistsSingle dHistsSinglePulse.cpp)
target_link_libraries(dHistsSingle PUBLIC ROOT::Physics ROOT::Hist)