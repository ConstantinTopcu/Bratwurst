#include "precomputed.h"
#include "attacks.h"

#include "magic_gen.h"

#include <atomic>
#include <chrono>
#include <iostream>

namespace Bratwurst::Precomputed
{

// precomputed tables definition

Magic magics[2][SquareNum];
Bitboard pseudoAttacks[PieceTypeNum + 1][SquareNum];
static std::atomic_bool initialized = false;

// piece offsets
constexpr int whitePawnOffsets[2][2] = { {-1, 1}, {1, 1} };
constexpr int blackPawnOffsets[2][2] = { {-1, -1}, {1, -1} };
constexpr int knightOffsets[8][2] = { {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2} };
constexpr int kingOffsets[8][2] = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1}, {1, 0}, {0, 1}, {-1, 0}, {0, -1} };

// piece directions
constexpr int bishopDirections[4][2] = { {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };
constexpr int rookDirections[4][2] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1} };
constexpr int queenDirections[8][2] = { {1, 0}, {0, 1}, {-1, 0}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1} };

constexpr Bitboard precomputedBishopMagics[SquareNum] =
{
	0x4010523004008014ULL,	0x120410a204410800ULL,	0x12808084080aa08ULL,	0x8185100003120ULL,		0x4052120000080ULL,		0x8000862020040280ULL,	0x1004010110129010ULL,	0xc00210410090808ULL,
	0x8108281010028500ULL,	0x228180800c840ULL,		0x8010080495020000ULL,	0x4108a0a02088800ULL,	0x401041420003000ULL,	0x100808220200800ULL,	0x510205880948a002ULL,	0x400002086186000ULL,
	0x20ac12046100602ULL,	0x1208644410128200ULL,	0x110000504012240ULL,	0x1038048104370000ULL,	0x42d018090400000ULL,	0x442010901008268ULL,	0x254700882101000ULL,	0x10210852a08a3008ULL,
	0x80800600a9004ULL,		0x320003002024aULL,		0x90a0221030008200ULL,	0x3002002008008021ULL,	0x2011010040504006ULL,	0x1020081081125ULL,		0xa428200c1080200ULL,	0x1084009004259400ULL,
	0x8401208a00105000ULL,	0x8011002180200ULL,		0x2018200104800ULL,		0x8200801210114ULL,		0x88002c00044100ULL,	0x4010030200084060ULL,	0x81100080a4121200ULL,	0x8404018200042310ULL,
	0x408c20820044108ULL,	0x180820820010610ULL,	0xa102002038000400ULL,	0x44208000281ULL,		0x210204100c010080ULL,	0x3020800400604ULL,		0x149000a2008100ULL,	0x1090108901000040ULL,
	0x2008220104001ULL,		0x1002030401449910ULL,	0x8810820201048000ULL,	0x80100a094140400ULL,	0xa004012820800ULL,		0x500266a810210000ULL,	0x9d4108411040010ULL,	0x50810808868000ULL,
	0x2808818110432ULL,		0x44804044242040ULL,	0x8480200a11420800ULL,	0x1100000400840422ULL,	0x4010c450402ULL,		0xc4088080320ULL,		0x10002004c4208408ULL,	0x5588627002022102ULL
};

constexpr Bitboard precomputedRookMagics[SquareNum] =
{
	0x4800030a1400380ULL,	0x4c0001008c02004ULL,	0x280200080100148ULL,	0x3280180010008044ULL,	0x100040800310042ULL,	0x300010002880400ULL,	0x20800b0002000080ULL,	0x200028044260104ULL,
	0x92800040088820ULL,	0x2804004600080ULL,		0x400805000802000ULL,	0x20a1801002800802ULL,	0x40ec800800040280ULL,	0xe000200240810ULL,		0x910c803200800500ULL,	0x2102000080590402ULL,
	0xa8018c002200040ULL,	0x830084008200140ULL,	0x8000120020814200ULL,	0x102808008009000ULL,	0x1000818044001800ULL,	0x200804400800aULL,		0xa0040042280510ULL,	0x131a000120c884ULL,
	0x80400080042088ULL,	0xc00600040100040ULL,	0xa184300480200080ULL,	0x100300282100ULL,		0x81050100100800ULL,	0x11420080040080ULL,	0x4a10080800200ULL,		0x1000918a0003006cULL,
	0x4064400088800728ULL,	0x101008429004000ULL,	0x101801000802000ULL,	0x1d00412202001049ULL,	0x2000041101001800ULL,	0x600800400802200ULL,	0x2202001802004401ULL,	0x20010142001084ULL,
	0x3018812040088000ULL,	0x32200450044000ULL,	0xe210088420008010ULL,	0x1000094200220010ULL,	0x24001800808004ULL,	0x8092001014060008ULL,	0x806000100404080ULL,	0x200016300820004ULL,
	0x800a002441018a00ULL,	0x460084010002040ULL,	0x6911822000100080ULL,	0x1a0110402200ULL,		0x3180040080680080ULL,	0x207008400380300ULL,	0x1080110020400ULL,		0x1001001448920100ULL,
	0x5022800011024025ULL,	0x8840804001041021ULL,	0x409219002001ULL,		0x810010004a029ULL,		0x602001804106102ULL,	0x102000108141012ULL,	0x880020100d00804ULL,	0x1000010034004882ULL
};

// this function uses a shift value of 64 - popCnt(relevantBlockerMask)
Magic initMagic(Square s, std::span<const int[2]> directions, Bitboard magicBB)
{
	Magic magic = {};

	magic.magic = magicBB;
	magic.mask = dynamicAttacks<true, true>(s, directions);
	int relevantBlockerCnt = popCnt(magic.mask);
	magic.shift = 64 - relevantBlockerCnt;

	size_t attacksCnt = 1ULL << relevantBlockerCnt;
	magic.attacksTable = new Bitboard[attacksCnt];
	std::memset(magic.attacksTable, 0ULL, sizeof(Bitboard) * attacksCnt);

	for (size_t i = 0; i < attacksCnt; i++)
	{
		// Generate occupancy that is unique even after applying & magic.mask.
		Bitboard occupancy = 0ULL;
		Bitboard tempMask = magic.mask;
		size_t tempI = i;

		while (tempMask)
		{
			Square s = popLsb(tempMask);
			occupancy |= (tempI & 1) << s;
			tempI >>= 1;
		}

		Bitboard attacks = dynamicAttacks<true, false>(s, directions, occupancy);
		size_t index = magic.index(occupancy);

		ASSERT(magic.attacksTable[index] == 0ULL | magic.attacksTable[index] == attacks);

		magic.attacksTable[index] = attacks;
	}

	return magic;
}


void init() noexcept
{
	if (initialized) return;

	using clock = std::chrono::high_resolution_clock;
	auto start = clock::now();

	for (Square s = A1; s < SquareNum; s++)
	{ 
		//init magics
		magics[0][s] = initMagic(s, bishopDirections, precomputedBishopMagics[s]);
		magics[1][s] = initMagic(s, rookDirections, precomputedRookMagics[s]);

		//init pseudo attacks
		pseudoAttacks[WhitePawn][s] = dynamicAttacks(s, whitePawnOffsets);
		pseudoAttacks[BlackPawn][s] = dynamicAttacks(s, blackPawnOffsets);
		pseudoAttacks[Knight][s] = dynamicAttacks(s, knightOffsets);
		pseudoAttacks[Bishop][s] = dynamicAttacks<true>(s, bishopDirections);
		pseudoAttacks[Rook][s] = dynamicAttacks<true>(s, rookDirections);
		pseudoAttacks[Queen][s] = pseudoAttacks[Bishop][s] | pseudoAttacks[Rook][s];
		pseudoAttacks[King][s] = dynamicAttacks(s, kingOffsets);
	}

	auto end = clock::now();
	std::chrono::duration<double, std::milli> duration = end - start;
	initialized = true;
}

void cleanup() noexcept
{
	if (!initialized) return;

	// cleanup all heap allocated memory
	for (Square s = A1; s < SquareNum; s++)
	{
		if (magics[0][s].attacksTable != nullptr) delete[] magics[0][s].attacksTable;
		if (magics[1][s].attacksTable != nullptr) delete[] magics[1][s].attacksTable;
	}

	initialized = false;
}

}