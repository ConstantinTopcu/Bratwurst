#pragma once

#include <engine/types/types.h>
#include <engine/core/core.h>

namespace Bratwurst::Evaluation
{

// constants
constexpr int StaleMate = 0;
constexpr int CheckMate = 67000;
constexpr int Infinity = 1000000;

// used to calculate game phase
inline constexpr int PiecePhaseValue[PieceNum] = { 0, 1, 1, 2, 4, 0, 0, 1, 1, 2, 4, 0 };

inline constexpr int MaxPhase =    16 * PiecePhaseValue[Pawn] + 
                                    4 * PiecePhaseValue[Knight] + 
                                    4 * PiecePhaseValue[Bishop] + 
                                    4 * PiecePhaseValue[Rook] + 
                                    2 * PiecePhaseValue[Queen];

constexpr int PawnValue = 100;
constexpr int KnightValue = 300;
constexpr int BishopValue = 310;
constexpr int RookValue = 500;
constexpr int QueenValue = 900;

inline constexpr int PieceValue[PieceNum] =
{
    PawnValue, KnightValue, BishopValue, RookValue, QueenValue, 0,
    PawnValue, KnightValue, BishopValue, RookValue, QueenValue, 0
};

constexpr int32 S(int16 mg, int16 eg) { return (int32(mg) << 16) | uint16(eg); }
constexpr int16 mg(int32 x) { return (int16_t)(x >> 16); }
constexpr int16 eg(int32 x) { return (int16)(x & 0xFFFF); }

extern int32 PSQT[PieceNum][SquareNum];
extern Bitboard passedPawnMasks[ColorNum][SquareNum];

void init();

}