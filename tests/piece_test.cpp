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

TEST_CASE("Piece conversion functions")
{
    SECTION("Invalid piece to char")
    {
        REQUIRE(pieceToChar(NonePiece) == '.');
        REQUIRE(pieceToChar(static_cast<Piece>(99)) == '.');
    }

    SECTION("Invalid char to piece")
    {
        REQUIRE(charToPiece('.') == NonePiece);
        REQUIRE(charToPiece('X') == NonePiece);
        REQUIRE(charToPiece('1') == NonePiece);
        REQUIRE(charToPiece(' ') == NonePiece);
        REQUIRE(charToPiece('\0') == NonePiece);
    }

    SECTION("Round-trip consistency")
    {
        Piece pieces[] = {
            WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing,
            BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing
        };

        for (Piece piece : pieces) {
            char c = pieceToChar(piece);
            REQUIRE(c != '.');
            REQUIRE(charToPiece(c) == piece);
        }
    }

    SECTION("Case sensitivity matters")
    {
        REQUIRE(pieceToChar(WhiteKing) != pieceToChar(BlackKing));
        REQUIRE(charToPiece('K') != charToPiece('k'));

        REQUIRE(pieceToChar(WhiteKing) == 'K');
        REQUIRE(pieceToChar(BlackKing) == 'k');
    }
}