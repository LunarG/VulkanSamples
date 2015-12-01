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
or VkPhysicalDevice as first parameter.  Device level entry points are those with VkDevice, VkCommandBuffer,
or VkQueue as the first parameter. Layers that want to intercept both instance and device
level entrypoints are called Global Layers. vkXXXXGetProcAddr is used internally by the Layers and
Loader to initialize dispatch tables. Device Layers are activated at vkCreateDevice time. Instance
Layers are activated at vkCreateInstance time.  Layers can also be activated via environment variables
(VK_INSTANCE_LAYERS or VK_DEVICE_LAYERS).

All validation layers work with the DEBUG_REPORT extension to provide the application or user with
validation feedback. When a validation layer is enabled, it will look at the vk_layer_settings.txt
file to determine its behavior. Such as outputing to a file, stdout or debug output (Windows). An
application can also register callback functions via the DEBUG_REPORT extension to receive callbacks
when the requested validation events happen. Application callbacks happen regardless of the
settings in vk_layer_settings.txt

### Layer library example code

Note that some layers are code-generated and will therefore exist in the directory (build_dir)/layers

-include/vkLayer.h  - header file for layer code.

### Templates
layers/basic.cpp (name=VK_LAYER_LUNARG_Basic) simple example wrapping a few entrypoints. Shows layer features:
- Multiple dispatch tables for supporting multiple GPUs.
- Example layer extension function shown.
- Layer extension advertised by vkGetXXXExtension().

layers/multi.cpp (name=VK_LAYER_LUNARG_multi1:VK_LAYER_LUNARG_multi2) simple example showing multiple layers per library

(build dir)/layer/generic_layer.cpp (name=VK_LAYER_LUNARG_Generic) - auto generated example wrapping all VK entrypoints.

### Layer Details
For complete details of current validation layers, including all of the validation checks that they perform, please refer to the document layers/vk_validation_layer_details.md. Below is a brief overview of each layer.

### Print API Calls and Parameter Values
(build dir)/layers/api_dump.cpp (name=VK_LAYER_LUNARG_APIDump) - print out API calls along with parameter values

### Print Object Stats
(build dir)/layers/object_track.cpp (name=VK_LAYER_LUNARG_ObjectTracker) - Track object creation, use, and destruction. As objects are created, they're stored in a map. As objects are used, the layer verifies they exist in the map, flagging errors for unknown objects. As objects are destroyed, they're removed from the map. At vkDestroyDevice() and vkDestroyInstance() times, if any objects have not been destroyed, they are reported as leaked objects. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Validate Draw State and Shaders
layers/draw\_state.cpp (name=VK_LAYER_LUNARG_DrawState) - DrawState tracks the Descriptor Set, Pipeline State, Shaders, and dynamic state performing some point validation as states are created and used, and further validation at each Draw call. Of primary interest is making sure that the resources bound to Descriptor Sets correctly align with the layout specified for the Set. Additionally DrawState include sharder validation (formerly separate ShaderChecker layer) that inspects the SPIR-V shader images and fixed function pipeline stages at PSO creation time. It flags errors when inconsistencies are found across interfaces between shader stages. The exact behavior of the checks depends on the pair of pipeline stages involved. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Track GPU Memory
layers/mem\_tracker.cpp (name=VK_LAYER_LUNARG_MemTracker) - The MemTracker layer tracks memory objects and references and validates that they are managed correctly by the application.  This includes tracking object bindings, memory hazards, and memory object lifetimes. MemTracker validates several other hazard-related issues related to command buffers, fences, and memory mapping. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Check parameters
layers/param_checker.cpp (name=VK_LAYER_LUNARG_ParamChecker) - Check the input parameters to API calls for validity. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Image parameters
layers/image.cpp (name=VK_LAYER_LUNARG_Image) - The Image layer is intended to validate image parameters, formats, and correct use. Images are a significant enough area that they were given a separate layer. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Check threading
<build dir>/layers/threading.cpp (name=VK_LAYER_LUNARG_Threading) - Check multithreading of API calls for validity. Currently this checks that only one thread at a time uses an object in free-threaded API calls. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Swapchain
<build dir>/layer/swapchain.cpp (name=VK_LAYER_LUNARG_Swapchain) - Check that WSI extensions are being used correctly.

### Device Limitations
layers/device_limits.cpp (name=VK_LAYER_LUNARG_DeviceLimits) - This layer is intended to capture underlying device features and limitations and then flag errors if an app makes requests for unsupported features or exceeding limitations. This layer is a work in progress and currently only flags some high-level errors without flagging errors on specific feature and limitation. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

## Using Layers

1. Build VK loader and i965 icd driver using normal steps (cmake and make)
2. Place libVKLayer<name>.so in the same directory as your VK test or app:

    cp build/layer/libVKLayerBasic.so build/layer/libVKLayerGeneric.so build/tests

    This is required for the Loader to be able to scan and enumerate your library.
    Alternatively, use the VK\_LAYER\_PATH environment variable to specify where the layer libraries reside.

3. Create a vk_layer_settings.txt file in the same directory to specify how your layers should behave.

    Model it after the following example:  [*vk_layer_settings.txt*](layers/vk_layer_settings.txt)

4. Specify which Layers to activate by using
vkCreateDevice and/or vkCreateInstance or environment variables.

    export VK\_INSTANCE\_LAYERS=VK_LAYER_LUNARG_Basic:VK_LAYER_LUNARG_Generic
    export VK\_DEVICE\_LAYERS=VK_LAYER_LUNARG_Basic:VK_LAYER_LUNARG_Generic
    cd build/tests; ./vkinfo

## Tips for writing new layers

1. Must implement vkGetInstanceProcAddr() (aka GIPA) and vkGetDeviceProcAddr() (aka GDPA);
2. Must have a local dispatch table to call next layer (see vk_layer.h);
3. Must have a layer manifest file for each Layer library for Loader to find layer properties (see loader/README.md)
4. Next layers GXPA can be found in the wrapped instance or device object;
5. Loader calls a layer's GXPA first so initialization should occur here;
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

- Layers with multiple layers per library: the manifest file parsing in Loader doesn't yet handle this
- multi.cpp Layer needs rewrite to allow manifest file to specify multiple layers
- multi1  and multi2 layers from multi.cpp: only multi1 layer working
