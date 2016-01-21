#include <string>
#include <vector>

#include "Hologram.h"

namespace {

Game *create_game(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);
    return new Hologram(args);
}

} // namespace

#if defined(VK_USE_PLATFORM_XCB_KHR)

#include "ShellXcb.h"

int main(int argc, char **argv)
{
    Game *game = create_game(argc, argv);
    {
        ShellXcb shell(*game);
        shell.run();
    }
    delete game;

    return 0;
}

#elif defined(VK_USE_PLATFORM_WIN32_KHR)

#include "ShellWin32.h"

int main(int argc, char **argv)
{
    Game *game = create_game(argc, argv);
    {
        ShellWin32 shell(*game);
        shell.run();
    }
    delete game;

    return 0;
}

#endif // VK_USE_PLATFORM_XCB_KHR
