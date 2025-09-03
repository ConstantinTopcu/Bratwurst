#include <catch2/catch_all.hpp>
#include <types/square.h>

using namespace Bratwurst;

TEST_CASE("Square, Rank, and File validation")
{
    SECTION("Valid Squares")
    {
        REQUIRE(isValid(A1));
        REQUIRE(isValid(H8));
    }

    SECTION("Invalid Square")
    {
        REQUIRE(!isValid(NoneSquare));
    }

    SECTION("Valid Files")
    {
        REQUIRE(isValid(FileA));
        REQUIRE(isValid(FileH));
    }

    SECTION("Invalid File")
    {
        REQUIRE(!isValid(NoneFile));
    }

    SECTION("Valid Ranks")
    {
        REQUIRE(isValid(Rank1));
        REQUIRE(isValid(Rank8));
    }

    SECTION("Invalid Rank")
    {
        REQUIRE(!isValid(NoneRank));
    }
}

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

TEST_CASE("Direction multiplication operators")
{
    SECTION("Multiplication with Up/Down/Left/Right")
    {
        Direction d = Direction::Up;

        REQUIRE(static_cast<int>(2 * d) == static_cast<int>(d) * 2);

        d *= 3;
        REQUIRE(static_cast<int>(d) == static_cast<int>(Direction::Up) * 3);
    }
}

TEST_CASE("Square + Direction operators")
{
    Square s = A1;

    SECTION("Functional + operator")
    {
        Square result = s + Direction::Up;
        REQUIRE(static_cast<int>(result) == static_cast<int>(s) + static_cast<int>(Direction::Up));
    }

    SECTION("In-place += operator")
    {
        s += Direction::Right;
        REQUIRE(static_cast<int>(s) == static_cast<int>(A1) + static_cast<int>(Direction::Right));
    }

    SECTION("Multiple additions")
    {
        s = D4;
        s += Direction::Up;
        s += Direction::Right;
        REQUIRE(static_cast<int>(s) == static_cast<int>(D4) + static_cast<int>(Direction::Up) + static_cast<int>(Direction::Right));
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
    SECTION("Valid squares")
    {
        REQUIRE(squareToString(A1) == "a1");
        REQUIRE(squareToString(H8) == "h8");
        REQUIRE(squareToString(D4) == "d4");
        REQUIRE(squareToString(E5) == "e5");
    }
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