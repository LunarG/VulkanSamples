# Layer Description and Status

## Overview

Layer libraries can be written to intercept or hook XGL entrypoints for various
debug and validation purposes.  One or more XGL entrypoints can be defined in your Layer
library.  Undefined entrypoints in the Layer library will be passed to the next Layer which
may be the driver.  Multiple layer libraries can be chained (actually a hierarchy) together.
xglEnumerateLayer can be called to list the available layer libraries.  xglGetProcAddr is
used internally by the Layers and ICD Loader to initialize dispatch tables. Layers are
activated at xglCreateDevice time. xglCreateDevice createInfo struct is extended to allow
a list of layers to be activated.  Layer libraries can alternatively be LD\_PRELOADed depending
upon how they are implemented.

##Layer library example code

Note that some layers are code-generated and will therefore exist in the directory (build_dir)/layers

-include/xglLayer.h  - header file for layer code.

### Templates
layer/Basic.cpp (name=Basic) simple example wrapping a few entrypoints. Shows layer features:
- Multiple dispatch tables for supporting multiple GPUs.
- Example layer extension function shown.
- Layer extension advertised by xglGetExtension().
- xglEnumerateLayers() supports loader layer name queries and call interception
- Can be LD\_PRELOADed individually

layer/Multi.cpp (name=multi1:multi2) simple example showing multiple layers per library
    
(build dir)/layer/generic_layer.c (name=Generic) - auto generated example wrapping all XGL entrypoints. Single global dispatch table. Can be LD\_PRELOADed.

### Print API Calls and Parameter Values
(build dir)/layer/api_dump.c (name=APIDump) - print out API calls along with parameter values

(build dir)/layer/api_dump.cpp (name=APIDumpCpp) - same as above but uses c++ strings and i/o streams

(build dir)/layer/api\_dump\_file.c (name=APIDumpFile) - Write API calls along with parameter values to xgl\_apidump.txt file.

(build dir)/layer/api\_dump\_no\_addr.c (name=APIDumpNoAddr) - print out API calls along with parameter values but replace any variable addresses with the static string "addr".

(build dir)/layer/api\_dump\_no\_addr.cpp (name=APIDumpNoAddrCpp) - same as above but uses c++ strings and i/o streams

### Print Object Stats
(build dir>/layer/object_track.c (name=ObjectTracker) - Print object CREATE/USE/DESTROY stats. Individually track objects by category. XGL\_OBJECT\_TYPE enum defined in object_track.h. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout. Provides custom interface to query number of live objects of given type  "XGL\_UINT64 objTrackGetObjectCount(XGL\_OBJECT\_TYPE type)" and a secondary call to return an array of those objects "XGL\_RESULT objTrackGetObjects(XGL\_OBJECT\_TYPE type, XGL\_UINT64 objCount, OBJTRACK\_NODE* pObjNodeArray)".

### Report Draw State
layer/draw\_state.c (name=DrawState) - NOTE: This layer is currently non-functional due to a number of changes in xgl.h. When updated, it will report the Descriptor Set, Pipeline State, and dynamic state at each Draw call. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout. 

### Track GPU Memory
layer/mem\_tracker.c (name=MemTracker) - NOTE: This layer is currently non-functional due to a number of changes in xgl.h. When updated, it will track GPU Memory and any binding it has to objects and/or Cmd Buffers. Report issues with freeing memory, memory dependencies on Cmd Buffers, and any memory leaks at DestroyDevice time. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Check parameters
<build dir>/layer/param_checker.c (name=ParamChecker) - Check the input parameters to API calls for validity. Currently this only checks ENUM params directly passed to API calls and ENUMs embedded in struct params. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

## Using Layers

1. Build XGL loader  and i965 icd driver using normal steps (cmake and make)
2. Place libXGLLayer<name>.so in the same directory as your XGL test or app:

    cp build/layer/libXGLLayerBasic.so build/layer/libXGLLayerGeneric.so build/tests

    This is required for the Icd loader to be able to scan and enumerate your library. Alternatively, use the LIBXGL\_LAYERS\_PATH environment variable to specify where the layer libraries reside.

3. Specify which Layers to activate by using 
xglCreateDevice XGL\_LAYER\_CREATE\_INFO struct or environment variable LIBXGL\_LAYER\_NAMES

    export LIBXGL\_LAYER\_NAMES=Basic:Generic
    cd build/tests; ./xglinfo

## Tips for writing new layers

1. Must implement xglGetProcAddr() (aka GPA);
2. Must have a local dispatch table to call next layer (see xglLayer.h);
3. Should implement xglEnumerateLayers() returning layer name when gpu == NULL; otherwise layer name is extracted from library filename by the Loader;
4. gpu objects must be unwrapped (gpu->nextObject) when passed to next layer;
5. next layers GPA can be found in the wrapped gpu object;
6. Loader calls a layer's GPA first  so initialization should occur here;
7. all entrypoints can be wrapped but only will be called after layer is activated
    via the first xglCreatDevice;
8. entrypoint names can be any name as specified by the layers xglGetProcAddr
    implementation; exceptions are xglGetProcAddr and xglEnumerateLayers,
    which must have the correct name since the Loader calls these entrypoints;
9. entrypoint names must be exported to the dynamic loader with XGL\_LAYER\_EXPORT;
10. For LD\_PRELOAD support: a)entrypoint names should be offical xgl names and
    b) initialization should occur on any call with a gpu object (Loader type
    initialization must be done if implementing xglInitAndEnumerateGpus).
11. Implement xglGetExtension() if you want to advertise a layer extension
    (only available after the layer is activated);
12. Layer naming convention is camel case same name as in library: libXGLLayer<name>.so
13. For multiple layers in one library should implement a separate GetProcAddr for each
    layer and export them to dynamic loader;  function name is <layerName>GetProcAddr().
    Main xglGetProcAddr() should also be implemented.

## Status

### Current Features

- scanning of available Layers during xglInitAndEnumerateGpus;
- layer names retrieved via xglEnumerateLayers();
- xglEnumerateLayers and xglGetProcAddr supported APIs in xgl.h, ICD loader and i965 driver;
- multiple layers in a hierarchy supported;
- layer enumeration supported per GPU;
- layers activated per gpu and per icd driver: separate  dispatch table and layer library list in loader for each gpu or icd driver;
- activation via xglCreateDevice extension struct in CreateInfo or via env var (LIBXGL\_LAYER\_NAMES);
- layer libraries can be LD\_PRELOADed if implemented correctly;

### Current known issues

- DrawState and MemTracker layers are both non-functional due to updates to xgl.h. Work is ongoing to bring these layers up to latest header.
- Layers with multiple threads are not well tested and some layers likely to have issues. APIDump family of layers should be thread-safe.
- layer libraries (except Basic) don't support multiple dispatch tables for multi-gpus;
- layer libraries not yet include loader init functionality for full LD\_PRELOAD of entire API including xglInitAndEnumerateGpus;
- Since Layers aren't activated until xglCreateDevice, any calls to xglGetExtension() will not report layer extensions unless implemented in the layer;
- layer extensions do NOT need to be enabled in xglCreateDevice to be available;
- no support for apps registering layers, must be discovered via initial scan

