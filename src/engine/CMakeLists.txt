add_library(Engine STATIC
	src/magic_gen.cpp
	src/move_gen.cpp
	src/position.cpp
	src/precomputed.cpp
)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)