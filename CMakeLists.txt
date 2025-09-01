cmake_minimum_required(VERSION 3.20)
project(BratwurstEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(bratwurst)
add_subdirectory(uci)
add_subdirectory(tests)