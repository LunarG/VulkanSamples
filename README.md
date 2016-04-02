<<<<<<< 87a50288ed2bf1bc8a9a07f7ec9a8531a579fc2a
# Vulkan Samples Kit
  - This repository is a collection of Vulkan C++ sample applications.
  - Run the following script to obtain a short description of all or a 
    specific sample:
    `$ src/get-short-descripts.sh`
  - Run the following script to obtain a more detailed description of all
    samples with a long description set:
    `$ src/get-descripts.sh`

## Requirements/Dependencies
  - A recent version of the LunarG Vulkan SDK must be installed on the system.
    In addition to the Vulkan headers and libraries, the SDK also includes the
    glslang and spirv-tools components necessary to build the sample progams.
  - Linux package dependencies:
    - Vulkan SDK required packages
    - cmake
  - Windows dependencies include:
    - Vulkan SDK required components
    - cmake
  - Note that if you have a LoaderAndValidationLayers repository,
    and/or a glslang repository that are file system peers to VulkanSamplesKit,
    cmake will look in those repositories for headers and libraries before
    looking in SDK or system locations.  Build directories in those repositories
    should be named "build".


## Kit Structure
  - The Vulkan Samples Kit is a set of source and data files in a specific
    directory hierarchy:
      - API-Samples - Samples that demonstrate the use of various aspects of the
        Vulkan API
      - Demos - Samples that are more functional and go deeper than simple API use.
      - Layer-Samples - Samples that are implemented as layers.
      - Android-Only - Samples that are only meant to be run on Android

## Sample progression
  - In general, the samples are not interrelated, but there is a progression
      among some of the samples that lead to drawing a cube.  Start with the
      instance sample, then device, initcommandbuffer, initswapchain, initdepthbuffer,
      inituniformbuffer, descriptor_pipeline_layouts, initrenderpass, initshaders,
      initframebuffers, vertexbuffer, allocdescriptorsets, initpipeline, and they
      culminate in the drawcube sample.  Each sample uses utility routines from
      the code from previous samples to get to the point to show something new.
      The drawtexturedcube sample takes all of the drawcube code and adds texturing.

## Building the Vulkan Samples Kit
### Linux:
  ```
  $ cmake -H. -Bbuild
  $ make -C build
  ```

#### Android:
  ```
  $ cmake -DANDROID=ON -DANDROID_ABI=[armeabi-v7a|arm64-v8a| x86|x86_64|all(default)]
  ```
And import VulkanSamples/API-Samples/android/build.gradle in Android Studio.

### Windows:
#### 64-bit Windows

From a PowerShell window, first build glslang:
  ```
  PS > cd C:\VulkanSDK\<version>\glslang
  PS > mkdir build
  PS > cd build
  PS > cmake -G "Visual Studio 12 Win64" ..
  ```
  Open the glslang.sln file in the build folder with Microsoft Visual Studio and build the solution in both Debug and Release mode.

Next build spirv-tools:
  ```
  PS > cd C:\VulkanSDK\<version>\spirv-tools
  PS > mkdir build
  PS > cd build
  PS > cmake -G "Visual Studio 12 Win64" ..
  ```

  Open the spirv-tools.sln file in the build folder with Microsoft Visual Studio and build the solution in both Debug and Release mode.

Next build the Vulkan Samples:
  ```
  PS > cd C:\VulkanSDK\<version>\Samples
  PS > mkdir build
  PS > cd build
  PS > cmake -G "Visual Studio 12 Win64" ..
  ```
  Open the VULKAN_SAMPLES.sln file in the build folder with Microsoft Visual Studio and build the solution.

#### 32-bit Windows

Perform the same operations as 64-bit Windows above, except with the following adjustments:
  ```
  PS > mkdir build32
  PS > cd build32
  PS > cmake -G "Visual Studio 12" ..
  ```

## Contributing
  Refer to the README.contrib file for specific info regarding contributing to
  the Vulkan samples creation effort.

=======
# Vulkan Ecosystem Components
*Version 1.0, January 25, 2016*

This project provides Khronos official ICD loader and validation layers for Vulkan developers on Windows and Linux.

## Introduction

Vulkan is an Explicit API, enabling direct control over how GPUs actually work. No (or very little) validation
or error checking is done inside a Vulkan driver. Applications have full control and responsibility. Any errors in
how Vulkan is used often result in a crash. This project provides standard validation layers that can be enabled to ease development by 
helping developers verify their applications correctly use the Vulkan API.

Vulkan supports multiple GPUs and multiple global contexts (VkInstance). The ICD loader is necessary to support multiple GPUs  and the VkInstance level Vulkan commands.  Additionally, the loader manages inserting Vulkan layer libraries,
including validation layers between the application and the ICD.

The following components are available in this repository:
- Vulkan header files
- [*ICD Loader*](loader/)
- [*Validation Layers*](layers/)
- Demos and tests for the loader and validation layers


## How to Build and Run

[BUILD.md](BUILD.md)
includes directions for building all the components, running the validation tests and running the demo applications.

Information on how to enable the various Validation layers is in
[layers/README.md](layers/README.md).

Architecture and interface information for the loader is in
[loader/LoaderAndLayerInterface.md](loader/LoaderAndLayerInterface.md).

## License
This work is released as open source under a MIT-style license from Khronos including a Khronos copyright.

See LICENSE.txt for a full list of licenses used in this repository.

## Acknowledgements
While this project has been developed primarily by LunarG, Inc; there are many other
companies and individuals making this possible: Valve Corporation, funding
project development; Google providing significant contributions to the validation layers;
Khronos providing oversight and hosting of the project.


>>>>>>> docs: fix spelling mistakes
