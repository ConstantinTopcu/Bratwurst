#pragma once

#include <engine/types/types.h>

namespace Bratwurst::Search
{

class TranspositionTable
{
public:
	enum class MoveBound : uint8
	{
		Lower,
		Exact,
		Upper,
		None
	};

	// TODO: add good replacement strategy i.e. by adding age
	struct TTEntry
	{
		Zobrist::Key key = 0; // exact key of the position
		Move bestMove = Move::Null(); // best move that was found in the position (might not be best move if beta cuttoff accurred)
		int score = 0; // score of the position
		MoveBound bound = MoveBound::None; // importent for verifying position and interpreting the move evaluation
		int depth = 0; // the depth the current position was evaluated at (maxDepth - ply)
	};

	// size in mega-bytes
	TranspositionTable(size_t sizeMB)
	{
		size_t bytes = (sizeMB * 1024 * 1024);
		size_t rawEntryCnt = bytes / sizeof(TTEntry);
		size_t entryCnt = 1ULL << msb(rawEntryCnt);

		m_table = new TTEntry[entryCnt];
		m_mask = entryCnt - 1;

		// Info Log
		//std::cout << "Transposition successfully created (entries: " << entryCnt << "; size: " << (entryCnt * sizeof(TTEntry)) / (1024 * 1024) << " mb)" << std::endl;
	}


	inline void store(TTEntry&& entry) noexcept
	{
		size_t index = entry.key & m_mask;

		if (m_table[index].key == entry.key && entry.depth < m_table[index].depth) return;

		m_table[index] = entry;
	}

	inline const TTEntry* probe(Zobrist::Key key) noexcept
	{
		size_t index = key & m_mask;
		TTEntry& entry = m_table[index];

		// check for valid entry
		if (entry.key == key) return &entry;

		return nullptr;
	}

private:
	TTEntry* m_table;
	uint64 m_mask;
};

}