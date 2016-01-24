#include <string>
#include <vector>

#include "Hologram.h"

#ifdef _WIN32
#include "ShellWin32.h"
#else
#include "ShellXcb.h"
#endif

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);
    Hologram hologram(args);

#ifdef _WIN32
    ShellWin32 shell(hologram);
#else
    ShellXcb shell(hologram);
#endif

    shell.run();

    return 0;
}
