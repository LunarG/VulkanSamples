Glave (aka gl5_debugger)
============

Working towards a tracer / debugger of OpenGL5 applications.  Currently,
Glave builds three binaries with associated XGL libraries. A tracer with XGL
tracing library.  A replayer with XGL replayer library.  And a debugger  with
XGL debugger and replayer libraries.

###Obtain code###
```
git clone https://github.com/KhronosGroup/GL-Next.git
```

###External dependencies###
* Python 3.4  (Ubuntu package python3.4-dev)
* Qt 5        (Ubuntu package qt5-default) (only needed for glave debugger)

###Building on Linux (make)###
Glave is built as part of top level XGL Cmake for project. Follow the
build directions for the top level XGL project build. Glave binaries and
libraries will be place in <build_dir>/tools/glave.
To build Glave project only:

```
cd tools/glave
mkdir glv_build
cd glv_build
cmake -DCMAKE_BUILD_TYPE=Debug  ..
make
```

###Building on Windows###

```
cd tools/glave
mkdir Win64
cd Win64
cmake -G "Visual Studio 12 2013 Win64" ..
```
// then open the solution file with Visual Studio 2013



###Building on Linux (QtCreator)###
open tools/glave/CMakeLists.txt with QtCreator

For Debug Builds:
Cmake options: -DCMAKE_BUILD_TYPE=Debug -DBUILD_X64=On
Change build directory from the suggested 'GL-Next/tools/glave-build/' to 'GL-Next/dbuild/tools/glave'

For Release Builds:
Cmake Options: -DCMAKE_BUILD_TYPE=Release -DBUILD_X64=On
Change build directory from the suggested 'GL-Next/tools/glave-build/' to 'GL-Next/build/tools/glave'


###Running Glave tracer as standalone server###
The Glave tracer program can run as a server.  Then the app/game to be traced
is launched separately with the Glave tracer library preloaded. To run
GLave tracer as a server one should omit the "-p" option and use the -l<number>
option. The "-l<number>" option specifies a Glave tracing library to listen for
socket connections.
```
cd <xgl build directory>/tools/glave
./glvtrace64 <options>
Example to trace spinning cube demo.
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvtrace64 -p /home/jon/xgl/dbuild/demos/cube -l0 /home/jon/xgl/dbuild/tools/glave/libglvtrace_xgl64.so -o glvtrace_cube.glv
```

In a separate terminal run your app, the cube demo in this example:
```
cd /home/jon/xgl/dbuild/demos
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
LD_PRELOAD=/home/jon/xgl/dbuild/tools/glave/libglvtrace_xgl64.so ./cube
```

Trace file is written into "glvtrace_cube<number>.glv".
As the app is rerun, the Glave tracer server will increment the output file
number for each succesive run of the app.

Rather than giving command line options, one can instead put tracer options in
a settings file: "glvtrace_settings.txt".


###Running Glave tracer and launch app/game from tracer###
The Glave tracer program launches the app/game you desire and then traces it.
To launch app/game from Glave tracer one must use the "-p" option. Also the
-l<number> option should be given for specifying the tracing library.
```
cd <xgl build directory>/tools/glave
./glvtrace64 -p <path to app to luanch>  <more options>
```
Example to trace the spinning cube demo from sample implementation
```
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvtrace64 -p /home/jon/xgl/dbuild/demos/cube -l0 /home/jon/xgl/dbuild/tools/glave/libglvtrace_xgl64.so -o glvtrace_cube.glv -w /home/jon/xgl/dbuild/demos
```
Trace file is in "glvtrace_cube.glv".

Rather than giving command line options, can instead put tracer options in a
settings file: "glvtrace_settings.txt".
```

###Running replayer###
The Glave replayer takes  a trace file  and will launch an XGL session based
on trace file.
```
cd <xgl build dir>/tools/glave
./glvreplay64 <options> trace_filename
```
Example to replay trace file captured above
```
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvreplay64 glvtrace_cube.glv
```
###Running debugger###
```
cd <xgl build dir>/tools/glave
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvdebug64
```