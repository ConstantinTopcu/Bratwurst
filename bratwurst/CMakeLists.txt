option(ENABLE_DEBUG "Enable runtime debug tools like ASSERT" OFF)

if(ENABLE_DEBUG)
    message(STATUS "Runtime debug tools enabled")
    add_compile_definitions(ENABLE_DEBUG=1)
else()
    message(STATUS "Runtime debug tools disabled")
endif()


add_library(BratwurstEngine STATIC 
    "src/position.cpp" 
    "src/precomputed.cpp" 
    "src/magic_gen.cpp"
    "include/move_gen.h" 
    "src/move_gen.cpp"
 "include/perft.h" "include/zobrist.h")

target_include_directories(BratwurstEngine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)