set OLDDIR=%CD%
@cd "C:\Program Files (x86)\Microsoft Visual Studio 14.0"
call "Common7\Tools\VsDevCmd.bat"
@set PATH=C:\Program Files\CMake\bin;%PATH%
@date /t
@time /t
cd %OLDDIR%

call .\build_windows_targets.bat
@date /t
@time /t
@pause
