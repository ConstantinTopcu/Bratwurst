#include <precomputed.h>
#include <zobrist.h>
#include <position.h>

#include <iostream>

#include <search/search.h>

using namespace Bratwurst;

int main(char argc, char* argv[])
{
	Precomputed::init();
	Zobrist::init();

	Position pos = Position::fromFEN().value();

	for (int i = 0; i < 100; i++)
	{
		auto result = Search::search(pos, 6);
		Move bestMove = result.bestMove;
		pos.doMove(bestMove);
		
		std::cout << "[Material Info]" << " White: " << pos.material(White)
			<< " | Black: " << pos.material(Black)
			<< " | Diff: " << pos.material(White) - pos.material(Black)
			<< std::endl;

		std::cout << "[Search Info]"
			<< " bestMove: " << result.bestMove.toString()
			<< " | Eval: " << result.eval
			<< " | Time: " << result.timeNS/1000000 << "ms"
			<< " | Nodes: " << result.nodes
			<< " | NPS: " << std::fixed << result.nps
			<< std::endl;

		std::cout << pos << std::endl;
	}

	Precomputed::cleanup();
}