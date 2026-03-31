#include <catch2/catch_all.hpp>
#include <engine/position.h>

using namespace Bratwurst;

// Global fixture for Precomputed
struct PrecomputedFixture 
{
    PrecomputedFixture() { Precomputed::init(); }
    ~PrecomputedFixture() { Precomputed::cleanup(); }
};

// Register fixture so it runs once
static PrecomputedFixture globalPrecomputed;

TEST_CASE("check valid FENs")
{
    std::string validFens[10] =
    {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b - - 0 1",
        "8/8/8/8/4k3/8/8/4K3 w - - 5 42",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq e3 0 2",
        "r3k2r/8/8/8/8/8/8/R3K2R w K - 12 34",
        "r3k2r/8/8/8/8/8/8/R3K2R b q - 7 55",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 3 10",
        "rnbqkb1r/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e6 0 2",
        "k7/7K/8/3pP3/8/8/8/8 w - d6 0 15",
        "r1bq1rk1/pppp1ppp/2n2n2/2b1p3/4P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 6 10"
    };

    for (const std::string& validFen : validFens)
    {
        auto result = Position::fromFEN(validFen);
        REQUIRE(result.has_value());
        REQUIRE(validFen == result->fen());
    }
}

TEST_CASE("check invalid FENs")
{
    std::pair<std::string, Position::FenError> invalidFens[10] =
    {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0", Position::FenError::InvalidFormat},
        {"9/8/8/8/8/8/8/8 w - - 0 1", Position::FenError::InvalidPiecePlacement},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x - - 0 1", Position::FenError::InvalidColorToMove},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w A - 0 1", Position::FenError::InvalidCastlingRights},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w -z - 0 1", Position::FenError::InvalidCastlingRights},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - i8 0 1", Position::FenError::InvalidEnPassantSquare},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - -1 1", Position::FenError::InvalidHalfmoveClock},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 51 1", Position::FenError::InvalidHalfmoveClock},
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 -1", Position::FenError::InvalidFullMoveCounter},
        {"rnbqkbnrr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1", Position::FenError::InvalidPiecePlacement},
    };

    for (const auto& [fen, error] : invalidFens)
    {
        auto result = Position::fromFEN(fen);
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == error);
    }
}

TEST_CASE("check doMove/undoMove consistency")
{
    auto result = Position::fromFEN(std::string(Position::StartPosFEN));
    Position pos = result.value();

    Move moves[] =
    {
        Move(E2, E4), Move(E7, E5), Move(G1, F3),
        Move(G8, G6), Move(F1, E2),Move(F8, E7),
        Move(E1, G1, Move::Flag::CastlingOO),
        Move(E8, G8, Move::Flag::CastlingOO),
        Move(D2, D4), Move(E5, D4), Move(C2, C4),
        Move(D4, C3, Move::Flag::EnPassant),
        Move(B2, B4), Move(C7, C5), Move(B4, C5), 
        Move(B7, B6), Move(C5, B6), Move(H7, H6),
        Move(B6, B7), Move(B8, C6), Move(B7, A8, 
        Move::Flag::QueenPromotion)
    };

    for (auto m : moves) 
        pos.doMove(m);
    
    for (auto m : moves) 
        pos.undoMove();

    REQUIRE(Position::StartPosFEN == pos.fen());
}

TEST_CASE("check pins and checks")
{
    // random attacker fen:
    Position pos = Position::fromFEN("rnbqkbnr/ppp2pp1/3p3p/4p3/2B1P3/5Q2/PPPP1PPP/RNB1K1NR b KQkq - 0 4").value();
    Bitboard attackers = pos.attackers(F7, White, pos.occupancyBB());
    REQUIRE(attackers == (squareMask(F3) | squareMask(C4)));

    // pinned fen
    Position pinnedPos = Position::fromFEN("rnbqk1nr/pppp1ppp/8/4p3/1b2P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3").value();
    Bitboard pinned = pinnedPos.pinned();
    REQUIRE(pinned == squareMask(D2));

    // white checked fen
    Position checkersPos = Position::fromFEN("r1bqk1nr/pppp1ppp/2n5/4p3/1b2P3/3P1N2/PPP2PPP/RNBQKB1R w KQkq - 1 4").value();
    Bitboard checkers = checkersPos.checkers();
    REQUIRE(checkers == squareMask(B4));
}
