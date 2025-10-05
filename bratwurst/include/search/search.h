#pragma once

#include "types/move.h"

namespace Bratwurst
{

class Position;

namespace Search
{

struct SearchResult
{
	Move bestMove;
	int eval;
	uint8 depth;
	size_t nodes;
	uint32 nps;
	uint32 timeNS;
};

// might be replaced with time later
SearchResult search(Position& pos, uint8 depth);

}
}