set OLDDIR=%CD%
@cd "C:\Program Files (x86)\Microsoft Visual Studio 14.0"
call "Common7\Tools\VsDevCmd.bat"
@set PATH=C:\Program Files\CMake\bin;%PATH%
@date /t
@time /t
cd %OLDDIR%

set TGT_DIR=BUILD
cmake -G "Visual Studio 14 Win64" -H. -B%TGT_DIR%
cd %TGT_DIR%
set VK_LAYER_PATH=%CD%\layers\Debug
start VULKAN.sln
