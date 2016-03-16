# Build Instructions
This document contains the instructions for building this repository on Linux and Windows.

This repository does not contain a Vulkan-capable driver.
Before proceeding, it is strongly recommended that you obtain a Vulkan driver from your graphics hardware vendor
and install it.

Note: The sample Vulkan Intel driver for Linux (ICD) is being deprecated in favor of other driver options from Intel.
This driver has been moved to the [VulkanTools repo](https://github.com/LunarG/VulkanTools).
Further instructions regarding this ICD are available there.

## Git the Bits

To create your local git repository:
```
git clone https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers 
```

If you intend to contribute, the preferred work flow is for you to develop your contribution
in a fork of this repo in your GitHub account and then submit a pull request.
Please see the CONTRIBUTING.md file in this respository for more details.

## Linux Build

The build process uses CMake to generate makefiles for this project.
The build generates the loader, layers, and tests.

This repo has been built and tested on Ubuntu 14.04.3 LTS, 14.10, 15.04 and 15.10.
It should be straightforward to use it on other Linux distros.

These packages are needed to build this repository: 
```
sudo apt-get install git cmake build-essential bison libxcb1-dev
```

Example debug build:
```
cd Vulkan-LoaderAndValidationLayers  # cd to the root of the cloned git repository
./update_external_sources.sh  # Fetches and builds glslang and spirv-tools
cmake -H. -Bdbuild -DCMAKE_BUILD_TYPE=Debug
cd dbuild
make
```

If you have installed a Vulkan driver obtained from your graphics hardware vendor, the install process should
have configured the driver so that the Vulkan loader can find and load it.

If you want to use the loader and layers that you have just built:
```
export LD_LIBRARY_PATH=<path to your repository root>/dbuild/loader
export VK_LAYER_PATH=<path to your repository root>/dbuild/layers
```
Note that if you have installed the [LunarG Vulkan SDK](https://vulkan.lunarg.com),
you will also have the SDK version of the loader and layers installed in your default system libraries.

You can run the `vulkaninfo` application to see which driver, loader and layers are being used.

The `LoaderAndLayerInterface` document in the `loader` folder in this repository is a specification that
describes both how ICDs and layers should be properly
packaged, and how developers can point to ICDs and layers within their builds.

## Validation Test

The test executables can be found in the dbuild/tests directory. 
Some of the tests that are available:
- vk_layer_validation_tests: Test Vulkan layers.

There are also a few shell and Python scripts that run test collections (eg,
`run_all_tests.sh`).

## Linux Demos

Some demos that can be found in the dbuild/demos directory are:
- vulkaninfo: report GPU properties
- tri: a textured triangle (which is animated to demonstrate Z-clipping)
- cube: a textured spinning cube
- smoke/smoke: A "smoke" test using a more complex Vulkan demo

## Windows System Requirements

Windows 7+ with additional required software packages:

- Microsoft Visual Studio 2013 Professional.  Note: it is possible that lesser/older versions may work, but that has not been tested.
- CMake (from http://www.cmake.org/download/).  Notes:
  - Tell the installer to "Add CMake to the system PATH" environment variable.
- Python 3 (from https://www.python.org/downloads).  Notes:
  - Select to install the optional sub-package to add Python to the system PATH environment variable.
  - Ensure the pip module is installed (it should be by default)
  - Need python3.3 or later to get the Windows py.exe launcher that is used to get python3 rather than python2 if both are installed on Windows
  - 32 bit python works
- Python lxml package must be installed
  - Download the lxml package from
        http://www.lfd.uci.edu/~gohlke/pythonlibs/#lxml
        32-bit latest for Python 3.5 is: lxml-3.5.0-cp35-none-win32.whl
        64-bit latest for Python 3.5 is: lxml-3.5.0-cp35-none-win_amd64.whl
  - The package can be installed with pip as follows:
        pip install lxml-3.5.0-cp35-none-win32.whl
        If pip is not in your path, you can find it at $PYTHON_HOME\Scripts\pip.exe, where PYTHON_HOME is the folder where you installed Python.
- Git (from http://git-scm.com/download/win).
  - Note: If you use Cygwin, you can normally use Cygwin's "git.exe".  However, in order to use the "update_external_sources.bat" script, you must have this version.
  - Tell the installer to allow it to be used for "Developer Prompt" as well as "Git Bash".
  - Tell the installer to treat line endings "as is" (i.e. both DOS and Unix-style line endings).
  - Install each a 32-bit and a 64-bit version, as the 64-bit installer does not install the 32-bit libraries and tools.
- glslang is required for demos and tests.
  - You can download and configure it (in a peer directory) here: https://github.com/KhronosGroup/glslang/blob/master/README.md
  - A windows batch file has been included that will pull and build the correct version.  Run it from Developer Command Prompt for VS2013 like so:
    - update_external_sources.bat --build-glslang

Optional software packages:

- Cygwin (from https://www.cygwin.com/).  Notes:
  - Cygwin provides some Linux-like tools, which are valuable for obtaining the source code, and running CMake.
    Especially valuable are the BASH shell and git packages.
  - If you don't want to use Cygwin, there are other shells and environments that can be used.
    You can also use a Git package that doesn't come from Cygwin.

## Windows Build

Cygwin is used in order to obtain a local copy of the Git repository, and to run the CMake command that creates Visual Studio files.  Visual Studio is used to build the software, and will re-run CMake as appropriate.

To build all Windows targets (e.g. in a "Developer Command Prompt for VS2013" window):
```
cd Vulkan-LoaderAndValidationLayers  # cd to the root of the cloned git repository
update_external_sources.bat --all
build_windows_targets.bat 
```

At this point, you can use Windows Explorer to launch Visual Studio by double-clicking on the "VULKAN.sln" file in the \build folder.  Once Visual Studio comes up, you can select "Debug" or "Release" from a drop-down list.  You can start a build with either the menu (Build->Build Solution), or a keyboard shortcut (Ctrl+Shift+B).  As part of the build process, Python scripts will create additional Visual Studio files and projects, along with additional source files.  All of these auto-generated files are under the "build" folder.

Vulkan programs must be able to find and use the vulkan-1.dll libary. Make sure it is either installed in the C:\Windows\System32 folder, or the PATH environment variable includes the folder that it is located in.

To run Vulkan programs you must tell the icd loader where to find the libraries.
This is described in a `LoaderAndLayerInterface` document in the `loader` folder in this repository.
This specification describes both how ICDs and layers should be properly
packaged, and how developers can point to ICDs and layers within their builds.

