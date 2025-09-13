#include <precomputed.h>
#include <move_gen.h>

#include <iostream>
#include <chrono>

using namespace Bratwurst;

// returns amound of nodes
void perft(Position& pos, int depth, size_t& nodes, bool print = false)
{
	if (depth == 1)
	{
		MoveList moves;
		generateMoves<GenType::All>(pos, moves);
		nodes += moves.size();
		return;
	}

	MoveList moves;
	generateMoves<GenType::All>(pos, moves);

	for (Move m : moves)
	{
		size_t tempNodes = nodes;

		pos.doMove(m);
		perft(pos, depth - 1, nodes);
		pos.undoMove();

		if (print) std::cout << m.toString() << ": " << nodes-tempNodes << "\n";
	}
}


int main(char argc, char* argv[])
{
	Precomputed::init();

	Position pos = Position::fromFEN("r2q1n1k/pppb1ppp/2n5/7Q/8/2P5/PP1P2PP/KB2RR2 w - - 0 1").value();

	using Clock = std::chrono::high_resolution_clock;
	auto start = Clock::now();
	
	size_t nodes = 0;
	perft(pos, 6, nodes, true);

	if (nodes != 4470850742) DebugBreak();

	auto end = Clock::now();
	std::chrono::duration<double, std::ratio<1, 1>> time = end - start;
	std::cout << "reached " << nodes << " nodes in " << time << "." << std::endl;

	Precomputed::cleanup();
}