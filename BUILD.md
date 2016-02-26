# Build Instructions
These instructions are for Linux and Windows.

It is strongly suggested that you first install a Vulkan-capable driver, obtained from your graphics hardware vendor.

Note: The sample Vulkan Intel driver for Linux (ICD) has been moved to the
[VulkanTools repo](https://github.com/LunarG/VulkanTools).
Further instructions regarding the ICD are available there.

## Git the Bits

If you intend to contribute, the preferred work flow is to fork the repo, create a branch in your forked repo, do the work, and create a pull request on GitHub to integrate that work back into the repo.

To create your local git repository:
```
git clone https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers 
# Or substitute the URL from your forked repo for https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers above.
```

## Linux Build

The build process uses cmake and should work with the usual cmake options and utilities.
The build generates the loader, layers, and tests.

This repo has been built and tested on Ubuntu 14.04.3 LTS, 14.10, 15.04 and 15.10.
It should be straightforward to use it on other Linux distros.

These packages should be installed 
```
sudo apt-get install git cmake build-essential bison libxcb1-dev
```

Example debug build:
```
cd YOUR_DEV_DIRECTORY  # cd to the root of the Vulkan-LoaderAndValidationLayers git repository
./update_external_sources.sh  # Fetches and builds glslang and spirv-tools
cmake -H. -Bdbuild -DCMAKE_BUILD_TYPE=Debug
cd dbuild
make
```

To run Vulkan programs you must tell the icd loader where to find the libraries.
This is described in a [specification](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/blob/sdk-1.0.3/loader/LoaderAndLayerInterface.md#vulkan-installable-client-driver-interface-with-the-loader).

This specification describes both how ICDs and layers should be properly
packaged, and how developers can point to ICDs and layers within their builds.

For example, you may wish to point to your just-built loader and layers with:
```
export LD_LIBRARY_PATH=<path to your build root>/dbuild/loader
export VK_LAYER_PATH=<path to your build root>/dbuild/layers
```

## Validation Test

The test executables can be found in the dbuild/tests directory. The tests use the Google
gtest infrastructure. Tests available so far:
- vk_layer_validation_tests: Test Vulkan layers.

There are also a few shell and Python scripts that run test collections (eg,
`run_all_tests.sh`).

## Linux Demos

The demos executables can be found in the dbuild/demos directory.
- vulkaninfo: report GPU properties
- tri: a textured triangle (which is animated to demonstrate Z-clipping)
- cube: a textured spinning cube

## Windows System Requirements

Windows 7+ with additional required software packages:

- Microsoft Visual Studio 2013 Professional.  Note: it is possible that lesser/older versions may work, but that has not been tested.
- CMake (from http://www.cmake.org/download/).  Notes:
  - Tell the installer to "Add CMake to the system PATH" environment variable.
- Python 3 (from https://www.python.org/downloads).  Notes:
  - Select to install the optional sub-package to add Python to the system PATH environment variable.
  - Need python3.3 or later to get the Windows py.exe launcher that is used to get python3 rather than python2 if both are installed on Windows
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

Example debug x64 build (e.g. in a "Developer Command Prompt for VS2013" window):
```
cd YOUR_DEV_DIRECTORY  # cd to the root of the Vulkan-LoaderAndValidationLayers git repository
update_external_sources.bat --all
build_windows_targets.bat 
```

At this point, you can use Windows Explorer to launch Visual Studio by double-clicking on the "VULKAN.sln" file in the \build folder.  Once Visual Studio comes up, you can select "Debug" or "Release" from a drop-down list.  You can start a build with either the menu (Build->Build Solution), or a keyboard shortcut (Ctrl+Shift+B).  As part of the build process, Python scripts will create additional Visual Studio files and projects, along with additional source files.  All of these auto-generated files are under the "build" folder.

Vulkan programs must be able to find and use the vulkan-1.dll libary. Make sure it is either installed in the C:\Windows\System32 folder, or the PATH environment variable includes the folder that it is located in.

To run Vulkan programs you must tell the icd loader where to find the libraries.
This is described in a [specification](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/blob/sdk-1.0.3/loader/LoaderAndLayerInterface.md#vulkan-installable-client-driver-interface-with-the-loader).

This specification describes both how ICDs and layers should be properly
packaged, and how developers can point to ICDs and layers within their builds.

### Windows 64-bit Installation Notes
If you plan on creating a Windows Install file (done in the windowsRuntimeInstaller sub-directory) you will need to build for both 32-bit and 64-bit Windows since both versions of EXEs and DLLs exist simultaneously on Windows 64.

To do this, simply create and build the release versions of each target:
```
cd LoaderAndTools  # cd to the root of the Vulkan git repository
update_external_sources.bat --all
mkdir build
cd build
cmake -G "Visual Studio 12 Win64" ..
msbuild ALL_BUILD.vcxproj /p:Platform=x64 /p:Configuration=Release
mkdir build32
cd build32
cmake -G "Visual Studio 12" ..
msbuild ALL_BUILD.vcxproj /p:Platform=x86 /p:Configuration=Release
```

