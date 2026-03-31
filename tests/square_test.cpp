#include <catch2/catch_all.hpp>
#include <engine/types/square.h>

using namespace Bratwurst;

TEST_CASE("Rank and File extraction from Square")
{
    SECTION("rankOf function")
    {
        REQUIRE(rankOf(A1) == Rank1);
        REQUIRE(rankOf(H1) == Rank1);
        REQUIRE(rankOf(A2) == Rank2);
        REQUIRE(rankOf(H8) == Rank8);
    }

    SECTION("fileOf function")
    {
        REQUIRE(fileOf(A1) == FileA);
        REQUIRE(fileOf(H1) == FileH);
        REQUIRE(fileOf(D4) == FileD);
        REQUIRE(fileOf(G7) == FileG);
    }
}

TEST_CASE("Square construction")
{
    SECTION("makeSquare function")
    {
        REQUIRE(makeSquare(FileA, Rank1) == A1);
        REQUIRE(makeSquare(FileH, Rank1) == H1);
        REQUIRE(makeSquare(FileA, Rank8) == A8);
        REQUIRE(makeSquare(FileH, Rank8) == H8);
        REQUIRE(makeSquare(FileD, Rank4) == D4);
    }

    SECTION("Round-trip consistency")
    {
        for (int r = Rank1; r < RankNum; ++r)
        {
            for (int f = FileA; f < FileNum; ++f)
            {
                Square s = makeSquare(static_cast<File>(f), static_cast<Rank>(r));
                REQUIRE(fileOf(s) == f);
                REQUIRE(rankOf(s) == r);
            }
        }
    }
}

TEST_CASE("String to Square conversion")
{
    SECTION("Valid square strings")
    {
        REQUIRE(stringToSquare("a1") == A1);
        REQUIRE(stringToSquare("h8") == H8);
        REQUIRE(stringToSquare("d4") == D4);
        REQUIRE(stringToSquare("e5") == E5);
    }
    SECTION("Invalid square strings")
    {
        REQUIRE(stringToSquare("-") == NoneSquare);
        REQUIRE(stringToSquare("") == NoneSquare);
        REQUIRE(stringToSquare("i1") == NoneSquare);
        REQUIRE(stringToSquare("a9") == NoneSquare);
        REQUIRE(stringToSquare("abc") == NoneSquare);
    }
}

TEST_CASE("Square to String conversion")
{
    SECTION("Invalid squares")
    {
        REQUIRE(squareToString(NoneSquare) == "-");
    }
    SECTION("Round-trip consistency")
    {
        REQUIRE(stringToSquare(squareToString(A1)) == A1);
        REQUIRE(stringToSquare(squareToString(H8)) == H8);
        REQUIRE(squareToString(stringToSquare("d4")) == "d4");
        REQUIRE(squareToString(stringToSquare("e5")) == "e5");
    }
}