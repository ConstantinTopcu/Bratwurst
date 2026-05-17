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

	struct TTEntry
	{
		Zobrist::Key key = 0; // exact key of the position
		Move bestMove = Move::Null(); // best move that was found in the position (might not be best move if beta cuttoff accurred)
		int score = 0; // score of the position
		MoveBound bound = MoveBound::None; // importent for verifying position and interpreting the move evaluation
		int depth = 0; // the depth the current position was evaluated at (maxDepth - ply)
		int generation = 0; // for aging entries and prefering newer ones over older ones
	};

	// size in mega-bytes
	TranspositionTable(size_t sizeMB)
	{
		size_t bytes = (sizeMB * 1024 * 1024);
		size_t rawEntryCnt = bytes / sizeof(TTEntry);
		size_t entryCnt = 1ULL << msb(rawEntryCnt);

		m_table = new TTEntry[entryCnt]();
		m_mask = entryCnt - 1;
	}

	~TranspositionTable()
	{
		delete[] m_table;
	}

	inline void startNewSearch()
	{
		currentGeneration++;
	}

	inline void store(TTEntry&& entry) noexcept
	{
		// replacement mainly based on depth and bound type
		size_t index = entry.key & m_mask;
		TTEntry& slot = m_table[index];
		entry.generation = currentGeneration;

		// same position
		if (slot.key == entry.key)
		{
			bool incomingExact	= entry.bound == MoveBound::Exact;
			bool existingExact	= slot.bound == MoveBound::Exact;
			bool incomingDeeper = entry.depth >= slot.depth;

			bool shouldReplace = incomingDeeper || (incomingExact && !existingExact);

			if (!shouldReplace) return;

			// preserve best move if new Entry doesnt have one
			if (entry.bestMove == Move::Null()) entry.bestMove = slot.bestMove;
		}

		// different position
		else if (replacementScore(entry) <= replacementScore(slot))
		{
			return;
		}

		slot = std::move(entry);
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
	int currentGeneration = 0;

private:

	int replacementScore(const TTEntry& e) const noexcept
	{
		int score = e.depth * 4;
		if (e.bound == MoveBound::Exact) score += 6;
		else if (e.bound == MoveBound::Lower) score += 3;
		score -= (currentGeneration - e.generation) * 2;

		return score;
	}
};

}