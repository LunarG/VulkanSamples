@echo off

set errorCode=0
set BUILD_DIR=%~dp0

set BASE_DIR="%BUILD_DIR%submodules"
set V_LVL_DIR=%BASE_DIR%\Vulkan-LoaderAndValidationLayers

git submodule update --init --recursive

:build_lvl
    echo.
    echo Setting Up %V_LVL_DIR%
    cd "%V_LVL_DIR%"
    call .\update_external_sources.bat %1 %2 %3 %4 %5 %6 %7 %8 %9