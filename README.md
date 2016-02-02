# Vulkan Ecosystem Components
*Version 1.0, January 25, 2016*

This project provides loader and validation layers for Vulkan developers on Windows and Linux.

## Introduction

Vulkan is an Explicit API, enabling direct control over how GPUs actually work. No (or very little) validation or error checking is done inside a VK driver. Applications have full control and responsibility. Any errors in how VK is used are likely to result in a crash. This project provides layered utility libraries to ease development and help guide developers to proven safe patterns.

New with Vulkan is an extensible layered architecture that enables validation libraries to be implemented as layers. The loader is essential in supporting multiple drivers and GPUs along with layer library enablement.

The following components are available in this repository:
- Vulkan header files
- [*ICD Loader*](loader) and [*Layer Manager*](layers/README.md, loader/README.md
- Core [*Validation Layers*](layers/)
- Demos and tests for the loader and validation layers


## How to Build and Run

[BUILD.md](BUILD.md)
includes directions for building all the components, running the validation tests and running the demo applications.

Information on how to enable the various Validation layers is in
[layers/README.md](layers/README.md).


## License
This work is intended to be released as open source under a MIT-style
license once the Vulkan specification is public. Until that time, this work
is covered by the Khronos NDA governing the details of the VK API.

## Acknowledgements
While this project is being developed by LunarG, Inc; there are many other
companies and individuals making this possible: Valve Software, funding
project development; Intel Corporation, providing full hardware specifications
and valuable technical feedback; AMD, providing VK spec editor contributions;
ARM, contributing a Chairman for this working group within Khronos; Nvidia,
providing an initial co-editor for the spec; Qualcomm for picking up the
co-editor's chair; and Khronos, for providing hosting within GitHub.

