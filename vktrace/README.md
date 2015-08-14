Glave Debugger
==============

Glave is a tracer and debugger for graphics applications. The Vulkan API is currently the most supported.

##Screen Shots##
The following screen shots illustrate the current functionality available from the interactive debugger.

###Open###
Freshly opened tool.

![ScreenShot1](docs/images/Vktrace.png "Open")

###Generate Trace###
Click on "Generate Trace" button to launch the application with the trace and save the trace file, then prompt user to load it.

![ScreenShot2](docs/images/Vktrace-GenerateTraceFile.png "Generate Trace")

###Play Trace###
Click "Play" and you will see the Vulkan entrypoints being replayed.

![ScreenShot3](docs/images/Vktrace-LoadedAndPlaying.png "Play Trace")

###Pause###
"Pause" the replay and the upcoming entrypoint will be highlighted.  The timeline shows progress in the tracefile.

![ScreenShot4](docs/images/Vktrace-Paused.png "Pause")

###Search For Memory###
Search the trace file for an entrypoint and parameter.  In this case we find all the calls to a specifc memory object.

![ScreenShot5](docs/images/Vktrace-SearchForMemory.png "Search For Memory")

##Glave Status for Tracing and Replay##
Based on vulkan.h Version 90

* cube             -works
* tri              -works but SEG FAULT  on closing window (XCB connection)
* vk\_image\_tests -works, nothing rendered by this test
* vkbase           -works, nothing rendered by this test
* vulkaninfo       -works, nothing rendered by this test
* vk\_blit\_tests  -crashes due to improper handling of PipelineBarrier
* vk\_render\_test -crashes

##Using Glave on Linux###
Glave builds three binaries with associated Vulkan libraries: a tracer with Vulkan
tracing library; a replayer with Vulkan replayer library; and a debugger with
Vulkan debugger and replayer libraries.

###Running Glave tracer as standalone server on Linux###
The Glave tracer program can run as a server.  Then the app/game to be traced
is launched separately with the Glave tracer library preloaded. To run
GLave tracer as a server one should omit the "-p" option and use the -l<number>
option. The "-l<number>" option specifies a Glave tracing library to listen for
socket connections.
```
cd <glave build directory>
./glvtrace <options>
Example to trace spinning cube demo.
export VK_ICD_FILENAMES=/home/jon/main_icd.json
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvtrace -l0 /home/jon/vulkan/dbuild/tools/glave/libglvtrace_vk.so -o glvtrace_cube.glv
```

In a separate terminal run your app, the cube demo in this example:
```
cd /home/jon/vulkan/dbuild/demos
export LIBVK_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
LD_PRELOAD=/home/jon/xgl/dbuild/tools/glave/libglvtrace_vk.so ./cube
```

Trace file is written into "glvtrace_cube<number>.glv".
As the app is rerun, the Glave tracer server will increment the output file
number for each succesive run of the app.

One can also set GLVLIB_TRACE_IPADDR to a remote system IP address. Then
the tracer inserted into an app will send the tarce packets to the remote
system rather than local system. In this case, the remote system should be
running the trace server.

###Running Glave tracer and launch app/game from tracer on Linux###
The Glave tracer program launches the app/game you desire and then traces it.
To launch app/game from Glave tracer one must use the "-p" option. Also the
-l<number> option should be given for specifying the tracing library.
```
cd <glave build dir>
./glvtrace -p <path to app to launch>  <more options>
```
Example to trace the spinning cube demo from sample implementation
```
export VK_ICD_FILENAMES=/home/jon/main_icd.json
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvtrace -p /home/jon/dbuild/demos/cube -l0 /home/jon/dbuild/tools/glave/libglvtrace_vk.so -o glvtrace_cube.glv -w /home/jon/dbuild/demos
```
Trace file is in "glvtrace_cube.glv".

###Running replayer on Linux###
The Glave replayer takes  a trace file  and will launch an Vulkan session based
on trace file.
```
cd <glave build dir>
export LD_LIBRARY_PATH=<glave build dir>:<loader dir>
./glvreplay <options> -t trace_filename
```
Example to replay trace file captured above
```
export VK_ICD_FILENAMES=/home/jon/main_icd.json
export LD_LIBRARY_PATH=/home/jon/glave/dbuild:/home/jon/dbuild/loader
./glvreplay -t glvtrace_cube.glv
```
###Running debugger on Linux###

NOTE : Debugger is currently disabled and cannot be built or used.

```
cd <glave build dir>
export VK_ICD_FILENAMES=/home/jon/main_icd.json
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvdebug
```

##Using Glave on Windows##
Glave builds three binaries with associated Vulkan libraries: a tracer with Vulkan
tracing library; a replayer with Vulkan replayer library; and a debugger  with
Vulkan debugger and replayer libraries.


###Running Glave tracer and launch app/game from tracer on Windows###
The Glave tracer program launches the app/game you desire and then traces it.
To launch app/game from Glave tracer one must use the "-p" option. Also the
-l<number> option should be given for specifying the tracing library (zero for Vulkan).
Also, you may need to copy the Vulkan.dll library into the directory of Glave,
and of the app/game (while we continue to put Windows support into place).
```
cd <glave build dir>
./glvtrace -p <path-to-app-to-launch> -l0 <path-to-glvtrace_vk-dll> -w <working-dir-path-of-app>  <more options>
```
Example to trace the spinning cube demo (Note: see other files for how to configure your ICD):
```
cd C:\\Users\developer\\Vktrace\\_out64\\Debug

glvtrace -p C:\\Users\developer\\vulkan\\_out64\\demos\\cube.exe
           -l0 C:\\Users\developer\\vulkan\\_out64\\tools\\glave\\Debug\\glvtrace_xgl.dll
           -w C:\\Users\developer\\vulkan\\_out64\\demos
           -o glvtrace_cube.glv
```
Trace file is in "glvtrace_cube.glv".

###Running replayer on Windows###
The Glave replayer takes  a trace file  and will launch an Vulkan session based
on trace file.
```
cd <glave build dir>
glvreplay <options> -t trace_filename
```
Example to replay trace file captured above
```
cd C:\\Users\developer\\Vktrace\\_out64\\Debug
glvreplay -t glvtrace_cube.glv
```
###Running debugger on Windows###
```
cd <glave build dir>
glvdebug
```

##Building Glave##

###External dependencies###
* Python 3.4
  - Ubuntu package: python3.4-dev
  - For Windows, download from: https://www.python.org/downloads.
    You must select to install the optional sub-package to add Python to the system PATH environment variable.
* Qt 5
  - Ubuntu package: qt5-default (only needed for Glave debugger).
  - For Windows, download from: http://www.qt.io/download/.  You must select "Qt 5.3" in the installer.  This is only needed for the Glave debugger.

###Building on Linux (make)###
Glave is built as part of top level Vulkan Cmake for project. Follow the
build directions for the top level Vulkan project build. Glave binaries and
libraries will be place in <build_dir>.
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
cd <glave repo work dir>
mkdir _out64
cd _out64
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
