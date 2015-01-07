# Loader Description 

## Overview
The Loader implements the main XGL library: libXGL.so on Linux.  It handles
layer management and driver management.  Loader driver management includes
finding driver librairies and loading them.  Aditionally, the loader dispatches
the API calls to the correct driver based on the GPU selected by the app. The
loader fully supports multi-gpu  operation.

Loader layer management includes finding layer libraries and activating them
as requested.  Loader correctly sets up layer and its own dispatch tables to
support multiple layers activated.  Each active layer can intercept a subset of
the full API entrypoints.  A layer which doesn't intercept a given entrypoint
will be skipped for that entrypoint.  The loader supports layers that operate
on multiple GPUs.

## Environment Variables
LIBXGL\_DRIVERS\_PATH  directory for loader to search for ICD driver libraries to open

LIBXGL\_LAYERS\_PATH   directory for loader to search for layer libraries that may get activated  and used at xglCreateDevice() time.

LIBXGL\_LAYER\_NAMES   colon separate list of layer names to be activated. Example,
   LIBXGL\_LAYER\_NAMES=MemTracker:DrawState

## Interface to driver (ICD)
- xglInitAndEnumerateGpus exported
- xglGetProcAddr exported and returns valid function pointers for all the XGL API entrypoints
- all objects created by ICD can be cast to (XGL\_LAYER\_DISPATCH\_TABLE **)
 where the loader will replace the first entry with a pointer to the dispatch table which is
 owned by the loader. This implies two things for ICD drivers:
  1. the ICD must return a pointer for the opaque object handle
  2. this pointer points to a structure with the first entry being a pointer.
- the ICD may or may not implement a dispatch table
- ICD entrypoints can be named anything including the offcial xgl name such as xglCreateDevice().  However, beware of inter
posing by dynamic OS library loaders if the offical names are used.  On Linux,
if offical names are used, the ICD library must be linked with -Bsymbolic-functions

