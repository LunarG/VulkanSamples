# Vulkan Ecosystem Components

This project provides Khronos official ICD loader and validation layers for Vulkan developers on Windows and Linux.

## Introduction

Vulkan is an Explicit API, enabling direct control over how GPUs actually work. No (or very little) validation
or error checking is done inside a Vulkan driver. Applications have full control and responsibility. Any errors in
how Vulkan is used often result in a crash. This project provides standard validation layers that can be enabled
to ease development by helping developers verify their applications correctly use the Vulkan API.

Vulkan supports multiple GPUs and multiple global contexts (VkInstance). The ICD loader is necessary to
support multiple GPUs  and the VkInstance level Vulkan commands.  Additionally, the loader manages inserting
Vulkan layer libraries, including validation layers between the application and the ICD.

The following components are available in this repository:
- Vulkan header files
- [*ICD Loader*](loader/)
- [*Validation Layers*](layers/)
- Demos and tests for the loader and validation layers

## Contributing

If you intend to contribute, the preferred work flow is for you to develop your contribution
in a fork of this repo in your GitHub account and then submit a pull request.
Please see the [CONTRIBUTING](CONTRIBUTING.md) file in this respository for more details

## How to Build and Run

[BUILD.md](BUILD.md)
includes directions for building all the components, running the validation tests and running the demo applications.

Information on how to enable the various Validation layers is in
[layers/README.md](layers/README.md).

Architecture and interface information for the loader is in
[loader/LoaderAndLayerInterface.md](loader/LoaderAndLayerInterface.md).

## License
This work is released as open source under a Apache-style license from Khronos including a Khronos copyright.

See COPYRIGHT.txt for a full list of licenses used in this repository.

## Acknowledgements
While this project has been developed primarily by LunarG, Inc; there are many other
companies and individuals making this possible: Valve Corporation, funding
project development; Google providing significant contributions to the validation layers;
Khronos providing oversight and hosting of the project.


