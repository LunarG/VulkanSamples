Vktrace Trace and Replay Tool
=============================

Vktrace is a Vulkan API tracer for graphics applications.

##Using Vktrace on Linux###
Vktrace builds two binaries with associated Vulkan libraries: a tracer with Vulkan
tracing library and a replayer.

###Running Vktrace tracer as standalone server on Linux###
The Vktrace tracer program can run as a server.  Then the app/game to be traced
is launched separately with the Vktrace tracer library preloaded. To run
Vktrace as a server one should omit the "-p" option.
```
cd <vktrace build directory>
./vktrace <options>
Example to trace spinning cube demo.
export VK_ICD_FILENAMES=/home/jon/LoaderAndTools/main_icd.json
export LD_LIBRARY_PATH=/home/jon/LoaderAndTools/dbuild/loader
./vktrace -o vktrace_cube.vktrace
```

In a separate terminal run your app, the cube demo in this example:
```
cd /home/jon/LoaderAndTools/dbuild/demos
export VK_ICD_FILENAMES=/home/jon/LoaderAndTools/dbuild/icd/intel/intel_icd.json
export LD_LIBRARY_PATH=/home/jon/LoaderAndTools/dbuild/loader
LD_PRELOAD=/home/jon/LoaderAndTools/dbuild/vktrace/libvulkan_trace.so ./cube
```

Trace file is written into "vktrace_cube<number>.vktrace".
As the app is rerun, the Vktrace tracer server will increment the output file
number for each succesive run of the app.

One can also set VKTRACE_LIB_IPADDR to a remote system IP address. Then
the tracer inserted into an app will send the tarce packets to the remote
system rather than local system. In this case, the remote system should be
running the trace server.

###Running Vktrace tracer and launch app/game from tracer on Linux###
The Vktrace tracer program launches the app/game you desire and then traces it.
To launch app/game from Vktrace tracer one must use the "-p" option. Also the
-l<number> option should be given for specifying the tracing library.
```
cd <vktrace build dir>
./vktrace -p <path to app to launch>  <more options>
```
Example to trace the spinning cube demo from sample implementation
```
export VK_ICD_FILENAMES=/home/jon/LoaderAndTools/main_icd.json
export LD_LIBRARY_PATH=/home/jon/LoaderAndTools/dbuild/loader
./vktrace -p /home/jon/LoaderAndTools/dbuild/demos/cube -o vktrace_cube.vktrace -w /home/jon/LoaderAndTools/dbuild/demos
```
Trace file is in "vktrace_cube.vktrace".

###Running replayer on Linux###
The Vktrace replayer takes  a trace file  and will launch an Vulkan session based
on trace file.
```
cd <vktrace build dir>
export LD_LIBRARY_PATH=<vktrace build dir>:<loader dir>
./vkreplay <options> -t trace_filename
```
Example to replay trace file captured above
```
export VK_ICD_FILENAMES=/home/jon/LoaderAndTools/main_icd.json
export LD_LIBRARY_PATH=/home/jon/LoaderAndTools/dbuild:/home/jon/LoaderAndTools/dbuild/loader
./vkreplay -t vktrace_cube.vktrace
```

##Using Vktrace on Windows##
Vktrace builds two binaries with associated Vulkan libraries: a tracer with Vulkan
tracing library and a replayer.


###Running Vktrace tracer and launch app/game from tracer on Windows###
The Vktrace tracer program launches the app/game you desire and then traces it.
To launch app/game from Vktrace tracer one must use the "-p" option. Also the
-l<number> option should be given for specifying the tracing library (zero for Vulkan).
Also, you may need to copy the Vulkan.dll library into the directory of Vktrace,
and of the app/game (while we continue to put Windows support into place).
```
cd <vktrace build dir>
./vktrace -p <path-to-app-to-launch> -w <working-dir-path-of-app>  <more options>
```
Example to trace the spinning cube demo (Note: see other files for how to configure your ICD):
```
cd C:\\Users\developer\\Vktrace\\_out64\\Debug

vktrace -p C:\\Users\developer\\LoaderAndTools\\_out64\\demos\\cube.exe
        -w C:\\Users\developer\\LoaderAndTools\\_out64\\demos
        -o vktrace_cube.vktrace
```
Trace file is in "vktrace_cube.vktrace".

###Running replayer on Windows###
The Vktrace replayer takes  a trace file  and will launch an Vulkan session based
on trace file.
```
cd <vktrace build dir>
vkreplay <options> -t trace_filename
```
Example to replay trace file captured above
```
cd C:\\Users\developer\\Vktrace\\_out64\\Debug
vkreplay -t vktrace_cube.vktrace
```
##Building Vktrace##

###External dependencies###
* Python 3.4
  - Ubuntu package: python3.4-dev
  - For Windows, download from: https://www.python.org/downloads.
    You must select to install the optional sub-package to add Python to the system PATH environment variable.

###Building on Linux (make)###
Vktrace is built as part of top level Vulkan Cmake for project. Follow the
build directions for the top level Vulkan project build. Vktrace binaries and
libraries will be place in <build_dir>.
To build Vktrace project only:

```
cd vktrace
mkdir dbuild
cd dbuild
cmake -DCMAKE_BUILD_TYPE=Debug  ..
make
```

###Building on Windows###

```
cd <vktrace repo work dir>
mkdir _out64
cd _out64
cmake -G "Visual Studio 12 2013 Win64" ..
```
// then open the solution file with Visual Studio 2013



###Building on Linux (QtCreator)###
open vktrace/CMakeLists.txt with QtCreator

For Debug Builds:
Cmake options: -DCMAKE_BUILD_TYPE=Debug -DBUILD_X64=On
Change build directory from the suggested 'LoaderAndTools/vktrace-build/' to 'LoaderAndTools/dbuild/vktrace'

For Release Builds:
Cmake Options: -DCMAKE_BUILD_TYPE=Release -DBUILD_X64=On
Change build directory from the suggested 'LoaderAndTools/vktrace-build/' to 'LoaderAndTools/dbuild/vktrace'
