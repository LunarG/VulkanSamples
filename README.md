# Explicit GL (XGL) Ecosystem Components
*Version 0.8, 04 Feb 2015*

This project provides *open source* tools for XGL Developers.

## Introduction

XGL is an Explicit API, enabling direct control over how GPUs actually work. No validation, shader recompilation, memory management or synchronization is done inside an XGL driver. Applications have full control and responsibility. Any errors in how XGL is used are likely to result in a crash. This project provides layered utility libraries to ease development and help guide developers to proven safe patterns.

New with XGL in an extensible layered architecture that enables significant innovation in tools:
- Cross IHV support enables tools vendors to plug into a common, extensible layer architecture
- Layered tools during development enable validating, debugging and profiling without production performance impact
- Modular validation architecture encourages many fine-grained layers--and new layers can be added easily
- Encourages open community of tool developers: led by Valve, LunarG, Codeplay and others
- Customized interactive debugging and validation layers will be available together with first drivers

The components here are being shared with the Khronos community to provide
insights into the specification as we approach an alpha header, and to assists those doing
demos for GDC.

The following components are available:
- XGL Library and header files, which include:
    - [*ICD Loader*](loader) and [*Layer Manager*](layers/README.md)
    - Snapshot of *XGL* and *BIL* header files from [*Khronos*](www.khronos.org)
    
- [*GLAVE Debugger*](tools/glave)

    ![ScreenShot](docs/images/Glave-Small.png)

- Core [*Validation Layers*](layers/)

- [*Sample Driver*](icd)

## New

- Updated loader, driver, demos, tests and many tools to use "alpha" xgl.h (~ version 47).
  Supports new resource binding model, memory allocation, pixel FORMATs and
  other updates.
  APIDump layer is working with these new API elements.
  Glave can trace and replay the cube and tri demos.
  Other layers in progress.

## Prior updates

- XGL API trace and capture tools. See tools/glave/README.md for details.
- Sample driver now supports multiple render targets. Added TriangleMRT to test that functionality.
- Added XGL_SLOT_SHADER_TEXTURE_RESOURCE to xgl.h as a descriptor slot type to work around confusion in GLSL
  between textures and buffers as shader resources.
- Misc. fixes for layers and Intel sample driver
- Added mutex to APIDump, APIDumpFile and DrawState to prevent apparent threading issues using printf
- Fix support for {Fill,Copy}Memory
- MemTracker can report issues to application via debug callback
- Update test infrastructure to improve ease of writing new tests. Add image comparison feature for regression testing. Requires ImageMagick library.
- Misc. fixes to demos, layers and Intel sample driver
- Added mutex to APIDump, APIDumpFile and DrawState to prevent apparent threading issues using printf
- Fix support for {Fill,Copy}Memory
- MemTracker can report issues to application via debug callback
- Update test infrastructure to improve ease of writing new tests. Add image comparison feature for regression testing. Requires ImageMagick library.
- Misc. fixes to demos, layers and Intel sample driver

## How to Build and Run

[BUILD.md](BUILD.md)
includes directions for building all the components, running the validation tests and running the demo applications.

Information on how to enable the various Debug and Validation layers is in
[layers/README.md](layers/README.md).

## References
This version of the components are written based on the following preliminary specs and proposals:
- [**XGL Programers Reference**, 1 Jul 2014](https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/proposals/AMD/Explicit%20GL%20Programming%20Guide%20and%20API%20Reference.pdf)
- [**BIL**, revision 27](https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/proposals/BIL/Specification/BIL.html)

## License
This work is intended to be released as open source under a BSD-style
license once the XGL specification is public. Until that time, this work
is covered by the Khronos NDA governing the details of the XGL API.

## Acknowledgements
While this project is being developed by LunarG, Inc; there are many other
companies and individuals making this possible: Valve Software, funding
project development; Intel Corporation, providing full hardware specifications
and valuable technical feedback; AMD, providing XGL spec editor contributions;
ARM, contributing a Chairman for this working group within Khronos; Nvidia,
providing an initial co-editor for the spec; Qualcomm for picking up the
co-editor's chair; and Khronos, for providing hosting within GitHub.

## Contact
If you have questions or comments about this driver; or you would like to contribute
directly to this effort, please contact us at XGL@LunarG.com; or if you prefer, via
the GL Common mailing list: gl_common@khronos.org
