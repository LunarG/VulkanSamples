Layer Description and Status
10/22/2014

Overview:
Layer libraries can be written to intercept or hook XGL entrypoints for various
debug and validation purpose.  One or more XGL entrypoints can be defined in your Layer
library.  Undefined entrypoints in the Layer library will be passed to the next Layer which
may be the driver.  Multiple layer libraries can be chained (actually a hierarchy) together.
xglEnumerateLayer can be called to list the available layer libraries.  xglGetProcAddr is
used internally by the Layers and ICD Loader to initialize dispatch tables. Layers are
activated at xglCreateDevice time. xglCreateDevice createInfo struct is extended to allow
a list of layers to be activated.  Layer libraries can alternatively be LD_PRELOADed.

Layer library example code:
include/xglLayer.h  - header file for layer code
layer/basic_plugin.c  - simple example wrapping three entrypoints. Single global dispatch
                        table for either single gpu/device or multi-gpu with same activated
                        layers for each device. Can be LD_PRELOADed individually.
<build dir>/layer/generic_layer.c  - auto generated example wrapping all XGL entrypoints.
                                     Single global dispatch table. Can be LD_PRELOADed.
<build dir>/layer/api_dump.c - print out API calls along with parameter values
<build dir>/layer/object_track.c - Print object CREATE/USE/DESTROY stats

Using Layers:
1) Build XGL loader  and i965 icd driver using normal steps (cmake and make)

2) Place libXGLLayer<name>.so in the same directory as your XGL test or app:
  cp build/layer/libXGLLayerBasic.so build/layer/libXGLLayerGeneric.so build/tests
This is required for the Icd loader to be able to scan and enumerate your library.

3) Specify which Layers to activate by using xglCreateDevice XGL_LAYER_CREATE_INFO struct or
environment variable LIBXGL_LAYER_LIBS
   export LIBXGL_LAYER_LIBS=libXGLLayerBasic.so:LibXGLLayerGeneric.so
   cd build/tests; ./xglinfo


Status:
Current Features:
-scanning of available Layers during xglInitAndEnumerateGpus
-xglEnumerateLayers and xglGetProcAddr supported APIs in xgl.h, ICD loader and i965 driver
-multiple layers in a hierarchy supported
-layer enumeration supported
-layers  activated per gpu and per icd driver: separate  dispatch table and layer library
   list in loader for each gpu or icd driver
-activation via xglCreateDevice extension struct in CreateInfo or via env var
   (LIBXGL_LAYER_LIBS)
-layer libraries can be LD_PRELOADed

Current known issues:
-layer libraries don't support multiple dispatch tables for multi-gpus
-layers with extension APIs not yet tested or supported
-layer libraries not yet include loader init functionality for full LD_PRELOAD of
    entire API including xglInitAndEnumerate
-no support for apps registering layers, must be discovered via initial scan
-no support for Loader discovering from layer and driver which layers support which
  gpus/drivers: any layer can be use any gpu right now
-xglEnumerateLayers doesn't qualify Layers based on gpu, but enumerates all that were scanned

