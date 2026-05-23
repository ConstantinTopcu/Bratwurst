#pragma once

#include <engine/position/position.h>

namespace Bratwurst::Search
{

struct SearchResult
{
	int evaluation;
	Move bestMove;
	size_t nodes;
	int depth; // the last depth level, that was fully searched
};

struct TimeLimit
{
	int msLeft;
	int msIncr;
	int msPerMove;
};

SearchResult search(Position& position, TimeLimit limit);

}