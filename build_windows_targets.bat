echo off
REM
REM This Windows batch file builds this repository for the following targets:
REM 64-bit Debug
REM 64-bit Release
REM 32-bit Debug
REM 32-bit Release
REM It uses CMake to genererate the project files and then invokes msbuild
REM to build them.
REM The update_external_sources.bat batch file must be executed before running
REM this batch file
REM

REM Determine the appropriate CMake strings for the current version of Visual Studio
echo Determining VS version
python .\determine_vs_version.py > vsversion.tmp
set /p VS_VERSION=< vsversion.tmp
echo Detected Visual Studio Version as %VS_VERSION%
del /Q /F vsversion.tmp

rmdir /Q /S build
rmdir /Q /S build32

REM *******************************************
REM 64-bit build
REM *******************************************
mkdir build
pushd build

echo Generating 64-bit CMake files for Visual Studio %VS_VERSION%
cmake -G "Visual Studio %VS_VERSION% Win64" ..
   
echo Building 64-bit Debug 
msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Debug /verbosity:quiet
if errorlevel 1 (
   echo.
   echo 64-bit Debug build failed!
   popd
   exit /B 1
)   
   
echo Building 64-bit Release 
msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release /verbosity:quiet
if errorlevel 1 (
   echo.
   echo 64-bit Release build failed!
   popd
   exit /B 1
)   

popd
 
REM *******************************************
REM 32-bit build
REM *******************************************
mkdir build32
pushd build32
  
echo Generating 32-bit CMake files for Visual Studio %VS_VERSION%
cmake -G "Visual Studio %VS_VERSION%" ..
   
echo Building 32-bit Debug 
msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Debug /verbosity:quiet
if errorlevel 1 (
   echo.
   echo 32-bit Debug build failed!
   popd
   exit /B 1
)   
   
echo Building 32-bit Release 
msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release /verbosity:quiet
if errorlevel 1 (
   echo.
   echo 32-bit Release build failed!
   popd
   exit /B 1
)   

popd
exit /b 0
