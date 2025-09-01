#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <types/square.h>

using namespace Bratwurst;

TEST_CASE("Square validation")
{
    REQUIRE(isValid(A1));
    REQUIRE(isValid(H8));
    REQUIRE(!isValid(NoneSquare));
}

TEST_CASE("File validation")
{
    REQUIRE(isValid(FileA));
    REQUIRE(isValid(FileH));
    REQUIRE(!isValid(NoneFile));
}

TEST_CASE("Rank validation")
{
    REQUIRE(isValid(Rank1));
    REQUIRE(isValid(Rank8));
    REQUIRE(!isValid(NoneRank));
}

TEST_CASE("RankOf function")
{
    REQUIRE(rankOf(A1) == Rank1);
    REQUIRE(rankOf(H1) == Rank1);
    REQUIRE(rankOf(A2) == Rank2);
    REQUIRE(rankOf(H8) == Rank8);
}

TEST_CASE("FileOf function")
{
    REQUIRE(fileOf(A1) == FileA);
    REQUIRE(fileOf(H1) == FileH);
    REQUIRE(fileOf(D4) == FileD);
    REQUIRE(fileOf(G7) == FileG);
}

TEST_CASE("MakeSquare function")
{
    REQUIRE(makeSquare(FileA, Rank1) == A1);
    REQUIRE(makeSquare(FileH, Rank1) == H1);
    REQUIRE(makeSquare(FileA, Rank8) == A8);
    REQUIRE(makeSquare(FileH, Rank8) == H8);
    REQUIRE(makeSquare(FileD, Rank4) == D4);
}

TEST_CASE("Square Make And Extract")
{
    // Full round-trip consistency for all squares
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
