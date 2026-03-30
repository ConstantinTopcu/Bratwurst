#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include <types/bitboard.h>

// Partially unnecessary

using namespace Bratwurst;

TEST_CASE("Bitboard squareMask") 
{
    REQUIRE(squareMask(A1) == 1ULL << A1);
    REQUIRE(squareMask(H8) == 1ULL << H8);
}

TEST_CASE("Bitboard rankMask") 
{
    REQUIRE(rankMask(Rank1) == 0xFFULL);
    REQUIRE(rankMask(Rank8) == (0xFFULL << (7 * FileNum)));
}

TEST_CASE("Bitboard fileMask") 
{
    REQUIRE(fileMask(FileA) == 0x0101010101010101ULL);
    REQUIRE(fileMask(FileH) == (0x0101010101010101ULL << 7));
}

TEST_CASE("Bitboard popCnt") 
{
    REQUIRE(popCnt(0ULL) == 0);
    REQUIRE(popCnt(0xFULL) == 4);
    REQUIRE(popCnt(0xFFFFFFFFFFFFFFFFULL) == 64);
}

TEST_CASE("Bitboard lsb/msb") 
{
    Bitboard bb = squareMask(C3) | squareMask(H8);
    REQUIRE(lsb(bb) == C3);
    REQUIRE(msb(bb) == H8);
}

TEST_CASE("Bitboard popLsb/popMsb") 
{
    Bitboard bb = squareMask(B2) | squareMask(G7);

    Square s1 = popLsb(bb);
    REQUIRE((s1 == B2 || s1 == G7));
    REQUIRE(popCnt(bb) == 1);

    Square s2 = popMsb(bb);
    REQUIRE((s2 == B2 || s2 == G7));
    REQUIRE(bb == 0ULL);
}

TEST_CASE("Bitboard randomBitboard") {
    Bitboard b1 = randomBitboard();
    Bitboard b2 = randomBitboard();
    REQUIRE(b1 != b2); // very unlikely to fail
}
