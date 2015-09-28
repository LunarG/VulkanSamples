# Layer Description and Status

## Overview

Layer libraries can be written to intercept or hook VK entry points for various
debug and validation purposes.  One or more VK entry points can be defined in your Layer
library.  Undefined entrypoints in the Layer library will be passed to the next Layer which
may be the driver.  Multiple layer libraries can be chained (actually a hierarchy) together.
vkEnumerateInstanceLayerProperties and vkEnumerateDeviceLayerProperties can be called to list the
available layers and their properties. Layers can intercept Vulkan instance level entry points
in which case they are called an Instance Layer.  Layers can intercept device entry  points
in which case they are called a Device Layer. Instance level entry points are those with VkInstance
or VkPhysicalDevice as first parameter.  Device level entry points are those with VkDevice, VkCmdBuffer,
or VkQueue as the first parameter. Layers that want to intercept both instance and device
level entrypoints are called Global Layers. vkXXXXGetProcAddr is used internally by the Layers and
Loader to initialize dispatch tables. Device Layers are activated at vkCreateDevice time. Instance
Layers are activated at vkCreateInstance.  Layers can also be activated via environment variables
(VK_INSTANCE_LAYERS or VK_DEVICE_LAYERS).

All validation layers work with the DEBUG_REPORT extension to provide the application or user with
validation feedback. When a validation layer is enabled, it will look at the vk_layer_settings.txt
file to determine it's behavior. Such as outputing to a file, stdout or debug output (Windows). An
application can also register callback functions via the DEBUG_REPORT extension to receive callbacks
when the requested validation events happen. Application callbacks happen regardless of the
settings in vk_layer_settings.txt

##Layer library example code

Note that some layers are code-generated and will therefore exist in the directory (build_dir)/layers

-include/vkLayer.h  - header file for layer code.

### Templates
layer/Basic.cpp (name=Basic) simple example wrapping a few entrypoints. Shows layer features:
- Multiple dispatch tables for supporting multiple GPUs.
- Example layer extension function shown.
- Layer extension advertised by vkGetXXXExtension().

layer/Multi.cpp (name=multi1:multi2) simple example showing multiple layers per library
    
(build dir)/layer/generic_layer.cpp (name=Generic) - auto generated example wrapping all VK entrypoints.

### Print API Calls and Parameter Values
(build dir)/layer/api_dump.cpp (name=APIDump) - print out API calls along with parameter values

### Print Object Stats
(build dir)/layer/object_track.cpp (name=ObjectTracker) - Track object CREATE/USE/DESTROY stats. Individually track objects by category. VkObjectType enum defined in vulkan.h.

### Report Draw State
layer/draw\_state.cpp (name=DrawState) - DrawState reports the Descriptor Set, Pipeline State, and dynamic state at each Draw call. DrawState layer performs a number of validation checks on this state. Of primary interest is making sure that the resources bound to Descriptor Sets correctly align with the layout specified for the Set.

### Track GPU Memory
layer/mem\_tracker.cpp (name=MemTracker) - MemTracker functions mostly as a validation layer, attempting to ensure that memory objects are managed correctly by the application.  These memory objects are bound to pipelines, objects, and command buffers, and then submitted to the GPU for work.  As an example, the layer validates that the correct memory objects have been bound, and that they are specified correctly when the command buffers are submitted.  Also, that only existing memory objects are referenced, and that any destroyed memory objects are not referenced.  Another type of validation done is that before any memory objects are reused or destroyed, the layer ensures that the application has confirmed that they are no longer in use, and that they have been properly unbound before destruction.

### Device Limits
<build dir>/layer/device_limits.cpp (name=DeviceLimits) - Check that parameters used do not exceed device limits.

### Check parameters
<build dir>/layer/param_checker.cpp (name=ParamChecker) - Check the input parameters to API calls for validity. Currently this only checks ENUM params directly passed to API calls and ENUMs embedded in struct params.

### Image Checker
<build dir>/layer/image.cpp (name=Image) - Verify parameters on Vulkan calls that use VkImage.

### Check threading
<build dir>/layer/threading.cpp (name=Threading) - Check multithreading of API calls for validity. Currently this checks that only one thread at a time uses an object in free-threaded API calls.

### Swapchain
<build dir>/layer/swapchain.cpp (name=Swapchain) - Check that WSI extensions are being used correctly.

### Shader Checker
<build dir>/layer/shader_checker.cpp (name=ShaderChecker) - Verify SPIR-V shader layout matches descriptor set.

## Using Layers

1. Build VK loader  and i965 icd driver using normal steps (cmake and make)
2. Place libVKLayer<name>.so in the same directory as your VK test or app:

    cp build/layer/libVKLayerBasic.so build/layer/libVKLayerGeneric.so build/tests

    This is required for the Loader to be able to scan and enumerate your library.
    Alternatively, use the VK\_LAYER\_PATH environment variable to specify where the layer libraries reside.

3. Specify which Layers to activate by using 
vkCreateDevice and/or vkCreateInstance or environment variables.

    export VK\_INSTANCE\_LAYERS=Basic:Generic
    export VK\_DEVICE\_LAYERS=Basic:Generic
    cd build/tests; ./vkinfo

## Tips for writing new layers

1. Must implement vkGetInstanceProcAddr() (aka GIPA) and vkGetDeviceProcAddr() (aka GDPA);
2. Must have a local dispatch table to call next layer (see vkLayer.h);
3. Must have a layer manifest file for each Layer library for Loader to find layer properties (see loader/README.md)
4. next layers GXPA can be found in the wrapped instance or device object;
5. Loader calls a layer's GXPA first  so initialization should occur here;
6. all entrypoints can be wrapped but only will be called after layer is activated
    via the first vkCreatDevice or vkCreateInstance;
7. entrypoint names can be any name as specified by the layers vkGetXXXXXProcAddr
    implementation; exceptions are vkGetXXXXProcAddr,
    which must have the correct name since the Loader calls these entrypoints;
8. entrypoint names must be exported to the OSes dynamic loader with VK\_LAYER\_EXPORT;
9. Layer naming convention is camel case same name as in library: libVKLayer<name>.so
10. For multiple layers in one library the manifest file can specify each layer.

## Status


### Current known issues

- Layers with multiple layers per library the manifest file parsing in Loader doesn't yet handle this;
- multi.cpp Layer needs rewrite to allow manifest file to specify multiple layers
- multi1  and multi2 layers from multi.cpp: only multi1 layer working


