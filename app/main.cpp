#include <engine/uci/uci.h>
#include <engine/move_gen/precomputed.h>
#include <engine/types/zobrist.h>

using namespace Bratwurst;

int main(int argc, char* argv[])
{
    Precomputed::init();
    Zobrist::init();

    UCI::loop();

    Precomputed::cleanup();
}