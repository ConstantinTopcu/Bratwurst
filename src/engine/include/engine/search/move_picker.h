#include <engine/search/evaluation_constants.h>
#include <engine/position/position.h>
#include <engine/types/move.h>

namespace Bratwurst::Search
{

int score_move(const Position& pos, Move move);

// Helper for efficient lazy select for from MoveList
class MovePicker
{
public:
	inline MovePicker(const Position& pos, MoveList& moves) noexcept
		: m_moves(moves)
		, m_index(0)
	{
		for (int i = 0; i < moves.size(); i++)
		{
			m_evals[i] = score_move(pos, moves[i]);
		}
	}

	inline Move pick() noexcept
	{
		int bestHeuristicIndex = m_index;

		for (int i = m_index; i < m_moves.size(); i++)
		{
			if (m_evals[i] > m_evals[bestHeuristicIndex])
			{
				bestHeuristicIndex = i;
			}
		}

		std::swap(m_moves[bestHeuristicIndex], m_moves[m_index]);
		std::swap(m_evals[bestHeuristicIndex], m_evals[m_index]);

		return m_moves[m_index++];
	}

	inline bool hasNext() noexcept
	{
		return m_index < m_moves.size();
	}

private:
	MoveList& m_moves;
	int m_evals[MaxMoves];
	int m_index;
};

}