#include <engine/uci/uci.h>

#include <engine/position/position.h>
#include <engine/search/search.h>
#include <engine/types/move.h>
#include <engine/types/zobrist.h>
#include <engine/testing/perft.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

//primitive AI Uci implementation to get start test-machtes
namespace Bratwurst::UCI
{

	static Position pos;

	static std::vector<std::string> split(const std::string& line)
	{
		std::istringstream ss(line);
		std::vector<std::string> tokens;
		std::string token;

		while (ss >> token) tokens.push_back(token);

		return tokens;
	}

	static void setPosition(const std::vector<std::string>& tokens)
	{
		size_t i = 1;

		if (tokens[i] == "startpos")
		{
			pos = Position::fromFEN().value();
			i++;
		}

		// load fen
		else if (tokens[i] == "fen")
		{
			i++;
			std::string fen;
			while (i < tokens.size() && tokens[i] != "moves") fen += tokens[i++] + " ";
			pos = Position::fromFEN(fen).value();
		}

		// apply moves
		if (i < tokens.size() && tokens[i] == "moves")
		{
			i++;
			for (; i < tokens.size(); i++) pos.doMove(Move::fromString(tokens[i], pos));
		}
	}

	static void go(const std::vector<std::string>& tokens)
	{
		int perftDepth = -1;
		Search::TimeLimit time = { 0, 0, 0};
		Color c = pos.colorToMove();

		for (size_t i = 1; i < tokens.size(); i++)
		{
			if		(tokens[i] == "perft" && i + 1 < tokens.size())					perftDepth = std::stoi(tokens[++i]);
			else if (tokens[i] == "movetime" && i + 1 < tokens.size())				time.msPerMove = std::stoi(tokens[++i]);
			else if (tokens[i] == "wtime" && i + 1 < tokens.size() && c == White)	time.msLeft = std::stoi(tokens[++i]);
			else if (tokens[i] == "btime" && i + 1 < tokens.size() && c == Black)   time.msLeft = std::stoi(tokens[++i]);
			else if (tokens[i] == "winc" && i + 1 < tokens.size() && c == White)    time.msIncr = std::stoi(tokens[++i]);
			else if (tokens[i] == "binc" && i + 1 < tokens.size() && c == Black)    time.msIncr = std::stoi(tokens[++i]);
		}

		if (perftDepth >= 0)
		{
			std::size_t nodes = Perft::perft(pos, perftDepth, true);
			std::cout << "nodes " << nodes << "\n";
			return;
		}

		auto result = Search::search(pos, time);
		std::cout << "bestmove " << result.bestMove.toString() << "\n";
	}

	void loop()
	{
		pos = Position::fromFEN().value();

		std::string line;

		while (std::getline(std::cin, line))
		{
			if (line.empty()) continue;

			auto tokens = split(line);

			if (tokens.empty()) continue;

			const std::string& cmd = tokens[0];

			if (cmd == "uci") { std::cout << "id name Bratwurst\nid author you\nuciok\n"; }
			else if (cmd == "isready") { std::cout << "readyok\n"; }
			else if (cmd == "ucinewgame") { pos = Position::fromFEN().value(); }
			else if (cmd == "position") { setPosition(tokens); }
			else if (cmd == "go") { go(tokens); }
			else if (cmd == "quit") { break; }
		}
	}

} // namespace Bratwurst::UCI
