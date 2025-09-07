add_library(BratwurstEngine STATIC "src/position.cpp" "src/precomputed.cpp")
target_include_directories(BratwurstEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)