#pragma once

#include <engine/position/position.h>

// Refractor into own namespace Evaluation when evaluation grows larger
namespace Bratwurst::Search
{

constexpr int StaleMate = 0;
constexpr int CheckMate = 67000;
constexpr int Infinity = 1000000;

// evaluates position relative to the color whose turn it is
inline int evaluate(const Position& pos)
{
	// TODO: check for checkmate

	Color c = pos.colorToMove();
	int eval = 0;

	// add white material
	eval += popCnt(pos.pieceBB(White, Pawn)) * 100;
	eval += popCnt(pos.pieceBB(White, Knight)) * 300;
	eval += popCnt(pos.pieceBB(White, Bishop)) * 310;
	eval += popCnt(pos.pieceBB(White, Rook)) * 500;
	eval += popCnt(pos.pieceBB(White, Queen)) * 900;

	// subtract black material
	eval -= popCnt(pos.pieceBB(Black, Pawn)) * 100;
	eval -= popCnt(pos.pieceBB(Black, Knight)) * 300;
	eval -= popCnt(pos.pieceBB(Black, Bishop)) * 310;
	eval -= popCnt(pos.pieceBB(Black, Rook)) * 500;
	eval -= popCnt(pos.pieceBB(Black, Queen)) * 900;

	return (c == White) ? eval : -eval;
}

}