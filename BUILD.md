# Build Instructions
This project fully supports Linux today.
Support for Windows is for the loader and layers (additional info below).  Additional Windows support will be coming in Q1'15.
Support for Android is TBD.

##Linux System Requirements
Ubuntu 14.10 needed for DRI 3

```
sudo apt-get install git subversion cmake libgl1-mesa-dev freeglut3-dev libglm-dev libpng12-dev libmagickwand-dev qt5-default
sudo apt-get build-dep mesa
```
Warning: Recent versions of 14.10 have **REMOVED** DRI 3.  
Version: 2:2.99.914-1~exp1ubuntu4.1 is known to work.  
To see status of this package:
```
dpkg -s xserver-xorg-video-intel
```

Note:  
Version 2:2.99.914-1~exp1ubuntu4.2 does not work anymore.  
To install the working driver from launchpadlibrarian.net:  
- Remove the current driver:  
```
sudo apt-get purge xserver-xorg-video-intel
```
- Download the old driver:  
```
wget http://launchpadlibrarian.net/189418339/xserver-xorg-video-intel_2.99.914-1%7Eexp1ubuntu4.1_amd64.deb
```
- Install the driver:
```
sudo dpkg -i xserver-xorg-video-intel_2.99.914-1~exp1ubuntu4.1_amd64.deb
```
- Either restart Ubuntu or just X11.  
  
##Linux Build

The sample driver uses cmake and should work with the usual cmake options and utilities.
The standard build process builds the icd, the icd loader and all the tests.

Example debug build:
```
cd GL-Next  # cd to the root of the xgl git repository
export KHRONOS_ACCOUNT_NAME= <subversion login name for svn checkout of BIL>
./update_external_sources.sh  # fetches and builds glslang, llvm, LunarGLASS, and BIL
cmake -H. -Bdbuild -DCMAKE_BUILD_TYPE=Debug
cd dbuild
make
```

To run XGL programs you must tell the icd loader where to find the libraries. Set the
environment variable LIBXGL_DRIVERS_PATH to the driver path. For example:
```
export LIBXGL_DRIVERS_PATH=$PWD/icd/intel
```

To enable debug and validation layers with your XGL programs you must tell the icd loader
where to find the layer libraries. Set the environment variable LIBXGL_LAYERS_PATH to
the layer folder and indicate the layers you want loaded via LIBXGL_LAYER_NAMES.
For example, to enable the APIDump and DrawState layers, do:
```
export LIBXGL_LAYERS_PATH=$PWD/layers
export LIBXGL_LAYER_NAMES=APIDump:DrawState
```

##Linux Test

The test executibles can be found in the dbuild/tests directory. The tests use the Google
gtest infrastructure. Tests available so far:
- xglinfo: Report GPU properties
- xglbase: Test basic entry points
- xgl_blit_tests: Test XGL Blits (copy, clear, and resolve)
- xgl_image_tests: Test XGL image related calls needed by render_test
- xgl_render_tests: Render a single triangle with XGL. Triangle will be in a .ppm in
the current directory at the end of the test.

##Linux Demos

The demos executables can be found in the dbuild/demos directory. The demos use DRI 3
to render directly onto window surfaces.
- tri: a textured triangle
- cube: a textured spinning cube

##Linux Render Nodes

The render tests depend on access to DRM render nodes.
To make that available, a couple of config files need to be created to set a module option
and make accessible device files.
The system will need to be rebooted with these files in place to complete initialization.
These commands will create the config files.

```
sudo tee /etc/modprobe.d/drm.conf << EOF
# Enable render nodes
options drm rnodes=1
EOF
# this will add the rnodes=1 option into the boot environment
sudo update-initramfs -k all -u
```
```
sudo tee /etc/udev/rules.d/drm.rules << EOF
# Add permissions to render nodes
SUBSYSTEM=="drm", ACTION=="add", DEVPATH=="/devices/*/renderD*", MODE="020666"
EOF
```

##Windows System Requirements

Windows 7+ with additional, software:

- Microsoft Visual Studio 2013 Professional.  Note: it is possible that lesser/older versions may work, but that has not been tested.
- CMake (from https://www.python.org/downloads).  Note: Configure to add itself to the system PATH environment variable.
- Python 3 (from https://www.python.org/downloads).  Note: Configure to add itself to the system PATH environment variable.
- Cygwin (especially a BASH shell and git packages--from https://www.cygwin.com/).

##Windows Build

Cygwin is used in order to obtain a local copy of the Git repository, and to run the CMake command that creates Visual Studio files.  Visual Studio is used to build the software, and will re-run CMake as appropriate.

Example debug build:
```
cd GL-Next  # cd to the root of the xgl git repository
mkdir _out64
cd _out64
cmake -G "Visual Studio 12 Win64" ..
```

At this point, you can use Windows Explorer to launch Visual Studio by double-clicking on the "XGL.sln" file in the _out64 folder.  Once Visual Studio comes up, you can select "Debug" or "Release" from a drop-down list.  You can start a build with either the menu (Build->Build Solution), or a keyboard shortcut (Ctrl+Shift+B).  As part of the build process, Python scripts will create additional Visual Studio files and projects, along with additional source files.  All of these auto-generated files are under the "_out64" folder.

To run XGL programs you must have an appropriate icd (installable client driver) that is either installed in the C:\Windows\System32 folder, or pointed to by the
environment variable LIBXGL_DRIVERS_PATH.  This environment variable cannot be set with Cygwin, but must be set via Windows, and may require a system restart in order for it to take effect.  Here is how to set this environment variable on a Windows 7 system:

- Launch Control Panel (e.g. Start->Control Panel)
- Within the search box, type "environment variable" and click on "Edit the system environment variables" (or navigate there via "System and Security->System->Advanced system settings").
- This will launch a window with several tabs, one of which is "Advanced".  Click on the "Environment Variables..." button.
- For either "User variables" or "System variables" click "New...".
- Enter "LIBXGL_DRIVERS_PATH" as the variable name, and an appropriate Windows path to where your driver DLL is (e.g. C:\Users\username\GL-Next\_out64\icd\drivername\Debug).

It is possible to specify multiple icd folders.  Simply use a semi-colon (i.e. ";") to separate folders in the environment variable.

The icd loader searches in all of the folders for files that are named "XGL_*.dll" (e.g. "XGL_foo.dll").  It attempts to dynamically load these files, and look for appropriate functions.

To enable debug and validation layers with your XGL programs you must tell the icd loader
where to find the layer libraries, and which ones you desire to use.  The default folder for layers is C:\Windows\System32.  However, you can use the following environment variables to specify alternate locations, and to specify which layers to use:

- LIBXGL_LAYERS_PATH (semi-colon-delimited set of folders to look for layers)
- LIBXGL_LAYER_NAMES (color-delimited list of layer names)

For example, to enable the APIDump and DrawState layers, set:

- "LIBXGL_LAYERS_PATH" to "C:\Users\username\GL-Next\_out64\layers\Debug"
- "LIBXGL_LAYER_NAMES to "APIDump:DrawState"

The icd loader searches in all of the folders for files that are named "XGLLayer*.dll" (e.g. "XGLLayerParamChecker.dll").  It attempts to dynamically load these files, and look for appropriate functions.
