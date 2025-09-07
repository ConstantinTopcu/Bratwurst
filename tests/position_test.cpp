#include <catch2/catch_all.hpp>
#include <position.h>

using namespace Bratwurst;

TEST_CASE("check valid fens")
{
    std::string validFens[] =
    {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "8/8/8/8/8/8/8/8 b - - 0 1",
        "8/8/8/8/4k3/8/8/4K3 w - - 5 42",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq e3 0 2",
        "r3k2r/8/8/8/8/8/8/R3K2R w K - 12 34",
        "r3k2r/8/8/8/8/8/8/R3K2R b q - 7 55",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 3 10",
        "rnbqkb1r/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e6 0 2",
        "8/8/8/3pP3/8/8/8/8 w - d6 0 15",
        "r1bq1rk1/pppp1ppp/2n2n2/2b1p3/4P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 6 10"
    };

    for (const std::string& validFen : validFens)
    {
        INFO("Testing valid FEN: " << validFen);
        auto result = Position::fromFEN(validFen);
        REQUIRE(result.has_value());
        REQUIRE(validFen == result->fen());
    }
}

TEST_CASE("check invalid fens")
{
    std::vector<std::pair<std::string, Position::FenError>> invalidFens =
    {
        {"8/8/8/8/8/8/8/8 w - - 0", Position::FenError::InvalidFormat},
        {"9/8/8/8/8/8/8/8 w - - 0 1", Position::FenError::InvalidPiecePlacement},
        {"8/8/8/8/8/8/8/8 x - - 0 1", Position::FenError::InvalidColorToMove},
        {"8/8/8/8/8/8/8/8 w A - 0 1", Position::FenError::InvalidCastlingRights},
        {"8/8/8/8/8/8/8/8 w -z - 0 1", Position::FenError::InvalidCastlingRights},
        {"8/8/8/8/8/8/8/8 w - i8 0 1", Position::FenError::InvalidEnPassantSquare},
        {"8/8/8/8/8/8/8/8 w - - -1 1", Position::FenError::InvalidHalfmoveClock},
        {"8/8/8/8/8/8/8/8 w - - 51 1", Position::FenError::InvalidHalfmoveClock},
        {"8/8/8/8/8/8/8/8 w - - 0 -1", Position::FenError::InvalidFullMoveCounter},
        {"rnbqkbnrr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", Position::FenError::InvalidPiecePlacement},
        {"/8/8/8/8/8/8/8 w - - 0 1", Position::FenError::InvalidFormat}
    };

    for (const auto& [fen, error] : invalidFens)
    {
        INFO("Testing invalid FEN: " << fen);
        auto result = Position::fromFEN(fen);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == error);
    }
}

TEST_CASE("check doMove/undoMove consistancy")
{
    auto result = Position::fromFEN(std::string(Position::StartPosFEN));
    Position pos = result.value();

    Move moves[] =
    {
        Move(E2, E4),
        Move(E7, E5),
        Move(G1, F3),
        Move(G8, G6),
        Move(F1, E2),
        Move(F8, E7),
        Move(E1, G1, Move::Flag::CastlingOO),
        Move(E8, G8, Move::Flag::CastlingOO),
        Move(D2, D4),
        Move(E5, D4), //black captures d4
        Move(C2, C4),
        Move(D4, C3, Move::Flag::EnPassant),
        Move(B2, B4),
        Move(C7, C5),
        Move(B4, C5),
        Move(B7, B6),
        Move(C5, B6),
        Move(H7, H6),
        Move(B6, B7),
        Move(B8, C6),
        Move(B7, A8, Move::Flag::QueenPromotion)
    };

    for (auto m : moves)
    {
        pos.doMove(m);
    }

    for (auto m : moves)
    {
        pos.undoMove();
    }

    REQUIRE(Position::StartPosFEN == pos.fen());
}