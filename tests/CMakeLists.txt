include(FetchContent)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0  # or latest stable version
)
FetchContent_MakeAvailable(catch2)

add_executable(Tests
    "test_main.cpp"
    "square_test.cpp"
    "piece_test.cpp"
    "bitboard_test.cpp")

target_link_libraries(Tests PRIVATE BratwurstEngine Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(Tests)