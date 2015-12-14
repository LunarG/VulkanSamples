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

#elif defined(VK_USE_PLATFORM_ANDROID_KHR)

#include <android/log.h>
#include "ShellAndroid.h"

void android_main(android_app *app)
{
    Game *game = create_game(0, nullptr);

    try {
        ShellAndroid shell(*app, *game);
        shell.run();
    } catch (const std::runtime_error &e) {
        __android_log_print(ANDROID_LOG_ERROR, game->settings().name.c_str(),
                "%s", e.what());
    }

    delete game;
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
