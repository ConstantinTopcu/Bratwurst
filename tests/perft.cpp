#include <catch2/catch_all.hpp>

#include <engine/testing/perft.h>

struct PerftCase
{
    std::string fen;
    std::vector<uint64_t> expectedNodes; // index 0 = depth 1
    int depth;
};

static const std::vector<PerftCase> cases =
{
    {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        {20, 400, 8902, 197281, 4865609, 119060324}, 6
    },
    {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        {48, 2039, 97862, 4085603, 193690690, 8031647685}, 6
    },
    {
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        {14, 191, 2812, 43238, 674624, 11030083, 178633661, 3009794393}, 8
    },
    {
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        {6, 264, 9467, 422333, 15833292, 706045033}, 6
    },
    {
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        {44, 1486, 62379, 2103487, 89941194}, 5
    },
    {
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        {46, 2079, 89890, 3894594, 164075551, 6923051137}, 6
    }
};

TEST_CASE("perft suite", "[movegen]")
{
    using namespace Bratwurst;

    Zobrist::init();
    Precomputed::init();
    Evaluation::init();

    for (auto& [fen, expectedNodesV, maxDepth] : cases)
    {
        INFO("FEN: " << fen);

        for (int depth = 1; depth <= maxDepth; depth++)
        {
            INFO("Depth: " << depth);

            // load position
            auto result = Position::fromFEN(fen);
            REQUIRE(result.has_value());
            Position pos = result.value();

            // compare perft results
            uint64_t nodes = Perft::perft(pos, depth);
            uint64_t expectedNodes = expectedNodesV[depth - 1];
            REQUIRE(nodes == expectedNodes);
        }
    }

    Precomputed::cleanup();
}