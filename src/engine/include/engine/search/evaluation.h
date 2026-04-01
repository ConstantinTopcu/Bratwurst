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
	int eval = pos.material();
	return (c == White) ? eval : -eval;
}

}