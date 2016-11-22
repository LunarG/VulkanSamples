TEAPOTS
-------
This app demonstrates multi-thread command buffer recording.  
It also demonstrates how to use WSIWindow.

Command-line flags: (at startup)
-------------------
    -v         : Enable validation. (Errors & Warnings only)
    -vv        : Enable Verbose validation.
    -b         : Turn off VSYNC
    -w <int>   : Set window width
    -h <int>   : Set window height
    -b         : Turn off VSYNC
    -nt        : Turn off timer
    -nr        : Turn off rendering
    -np        : Turn off presenting

Keyboard Keys: (runtime)
--------------
    A          : Toggle message-loop blocking / non-blocking mode.
    W/Up-key   : Move camera forward.
    S/Down-key : Move camera backward.
    Space      : Pause / Resume
    F          : Enable fade-mode.
    1          : toggle flag: VK_DEBUG_REPORT_INFORMATION_BIT_EXT
    2          : toggle flag: VK_DEBUG_REPORT_WARNING_BIT_EXT
    3          : toggle flag: VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
    4          : toggle flag: VK_DEBUG_REPORT_ERROR_BIT_EXT
    5          : toggle flag: VK_DEBUG_REPORT_DEBUG_BIT_EXT
    Q / Escape : Quit

CMake settings: (compile-time)
---------------
    ENABLE_VALIDATION : Enable Vulkan Validation. (Recommended for this demo)
    ENABLE_LOGGING    : Allow WSIWindow to print log messages to console or Android LogCat.
    ENABLE_MULTITOUCH : Enables Multi-touch on Windows and Linux. (Not used by this demo.)
    VULKAN_LOADER     : Set path to the libvulkan.so or vulkan-1.lib file.
    VULKAN_INCLUDE    : Set path to the vulkan.h file.

## LINUX
1. Use Qt Creator to open the CMakeLists.txt file.
2. Tweak CMake settings under "Projects". eg. Check the "ENABLE_VALIDATION" flag.
3. Select build type (Debug / Release / ..), Build and Run.

## WINDOWS
1. Use cmake-gui to load the CMakeLists.txt file.
2. Configure the CMake settings, and generate a Visual Studio Solution.
3. Use Visual Studio to open the Solution.
4. Set 'Teapots' as the active project.
3. Select build type (Debug / Release), Build and Run.

## ANDROID (Linux host)
1. Use Android Studio to load the project, using the /Teapots/android/build.gradle file.
2. Connect Android device via USB cable, for ADB connection. (Device must support Vulkan.)
3. Build and run.

