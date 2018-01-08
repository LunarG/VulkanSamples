# Vulkan Ecosystem Components

This project provides the Khronos official Vulkan ICD desktop loader and the Vulkan validation layers for Windows, Linux, and Android.

## CI Build Status
| Platform | Build Status |
|:--------:|:------------:|
| Linux/Android | [![Build Status](https://travis-ci.org/KhronosGroup/Vulkan-LoaderAndValidationLayers.svg?branch=master)](https://travis-ci.org/KhronosGroup/Vulkan-LoaderAndValidationLayers) |
| Windows |[![Build status](https://ci.appveyor.com/api/projects/status/ri4584d6qramrjiv/branch/master?svg=true)](https://ci.appveyor.com/project/Khronoswebmaster/vulkan-loaderandvalidationlayers/branch/master) |


## Introduction

Vulkan is an Explicit API, enabling direct control over how GPUs actually work. By design, minimal error checking is done inside
a Vulkan driver. Applications have full control and responsibility for correct operation. Any errors in
how Vulkan is used can result in a crash. This project provides Vulkan validation layers that can be enabled
to assist development by enabling developers to verify their applications correct use of the Vulkan API.

Vulkan supports multiple GPUs and multiple global contexts (VkInstance). The ICD loader is necessary to
support multiple GPUs and VkInstance-level Vulkan commands.  Additionally, the loader manages inserting
Vulkan layer libraries such as validation layers between the application and the ICD.

The following components are available in this repository:
- [Vulkan header files](include/vulkan/)
- [*ICD Loader*](loader/)
- [*Validation Layers*](layers/)
- [*Mock ICD*](icd/)
- [*Demos*](demos/)
- [*Tests*](tests/)

## Contact Information
* [Tobine Ehlis](mailto:tobine@google.com)
* [Mark Lobodzinski](mailto:mark@lunarg.com)

## Information for Developing or Contributing:

Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file in this repository for more details.
Please see the [GOVERNANCE.md](GOVERNANCE.md) file in this repository for repository management details.

## How to Build and Run

[BUILD.md](BUILD.md)
Includes directions for building all components as well as running validation tests and demo applications.

Information on how to enable the various Validation layers is in
[layers/README.md](layers/README.md).

Architecture and interface information for the loader is in
[loader/LoaderAndLayerInterface.md](loader/LoaderAndLayerInterface.md).

## License
This work is released as open source under a Apache-style license from Khronos including a Khronos copyright.

See COPYRIGHT.txt for a full list of licenses used in this repository.

## Acknowledgements
While this project has been developed primarily by LunarG, Inc., there are many other
companies and individuals making this possible: Valve Corporation, funding
project development; Google providing significant contributions to the validation layers;
Khronos providing oversight and hosting of the project.
