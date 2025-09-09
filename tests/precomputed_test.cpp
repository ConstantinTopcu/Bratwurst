#include <catch2/catch_all.hpp>
#include <precomputed.h>

using namespace Bratwurst;

TEST_CASE("validate precomputed data")
{
	Precomputed::init();

	// briefly validate magics
	REQUIRE(Bratwurst::Precomputed::magics[0][A1].attacks(0ULL) == 0x8040201008040200ULL);
	REQUIRE(Bratwurst::Precomputed::magics[0][A1].attacks(0xFF00ULL) == 0x200ULL);
	REQUIRE(Bratwurst::Precomputed::magics[1][A1].attacks(0ULL) == 0x1010101010101feULL);
	REQUIRE(Bratwurst::Precomputed::magics[1][A1].attacks(0xFF00ULL) == 0x1feULL);

	// briefly validate pseudo-attacks
	REQUIRE(Bratwurst::Precomputed::pseudoAttacks[WhitePawn][A1] == 0x200ULL);
	REQUIRE(Bratwurst::Precomputed::pseudoAttacks[Knight][A1] == 0x20400ULL);
	REQUIRE(Bratwurst::Precomputed::pseudoAttacks[Bishop][A1] == 0x8040201008040200ULL);
	REQUIRE(Bratwurst::Precomputed::pseudoAttacks[Rook][A1] == 0x1010101010101feULL);
	REQUIRE(Bratwurst::Precomputed::pseudoAttacks[Queen][A1] == 0x81412111090503feULL);
	REQUIRE(Bratwurst::Precomputed::pseudoAttacks[King][A1] == 0x302ULL);
	REQUIRE(Bratwurst::Precomputed::pseudoAttacks[BlackPawn][E4] == 0x280000ULL);
	
	Precomputed::cleanup();
}