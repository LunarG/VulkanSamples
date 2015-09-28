# Explicit GL (VK) Ecosystem Components
*Version 0.9, 28 Sep 2015*

This project provides *open source* tools for VK Developers.

## Introduction

VK is an Explicit API, enabling direct control over how GPUs actually work. No validation, shader recompilation, memory management or synchronization is done inside an VK driver. Applications have full control and responsibility. Any errors in how VK is used are likely to result in a crash. This project provides layered utility libraries to ease development and help guide developers to proven safe patterns.

New with VK in an extensible layered architecture that enables significant innovation in tools:
- Cross IHV support enables tools vendors to plug into a common, extensible layer architecture
- Layered tools during development enable validating, debugging and profiling without production performance impact
- Modular validation architecture encourages many fine-grained layers--and new layers can be added easily
- Encourages open community of tool developers: led by Valve, LunarG, Codeplay and others
- Customized interactive debugging and validation layers will be available together with first drivers

The components here are being shared with the Khronos community to provide
insights into the specification as we approach an alpha header, and to assists those doing
demos for GDC.

The following components are available:
- VK Library and header files, which include:
    - [*ICD Loader*](loader) and [*Layer Manager*](layers/README.md)
    - Snapshot of *VK* and *SPIR-V* header files from [*Khronos*](www.khronos.org)

- [*VKTRACE tools*](vktrace)

- Core [*Validation Layers*](layers/)

- [*Sample Driver*](icd)

## New

- Header matches provisional specification (v170) with two changes.
 - DrawIndirect correction (bug #14715)
 - DynamicState refactor (bug #14365)
- Provisional specification document (PDF & HTML)
- Loader now uses JSON manifest files for ICDs and layers. See BUILD.md for details on customizing loader behavior.
- All validation errors now reported via DEBUG_REPORT extension. [*DEBUG_REPORT*, Extension Document](TODO: link to VulkanDbgExtensions.docx)
- Warning: The sample driver requires DRI3 and recent versions of ubuntu 14.10 have **REMOVED** DRI 3.
  Version: 2:2.99.914-1~exp1ubuntu4.1 is known to work.
  Ubuntu 15.04 requires customization to add DRI3.
  See BUILD.md for details.

## Prior updates


## How to Build and Run

[BUILD.md](BUILD.md)
includes directions for building all the components, running the validation tests and running the demo applications.

Information on how to enable the various Debug and Validation layers is in
[layers/README.md](layers/README.md).

## References
This version of the components are written based on the following preliminary specs and proposals:
- [**Core Vulkan Header**, vulkan.h](https://gitlab.khronos.org/vulkan/vulkan/blob/6e1463d85b747fcad43c48eb8abd94d0f58824de/src/include/vulkan.h)
- [**SPIR-V**, revision 32](https://cvs.khronos.org/svn/repos/SPIRV/trunk/Promoter32)
- [**WSI Device Swapchain**, Revision 53 for VK_EXT_KHR_device_swapchain](https://cvs.khronos.org/svn/repos/promoters/specs/candidates/oglc/extensions/20150910/vk_ext_khr_device_swapchain.txt)
- [**WSI Swapchain**, Revision 17 for VK_EXT_KHR_swapchain](https://cvs.khronos.org/svn/repos/promoters/specs/candidates/oglc/extensions/20150910/vk_ext_khr_swapchain.txt)


## License
This work is intended to be released as open source under a BSD-style
license once the VK specification is public. Until that time, this work
is covered by the Khronos NDA governing the details of the VK API.

## Acknowledgements
While this project is being developed by LunarG, Inc; there are many other
companies and individuals making this possible: Valve Software, funding
project development; Intel Corporation, providing full hardware specifications
and valuable technical feedback; AMD, providing VK spec editor contributions;
ARM, contributing a Chairman for this working group within Khronos; Nvidia,
providing an initial co-editor for the spec; Qualcomm for picking up the
co-editor's chair; and Khronos, for providing hosting within GitHub.

## Contact
If you have questions or comments about this driver; or you would like to contribute
directly to this effort, please contact us at VK@LunarG.com; or if you prefer, via
the GL Common mailing list: gl_common@khronos.org
