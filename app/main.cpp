#include <engine/position/position.h>
#include <engine/testing/perft.h>

#include <iostream>

using namespace Bratwurst;

int main(char argc, char* argv[])
{
	Precomputed::init();
	Zobrist::init();

	Position pos = Position::fromFEN("r2q1n1k/pppb1ppp/2n5/7Q/8/2P5/PP1P2PP/KB2RR2 w - - 0 1").value();
	size_t perftResult = Perft::perft(pos, 5, true);
	std::cout << "Total Nodes: " << perftResult << std::endl;

	Precomputed::cleanup();
}