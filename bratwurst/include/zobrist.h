#pragma once

#include "core.h"

#include "types/bitboard.h"
#include "types/piece.h"
#include "types/square.h"

namespace Bratwurst::Zobrist
{
    // sometimes add +1, because its cheaper to do & 0ULL than checking for invalidity
    inline uint64 piece[PieceNum + 1][SquareNum]; // 12 pieces + NonePiece × 64 squares
    inline uint64 castling[16];                   // 16 castling states
    inline uint64 enPassant[FileNum + 1];         // 8 files
    inline uint64 side;                           // side to move

    using Key = uint64;

    inline void init()
    {
        for (Square s = A1; s < SquareNum; s++)
        {
            for (Piece p = WhitePawn; p < PieceNum; p++) piece[p][s] = randomBitboard();
            piece[NonePiece][s] = 0ULL;
        }

        for (int i = 0; i < 16; ++i)
            castling[i] = randomBitboard();

        for (File f = FileA; f < FileNum; f++)
            enPassant[f] = randomBitboard();

        enPassant[NoneFile] = 0ULL;

        side = randomBitboard();
    }
}
