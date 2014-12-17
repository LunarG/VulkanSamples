# Explicit GL (XGL) Ecosystem Components
*Version 0.7, 17 Dec 2014*

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
early insights into the specification of XGL and to assists those doing
prototyping at this point.

The following components are available:
- Proposed Reference [*ICD Loader*](loader) (including [*Layer Management*](layers/README.md))
- Proposed Reference [*Validation Layers*](layers/)
  - [Object Tracker](layers/object_track.c)
  - [Draw State](layers/draw_state.c)
  - [MemTracker](layers/mem_tracker.c)
- [*GLAVE Debugger*](tools/glave)
  - APIDump (generated)
  - APIDumpFile (generated)
  - glvtrace64: capture trace of XGL API of an application.
  - glvreplay64: replay captured trace.
  - screenshot: ![ScreenShot](http://www.lunarg.com/wp-content/themes/LunarG/images/logo.png)
- [*Sample Drivers*](icd)
  - [Common Infrastructure](icd/common)
  - [Implementation for Intel GPUs](icd/intel)
- [*Sample Driver Tests*](tests)
  - Now includes Golden images to verify xgl_render_tests rendering.

## New

- XGL API trace and capture tools. See tools/glave/README.md for details.
- Sample driver now supports multiple render targets. Added TriangleMRT to test that functionality.
- Added XGL_SLOT_SHADER_TEXTURE_RESOURCE to xgl.h as a descriptor slot type to work around confusion in GLSL
  between textures and buffers as shader resources.
- Misc. fixes for layers and Intel sample driver

## Prior updates

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
- [**BIL**, version 1.0, revision 18](https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/proposals/BIL/Specification/BIL.html)
- [**IMG's Fixed Function Proposal**, 13 Nov 2014](https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/XGL/accepted/xgl_fixed_function_vertex_fetch_proposal.txt)
- [**Valve's Loader Proposal**, 7 Oct 2014](https://cvs.khronos.org/svn/repos/oglc/trunk/nextgen/proposals/Valve/xglLayers.pptx)

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
