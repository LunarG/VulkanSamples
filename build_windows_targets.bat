echo off
REM
REM This Windows batch file builds this repository for the following targets:
REM 64/32-bit Release/Debug
REM It uses CMake to genererate the project files and then invokes msbuild
REM to build them.
REM The update_external_sources.bat batch file must be executed before running
REM this batch file
REM
REM Arguments:
REM None: Runs CMake and builds all 4 combinations
REM Argument contains:
REM     cmake (case insensitive): Deletes build and build32 and runs just CMake on both
REM     32: Deletes build32, runs CMake and builds 32-bit versions
REM     64: Deletes build, runs CMake and builds 64-bit versions
REM     debug (case insensitive): Builds just the debug config of a 32 and/or 64-bit build
REM     release (case insensitive): Builds just the release config of a 32 and/or 64-bit build
REM Notes:
REM cmake: When specified, generate the CMake build files only - don't compile
REM 32/64: Specifying neither or both builds both
REM debug/release: Specifying neither or both builds both
REM Examples:
REM build_windows_targets.bat 64
REM   -- deletes build, creates build, runs CMake and compiles 64-bit Debug and Release.
REM build_windows_targets.bat 64 debug
REM   -- deletes build, creates build, runs CMake and compiles 64-bit Debug.

set arg_cmake=0
set arg_32=0
set arg_64=0
set arg_debug=0
set arg_release=0

set do_cmake=0
set do_32=0
set do_64=0
set do_debug=0
set do_release=0

for %%a in (%*) do (
    echo.%%a | %WINDIR%\system32\find.exe /I "cmake">Nul && (set arg_cmake=1)
    echo.%%a | %WINDIR%\system32\find.exe "32">Nul && (set arg_32=1)
    echo.%%a | %WINDIR%\system32\find.exe "64">Nul && (set arg_64=1)
    echo.%%a | %WINDIR%\system32\find.exe /I "debug">Nul && (set arg_debug=1)
    echo.%%a | %WINDIR%\system32\find.exe /I "release">Nul && (set arg_release=1)
)

if %arg_32%==1 (
    set do_32=1
)
if %arg_64%==1 (
    set do_64=1
)
if %arg_32%==0 (
    if %arg_64%==0 (
        set do_32=1
        set do_64=1
    )
)

if %arg_debug%==1 (
    set do_debug=1
)
if %arg_release%==1 (
    set do_release=1
)
if %arg_debug%==0 (
    if %arg_release%==0 (
        set do_debug=1
        set do_release=1
    )
)

if %arg_cmake%==1 (
    set do_cmake=1
    set do_32=0
    set do_64=0
    set do_debug=0
    set do_release=0
)

REM Determine the appropriate CMake strings for the current version of Visual Studio
echo Determining VS version
python .\scripts\determine_vs_version.py > vsversion.tmp
set /p VS_VERSION=< vsversion.tmp
echo Detected Visual Studio Version as %VS_VERSION%
del /Q /F vsversion.tmp

if %do_cmake%==1 (
    rmdir /Q /S build
    rmdir /Q /S build32
    mkdir build
    pushd build
    echo Generating 64-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION% Win64" ..
    popd
    mkdir build32
    pushd build32
    echo Generating 32-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION%" ..
    popd
)

REM *******************************************
REM 64-bit build
REM *******************************************
if %do_64%==1 (
    rmdir /Q /S build
    mkdir build
    pushd build
    echo Generating 64-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION% Win64" ..
    if %do_debug% equ 1 (
        echo Building 64-bit Debug 
        msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Debug /maxcpucount /verbosity:quiet
        if errorlevel 1 (
        echo.
        echo 64-bit Debug build failed!
        popd
        exit /b 1
        )
    )
   
    if %do_release%==1 (
        echo Building 64-bit Release 
        msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release /maxcpucount /verbosity:quiet
        if errorlevel 1 (
        echo.
        echo 64-bit Release build failed!
        popd
        exit /b 1
        )
    )
    popd
)
 
REM *******************************************
REM 32-bit build
REM *******************************************
  
if %do_32%==1 (
    rmdir /Q /S build32
    mkdir build32
    pushd build32
    echo Generating 32-bit CMake files for Visual Studio %VS_VERSION%
    cmake -G "Visual Studio %VS_VERSION%" ..
    if %do_debug%==1 (
        echo Building 32-bit Debug 
        msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Debug /maxcpucount /verbosity:quiet
        if errorlevel 1 (
        echo.
        echo 32-bit Debug build failed!
        popd
        exit /b 1
        )
    )

    if %do_release%==1 (
        echo Building 32-bit Release 
        msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release /maxcpucount /verbosity:quiet
        if errorlevel 1 (
        echo.
        echo 32-bit Release build failed!
        popd
        exit /b 1
        )
    )
    popd
)
exit /b 0
