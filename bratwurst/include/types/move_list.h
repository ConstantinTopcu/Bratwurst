#pragma once

#include "core.h"

#include "types/move.h"

namespace Bratwurst
{

class MoveList
{

public:
	constexpr MoveList() : m_size(0) {}
	constexpr ~MoveList() = default;

	constexpr void add(Move move) noexcept;
	constexpr void emplace(Square s1, Square s2, Move::Flag flag) noexcept;
	constexpr void remove(uint8 i) noexcept;

	constexpr Move get(uint8 i) const noexcept;
	constexpr uint8 size() const noexcept;
	constexpr Move* begin() noexcept;
	constexpr Move* end() noexcept;
	constexpr const Move* begin() const noexcept;
	constexpr const Move* end() const noexcept;
	constexpr Move& operator[](uint8 i) noexcept;
	constexpr const Move& operator[](uint8 i) const noexcept;

private:
	// In each position there is a maximum of 218 legal moves
	static constexpr uint8 MaxMoves = 218;
	Move m_moves[218];
	uint8 m_size;
};

constexpr void MoveList::add(Move move) noexcept
{
	ASSERT(m_size < MaxMoves);
	m_moves[m_size++] = move;
}

constexpr void MoveList::emplace(Square s1, Square s2, Move::Flag flag) noexcept
{
	ASSERT(m_size < MaxMoves);
	m_moves[m_size++] = Move(s1, s2, flag);
}

constexpr void MoveList::remove(uint8 i) noexcept
{
	ASSERT(i < m_size);
	m_moves[i] = m_moves[--m_size];
}

constexpr Move MoveList::get(uint8 i) const noexcept
{
	ASSERT(i < m_size);
	return m_moves[i];
}
constexpr uint8 MoveList::size() const noexcept
{
	return m_size;
}

constexpr Move* MoveList::begin() noexcept
{
	return m_moves;
}

constexpr Move* MoveList::end() noexcept
{
	ASSERT(m_size <= MaxMoves);
	return m_moves + m_size;
}

constexpr const Move* MoveList::begin() const noexcept
{
	return m_moves;
}

constexpr const Move* MoveList::end() const noexcept
{
	ASSERT(m_size <= MaxMoves);
	return m_moves + m_size;
}

constexpr Move& MoveList::operator[](uint8 i) noexcept
{
	ASSERT(i < m_size);
	return m_moves[i];
}
constexpr const Move& MoveList::operator[](uint8 i) const noexcept
{
	ASSERT(i < m_size);
	return m_moves[i];
}

}