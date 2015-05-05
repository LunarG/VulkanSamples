# Layer Description and Status

## Overview

Layer libraries can be written to intercept or hook VK entrypoints for various
debug and validation purposes.  One or more VK entrypoints can be defined in your Layer
library.  Undefined entrypoints in the Layer library will be passed to the next Layer which
may be the driver.  Multiple layer libraries can be chained (actually a hierarchy) together.
vkEnumerateLayer can be called to list the available layer libraries.  vkGetProcAddr is
used internally by the Layers and ICD Loader to initialize dispatch tables. Layers are
activated at vkCreateDevice time. vkCreateDevice createInfo struct is extended to allow
a list of layers to be activated.  Layer libraries can alternatively be LD\_PRELOADed depending
upon how they are implemented.

##Layer library example code

Note that some layers are code-generated and will therefore exist in the directory (build_dir)/layers

-include/vkLayer.h  - header file for layer code.

### Templates
layer/Basic.cpp (name=Basic) simple example wrapping a few entrypoints. Shows layer features:
- Multiple dispatch tables for supporting multiple GPUs.
- Example layer extension function shown.
- Layer extension advertised by vkGetExtension().
- vkEnumerateLayers() supports loader layer name queries and call interception
- Can be LD\_PRELOADed individually

layer/Multi.cpp (name=multi1:multi2) simple example showing multiple layers per library
    
(build dir)/layer/generic_layer.c (name=Generic) - auto generated example wrapping all VK entrypoints. Single global dispatch table. Can be LD\_PRELOADed.

### Print API Calls and Parameter Values
(build dir)/layer/api_dump.c (name=APIDump) - print out API calls along with parameter values

(build dir)/layer/api_dump.cpp (name=APIDumpCpp) - same as above but uses c++ strings and i/o streams

(build dir)/layer/api\_dump\_file.c (name=APIDumpFile) - Write API calls along with parameter values to vk\_apidump.txt file.

(build dir)/layer/api\_dump\_no\_addr.c (name=APIDumpNoAddr) - print out API calls along with parameter values but replace any variable addresses with the static string "addr".

(build dir)/layer/api\_dump\_no\_addr.cpp (name=APIDumpNoAddrCpp) - same as above but uses c++ strings and i/o streams

### Print Object Stats
(build dir>/layer/object_track.c (name=ObjectTracker) - Print object CREATE/USE/DESTROY stats. Individually track objects by category. VkObjectType enum defined in vulkan.h. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout. Provides custom interface to query number about the total number of objects or of live objects of given type.  To get information on all objects, use  "VK\_UINT64 objTrackGetObjectsCount()" and the secondary call to return an array of those objects "VK\_RESULT objTrackGetObjects(VK\_UINT64 objCount, OBJTRACK\_NODE\* pObjNodeArray)". For objects of a specific type, use  "VK\_UINT64 objTrackGetObjectsOfTypeCount(VkObjectType type)" and the secondary call to return an array of those objects "VK\_RESULT objTrackGetObjectsOfType(VK\_OBJECT\_TYPE type, VK\_UINT64 objCount, OBJTRACK\_NODE\* pObjNodeArray)".

### Report Draw State
layer/draw\_state.c (name=DrawState) - DrawState reports the Descriptor Set, Pipeline State, and dynamic state at each Draw call. DrawState layer performs a number of validation checks on this state. Of primary interest is making sure that the resources bound to Descriptor Sets correctly align with the layout specified for the Set. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout. 

### Track GPU Memory
layer/mem\_tracker.c (name=MemTracker) - MemTracker functions mostly as a validation layer, attempting to ensure that memory objects are managed correctly by the application.  These memory objects are bound to pipelines, objects, and command buffers, and then submitted to the GPU for work.  As an example, the layer validates that the correct memory objects have been bound, and that they are specified correctly when the command buffers are submitted.  Also, that only existing memory objects are referenced, and that any destroyed memory objects are not referenced.  Another type of validation done is that before any memory objects are reused or destroyed, the layer ensures that the application has confirmed that they are no longer in use, and that they have been properly unbound before destruction. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Check parameters
<build dir>/layer/param_checker.c (name=ParamChecker) - Check the input parameters to API calls for validity. Currently this only checks ENUM params directly passed to API calls and ENUMs embedded in struct params. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Check threading
<build dir>/layer/threading.c (name=Threading) - Check multithreading of API calls for validity. Currently this checks that only one thread at a time uses an object in free-threaded API calls. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

## Using Layers

1. Build VK loader  and i965 icd driver using normal steps (cmake and make)
2. Place libVKLayer<name>.so in the same directory as your VK test or app:

    cp build/layer/libVKLayerBasic.so build/layer/libVKLayerGeneric.so build/tests

    This is required for the Icd loader to be able to scan and enumerate your library. Alternatively, use the LIBVK\_LAYERS\_PATH environment variable to specify where the layer libraries reside.

3. Specify which Layers to activate by using 
vkCreateDevice VK\_LAYER\_CREATE\_INFO struct or environment variable LIBVK\_LAYER\_NAMES

    export LIBVK\_LAYER\_NAMES=Basic:Generic
    cd build/tests; ./vkinfo

## Tips for writing new layers

1. Must implement vkGetProcAddr() (aka GPA);
2. Must have a local dispatch table to call next layer (see vkLayer.h);
3. Should implement vkEnumerateLayers() returning layer name when gpu == NULL; otherwise layer name is extracted from library filename by the Loader;
4. gpu objects must be unwrapped (gpu->nextObject) when passed to next layer;
5. next layers GPA can be found in the wrapped gpu object;
6. Loader calls a layer's GPA first  so initialization should occur here;
7. all entrypoints can be wrapped but only will be called after layer is activated
    via the first vkCreatDevice;
8. entrypoint names can be any name as specified by the layers vkGetProcAddr
    implementation; exceptions are vkGetProcAddr and vkEnumerateLayers,
    which must have the correct name since the Loader calls these entrypoints;
9. entrypoint names must be exported to the dynamic loader with VK\_LAYER\_EXPORT;
10. For LD\_PRELOAD support: a)entrypoint names should be offical vk names and
    b) initialization should occur on any call with a gpu object (Loader type
    initialization must be done if implementing vkInitAndEnumerateGpus).
11. Implement vkGetExtension() if you want to advertise a layer extension
    (only available after the layer is activated);
12. Layer naming convention is camel case same name as in library: libVKLayer<name>.so
13. For multiple layers in one library should implement a separate GetProcAddr for each
    layer and export them to dynamic loader;  function name is <layerName>GetProcAddr().
    Main vkGetProcAddr() should also be implemented.

## Status

### Current Features

- scanning of available Layers during vkInitAndEnumerateGpus;
- layer names retrieved via vkEnumerateLayers();
- vkEnumerateLayers and vkGetProcAddr supported APIs in vulkan.h, ICD loader and i965 driver;
- multiple layers in a hierarchy supported;
- layer enumeration supported per GPU;
- layers activated per gpu and per icd driver: separate  dispatch table and layer library list in loader for each gpu or icd driver;
- activation via vkCreateDevice extension struct in CreateInfo or via env var (LIBVK\_LAYER\_NAMES);
- layer libraries can be LD\_PRELOADed if implemented correctly;

### Current known issues

- Layers with multiple threads are not well tested and some layers likely to have issues. APIDump family of layers should be thread-safe.
- layer libraries (except Basic) don't support multiple dispatch tables for multi-gpus;
- layer libraries not yet include loader init functionality for full LD\_PRELOAD of entire API including vkInitAndEnumerateGpus;
- Since Layers aren't activated until vkCreateDevice, any calls to vkGetExtension() will not report layer extensions unless implemented in the layer;
- layer extensions do NOT need to be enabled in vkCreateDevice to be available;
- no support for apps registering layers, must be discovered via initial scan

