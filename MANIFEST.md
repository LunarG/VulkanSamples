## A manifest of files in the VulkanSamples repository

### **The Repositories**

The source code for various Vulkan components is distributed across several GitHub repositories.  The repositories sponsored by Khronos and LunarG are described here.  In general, the canonical Vulkan Loader and Validation Layers sources are in the Khronos repository, while the LunarG repositories host sources for additional tools and sample programs.

* [Khronos Vulkan-LoaderAndValidationLayers](https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers)
* [LunarG VulkanTools](https://github.com/LunarG/VulkanTools)
* [LunarG VulkanSamples](https://github.com/LunarG/VulkanSamples)

As a convenience, the contents of the Vulkan-LoaderAndValidationLayers repository are downstreamed into the VulkanTools and VulkanSamples repositories via a branch named trunk.  This makes the VulkanTools and VulkanSamples easier to work with and avoids compatibility issues that might arise with Vulkan-LoaderAndValidationLayers components if they were obtained from a separate repository.

### **The Files**

As a general rule, if a file exists in Vulkan-LoaderAndValidationLayers (LVL), a copy of that file will be copied and added to this VulkanSamples (VS) repository.  The most obvious exception to that is the CMakeLists.txt file in the base directory of the VS repository which at the top is the LVL version with VS specific additions at the bottom.

The files specific to VS reside in 4 main directories:
>  API-Samples - Samples that show a basic use of the API, no frills, just Vulkan API calls
>  Layer-Samples - Samples that show how to implement layers that can augment or change the behavior of Vulkan programs
>  Sample-Programs - Samples that go beyond basic API calls and use Vulkan for more complex rendering
>  Utilities - Samples that can be used to assist in writing other Vulkan applications

Also specific to the VS repository is the samples index html page and its supporting images.

A non-exhaustive list of directories copied from LVL would be:
>  common
>  demos
>  include
>  layers
>  loader
>  scripts
>  tests
>  windowsRuntimeInstaller

Changes to files in these locations should be made in LVL, and they will propagate downstream into VS through regular trunk merges done in VS.
