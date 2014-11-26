# Explicit GL (XGL) Ecosystem Components
*Version 0.3, 26 Nov 2014*

This project provides *open source* components for the XGL Ecosystem.

## Introduction

The components here are being shared with the Khronos community to provide
early insights into the specification of XGL and to assists those doing
prototyping at this point.

The following components are available:
- Proposed Reference [*ICD Loader*](https://github.com/KhronosGroup/GL-Next/tree/master/loader) (including [*Layer Management*](https://github.com/KhronosGroup/GL-Next/tree/master/layers/README.md))
- Proposed Reference [*Validation Layers*](https://github.com/KhronosGroup/GL-Next/tree/master/layers/)
  - [Object Tracker](https://github.com/KhronosGroup/GL-Next/blob/master/layers/object_track.c)
  - [Draw State](https://github.com/KhronosGroup/GL-Next/blob/master/layers/draw_state.c)
  - [MemTracker](https://github.com/KhronosGroup/GL-Next/blob/master/layers/mem_tracker.c)
- *GLAVE Debugger* 
  - API Dump (generated)
  - APIDumpFile (generated)
- [*Sample Drivers*](https://github.com/KhronosGroup/GL-Next/tree/master/icd)
  - [Common Infrastructure](https://github.com/KhronosGroup/GL-Next/tree/master/icd/common)
  - [Implementation for Intel GPUs](https://github.com/KhronosGroup/GL-Next/tree/master/icd/intel)

## New

- Added APIDumpFile to write APIDump directly to a file
- Added MemTracker validation layer
- Added various turn-on tests for variety of driver / compiler features
- Fix to wsiPresent to avoid deadlock with X server
- Misc. fixes to layers and Intel sample driver

## How to Build and Run

[BUILD.md](https://github.com/KhronosGroup/GL-Next/tree/master/BUILD.md)
includes directions for building all the components, running the validation tests and running the demo applications.

Information on how to enable the various Debug and Validation layers is in
[layers/README.md](https://github.com/KhronosGroup/GL-Next/tree/master/layers/README.md).

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
