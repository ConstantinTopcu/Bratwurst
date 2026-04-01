#pragma once

#include <engine/position/position.h>

namespace Bratwurst::Search
{

struct SearchResult
{
	int evaluation;
	Move bestMove;
	int nodes;
};

SearchResult search(Position& position, int depth);

}