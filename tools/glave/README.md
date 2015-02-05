Glave Debugger
==============

Glave is a tracer and debugger for XGL applications.

##Screen Shots##
The following screen shots illustrate the current functionality available from the interactive debugger.

###Open###
Freshly opened tool.

![ScreenShot1](../../docs/images/Glave.png "Open")

###Generate Trace###
Click on "Generate Trace" button to launch the application with the trace and save the trace file, then prompt user to load it.

![ScreenShot2](../../docs/images/Glave-GenerateTraceFile.png "Generate Trace")

###Play Trace###
Click "Play" and you will see the XGL entrypoints being replayed.

![ScreenShot3](../../docs/images/Glave-LoadedAndPlaying.png "Play Trace")

###Pause###
"Pause" the replay and the upcoming entrypoint will be highlighted.  The timeline shows progress in the tracefile.

![ScreenShot4](../../docs/images/Glave-Paused.png "Pause")

###Search For Memory###
Search the trace file for an entrypoint and parameter.  In this case we find all the calls to a specifc memory object.

![ScreenShot5](../../docs/images/Glave-SearchForMemory.png "Search For Memory")

##Glave Status for Tracing and Replay##
Based on xgl.h Version 47.1

* cube             -works
* tri              -works but SEG FAULT  on closing window (XCB connection)
* xgl\_image\_tests  -works, nothing rendered by this test
* xglbase          -works, nothing rendered by this test
* xglinfo          -works, nothing rendered by this test
* xgl\_blit\_tests   -crashes due to improper handling of PipelineBarrier
* xgl\_render\_tests -crashes

##Using Glave###
Glave builds three binaries with associated XGL libraries: a tracer with XGL
tracing library; a replayer with XGL replayer library; and a debugger  with
XGL debugger and replayer libraries.

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
./glvtrace64 -l0 /home/jon/xgl/dbuild/tools/glave/libglvtrace_xgl64.so -o glvtrace_cube.glv
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

One can also set GLVLIB_TRACE_IPADDR to a remote system IP address. Then
the tracer inserted into an app will send the tarce packets to the remote
system rather than local system. In this case, the remote system should be
running the trace server.

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

###Running replayer###
The Glave replayer takes  a trace file  and will launch an XGL session based
on trace file.
```
cd <xgl build dir>/tools/glave
./glvreplay64 <options> -t trace_filename
```
Example to replay trace file captured above
```
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvreplay64 -t glvtrace_cube.glv
```
###Running debugger###
```
cd <xgl build dir>/tools/glave
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvdebug64
```

##Building Glave##

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
