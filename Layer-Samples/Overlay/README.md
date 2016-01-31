Overlay layer example

This is a little different from the other samples in that it is implemented as a layer, rather than a vulkan client app. This carries some extra requirements:

- A colocated LoaderAndTools (LoaderAndValidationLayers) tree is required at build time. Some required components are not included in the SDK.
- Build directory should be added to VK_LAYER_PATH.
- The overlay layer name (currently "Overlay") should be added to VK_INSTANCE_LAYERS and VK_DEVICE_LAYERS.
