Glave (aka gl5_debugger)
============

Working towards a tracer / debugger of OpenGL5 applications.

###Obtain code###
```
git clone https://github.com/KhronosGroup/GL-Next.git
```

###External dependencies###
* XGL
  XGL headers retrieved in place (tools/glave/../../include) by Cmake
  XGL library retrieved in place (tools/glave/../../\<build_dir\>/loader by Cmake
* Python 3.4  (Ubuntu package python3.4-dev)
* Qt 5        (Ubuntu package qt5-default) (only needed for glave debugger)

###Building on Windows###
```
cd tools/glave
mkdir Win64
cd Win64
cmake -G "Visual Studio 10 2010 Win64" ..
// then open the solution file with Visual Studio 2010
```

###Building on Linux (make)###
```
Glave is built as part of top level Cmake for XGL project.
To build Glave only:
cd tools/glave
mkdir glv_build
cd glv_build
cmake -DCMAKE_BUILD_TYPE=Debug  ..
make
```

###Building on Linux (QtCreator)###
```
open tools/glave/CMakeLists.txt with QtCreater

For Debug Builds:
Cmake options: -DCMAKE_BUILD_TYPE=Debug -DBUILD_X64=On
Change build directory from the suggested 'GL-Next/tools/glave-build/' to 'GL-Next/dbuild/tools/glave'

For Release Builds:
Cmake Options: -DCMAKE_BUILD_TYPE=Release -DBUILD_X64=On
Change build directory from the suggested 'GL-Next/tools/glave-build/' to 'GL-Next/build/tools/glave'
```

###Running tracer###
```
The Glave tracer program launches the app/game you desire and then traces it.
cd glv_build
./glvtrace64 <options>
Example to trace the spinning cube demo from sample implementation
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvtrace64 -p /home/jon/xgl/dbuild/demos/cube -l0 /home/jon/xgl/glv_build/libglvtrace_xgl64.so -o glvtrace_cube.glv -w /home/jon/xgl/dbuild/demos
Trace file is in "glvtrace_cube.glv".

Rather than giving command line options, can instead put tracer options in a
settings file: "glvtrace_settings.txt".
```

###Running replayer###
```
The Glave replayer takes  a trace file  and will launch an XGL session based
on trace file.
cd glv_build
./glvreplay64 <options> trace_filename
Example to replay trace file captured above
export LIBXGL_DRIVERS_PATH=/home/jon/dbuild/icd/intel
export LD_LIBRARY_PATH=/home/jon/dbuild/loader
./glvreplay64 glvtrace_cube.glv
```
