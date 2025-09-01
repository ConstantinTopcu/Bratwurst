#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <types/piece.h>

using namespace Bratwurst;

TEST_CASE("Piece Validation")
{
    for (int p = WhitePawn; p < NonePiece; ++p)
    {
        REQUIRE(isValid(static_cast<Piece>(p)));
    }

    REQUIRE(!isValid(NonePiece));
}

TEST_CASE("PieceType Validation")
{
    for (int pt = Pawn; pt < NonePieceType; ++pt)
    {
        REQUIRE(isValid(static_cast<PieceType>(pt)));
    }

    REQUIRE(!isValid(NonePieceType));
}

TEST_CASE("Color Validation")
{
    for (int c = White; c < NoneColor; ++c)
    {
        REQUIRE(isValid(static_cast<Color>(c)));
    }

    REQUIRE(!isValid(NoneColor));
}

TEST_CASE("Piece Type Extraction")
{
    for (int p = WhitePawn; p < NonePiece; ++p)
    {
        Piece piece = static_cast<Piece>(p);
        PieceType expected = static_cast<PieceType>(p % PieceTypeNum);
        REQUIRE(pieceTypeOf(piece) == expected);
    }
}

TEST_CASE("Piece Color Extraction")
{
    for (int p = WhitePawn; p < NonePiece; ++p)
    {
        Piece piece = static_cast<Piece>(p);
        Color expected = (p >= BlackPawn) ? Black : White;
        REQUIRE(colorOf(piece) == expected);
    }
}

TEST_CASE("Piece Make And Extract")
{
    for (int c = White; c < ColorNum; ++c)
    {
        for (int pt = Pawn; pt < PieceTypeNum; ++pt)
        {
            Piece p = makePiece(static_cast<Color>(c), static_cast<PieceType>(pt));
            REQUIRE(pieceTypeOf(p) == pt);
            REQUIRE(colorOf(p) == c);
        }
    }
}