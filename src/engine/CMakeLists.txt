add_library(Engine STATIC
	src/magic_gen.cpp
	src/move_gen.cpp
	src/position.cpp
	src/precomputed.cpp
	src/search.cpp
	src/move_picker.cpp
	src/move.cpp
	src/uci.cpp
	src/evaluation_constants.cpp)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)