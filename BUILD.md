The XGL Sample Intel driver supports Linux.

System Requirements
===================

Ubuntu 13.10, Ubuntu 14.04 LTS
gcc
drm


BUILD
=====

The sample driver users cmake and should work with the usual cmake options and utilities.
The standard build process builds the icd, the icd loader and all the tests.

Example debug build:
cd xgl  # cd to the root of the xgl git repository
cmake -H. -Bdbuild -DCMAKE_BUILD_TYPE=Debug
cd dbuild
make

To run XGL programs you must tell the icd loader where to find the libraries. Set the
environment variable LIBXGL_DRIVERS_PATH to the driver path. For example:
> export LIBXGL_DRIVERS_PATH=~/xgl/dbuild/icd/intel

TEST
====

The test executibles can be found in the build/tests directory. The tests use the Google
gtest infrastructure. Tests avilable so far:
- xglinfo: Report GPU properties
- xglbase: Test basic entry points
- xgl_image_tests: Test XGL image related calls needed by render_test
- xgl_render_tests: Render a single triangle with XGL. Triangle will be in a .ppm in
the current directory at the end of the test.
