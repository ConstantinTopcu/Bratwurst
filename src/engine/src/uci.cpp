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

	static void debugPosition()
	{
		std::cout << "info string castling_rights:"
			<< " wK=" << pos.hasCastlingRight(WhiteOO)
			<< " wQ=" << pos.hasCastlingRight(WhiteOOO)
			<< " bK=" << pos.hasCastlingRight(BlackOO)
			<< " bQ=" << pos.hasCastlingRight(BlackOOO)
			<< "\n";
		std::cout.flush();
	}

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
		int movetime = -1;
		int perftDepth = -1;
		int wtime = -1, btime = -1;
		int winc = 0, binc = 0;

		for (size_t i = 1; i < tokens.size(); i++)
		{
			if (tokens[i] == "perft" && i + 1 < tokens.size()) perftDepth = std::stoi(tokens[++i]);
			else if (tokens[i] == "movetime" && i + 1 < tokens.size()) movetime = std::stoi(tokens[++i]);
			else if (tokens[i] == "wtime" && i + 1 < tokens.size())    wtime = std::stoi(tokens[++i]);
			else if (tokens[i] == "btime" && i + 1 < tokens.size())    btime = std::stoi(tokens[++i]);
			else if (tokens[i] == "winc" && i + 1 < tokens.size())     winc = std::stoi(tokens[++i]);
			else if (tokens[i] == "binc" && i + 1 < tokens.size())     binc = std::stoi(tokens[++i]);
		}

		if (perftDepth >= 0)
		{
			std::size_t nodes = Perft::perft(pos, perftDepth, true);
			std::cout << "nodes " << nodes << "\n";
			return;
		}

		if (movetime == -1)
		{
			int time = (pos.colorToMove() == White) ? wtime : btime;
			int inc = (pos.colorToMove() == White) ? winc : binc;
			movetime = time / 20 + inc; // naive time management
		}

		debugPosition();

		auto result = Search::search(pos, movetime);
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
