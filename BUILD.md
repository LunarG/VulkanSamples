# Build Instructions
This project supports Linux today.
Support for Windows will start in Q1'15.
Support for Android is TBD.

##System Requirements
Ubuntu 14.04 LTS, (Ubuntu 14.10 needed for DRI 3 demos)

```
sudo apt-get install git subversion cmake libgl1-mesa-dev freeglut3-dev libglm-dev libpng12-dev
sudo apt-get build-dep mesa
```

##Build

The sample driver uses cmake and should work with the usual cmake options and utilities.
The standard build process builds the icd, the icd loader and all the tests.

Example debug build:
```
cd xgl  # cd to the root of the xgl git repository
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

##Test

The test executibles can be found in the dbuild/tests directory. The tests use the Google
gtest infrastructure. Tests avilable so far:
- xglinfo: Report GPU properties
- xglbase: Test basic entry points
- xgl_blit_tests: Test XGL Blits (copy, clear, and resolve)
- xgl_image_tests: Test XGL image related calls needed by render_test
- xgl_render_tests: Render a single triangle with XGL. Triangle will be in a .ppm in
the current directory at the end of the test.

#Demos

The demos executables can be found in the dbuild/demos directory. The demos use DRI 3
to render directly onto window surfaces.
- tri: a textured triangle
- cube: a textured spinning cube

#Render Nodes

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
```
```
sudo tee /etc/udev/rules.d/drm.rules << EOF
# Add permissions to render nodes
SUBSYSTEM=="drm", ACTION=="add", DEVPATH=="/devices/*/renderD*", MODE="020666"
EOF
```
