#include <string>
#include <vector>

#include "Hologram.h"

#ifdef _WIN32
#include "ShellWin32.h"
#else
#include "ShellXCB.h"
#endif

namespace {

Game *create_game(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);
    Hologram hologram(args);

#ifdef _WIN32
    ShellWin32 shell(hologram);
#else
    ShellXCB shell(hologram);
#endif

    shell.run();

    return 0;
}

#endif // VK_USE_PLATFORM_XCB_KHR
