#include <string>
#include <vector>

#include "Hologram.h"
#include "ShellXCB.h"

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);

    Hologram hologram(args);
    ShellXCB shell(hologram);
    shell.run();

    return 0;
}
