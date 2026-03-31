#include <catch2/catch_all.hpp>
#include <engine/types/move.h>

// Partially unnecessary

using namespace Bratwurst;

TEST_CASE("Move creation and field access", "[move]")
{

	Move m(Square::E2, Square::E4, Move::Flag::None);

	SECTION("Source and Destination squares")
	{
		REQUIRE(m.src() == E2);
		REQUIRE(m.dst() == E4);
	}

	SECTION("Flags")
	{
		REQUIRE(m.flag() == Move::Flag::None);
		REQUIRE(!m.special());
		REQUIRE(!m.castling());
		REQUIRE(!m.promotion());
	}
}

TEST_CASE("Move flags behavior", "[move][flags]") 
{
	Move epMove(Square::E5, Square::D6, Move::Flag::EnPassant);
	REQUIRE(epMove.enPassant());
	REQUIRE(epMove.special());
	REQUIRE(!epMove.castling());
	REQUIRE(!epMove.promotion());  

	Move castleOO(Square::E1, Square::G1, Move::Flag::CastlingOO);
	REQUIRE(castleOO.special());
	REQUIRE(castleOO.castling());
	REQUIRE(castleOO.castlingOO());
	REQUIRE(!castleOO.enPassant());
	REQUIRE(!castleOO.promotion());

	Move promoMove(Square::E7, Square::E8, Move::Flag::QueenPromotion);
	REQUIRE(promoMove.special());
	REQUIRE(promoMove.promotion());
	REQUIRE(promoMove.promotionType() == PieceType::Queen);
	REQUIRE(!promoMove.enPassant());
	REQUIRE(!promoMove.castling());
}
