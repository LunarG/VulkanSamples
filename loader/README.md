# Loader Description 

## Overview
The Loader implements the main VK library (e.g. "VK.dll" on Windows and
"libVK.so" on Linux).  It handles layer management and driver management.  The
loader fully supports multi-gpu operation.  As part of this, it dispatches API
calls to the correct driver, and to the correct layers, based on the GPU object
selected by the application.

The loader's driver management includes finding driver libraries and loading
them.  When a driver is initialized, the loader sets up its dispatch tables,
using a very light-weight "trampoline" mechanism.  To do so, it reserves space
for a pointer in API objects (see below for more information about this).

The loader's layer management includes finding layer libraries and activating
them as requested.  The loader correctly sets up each activated layer, and its
own dispatch tables in order to support all activated layers.  Each active
layer can intercept a subset of the full API entrypoints.  A layer which
doesn't intercept a given entrypoint will be skipped for that entrypoint.  The
loader supports layers that operate on multiple GPUs.

## Environment Variables
**LIBVK\_DRIVERS\_PATH**  directory for loader to search for ICD driver libraries to open

**LIBVK\_LAYERS\_PATH**   directory for loader to search for layer libraries that may get activated  and used at vkCreateDevice() time.

**LIBVK\_LAYER\_NAMES**   colon-separated list of layer names to be activated (e.g., LIBVK\_LAYER\_NAMES=MemTracker:DrawState).

Note: Both of the LIBVK\_*\_PATH variables may contain more than one directory.  Each directory must be separated by one of the following characters, depending on your OS:

- ";" on Windows
- ":" on Linux

## Interface to driver (ICD)
- vkEnumerateGpus exported
- vkCreateInstance exported
- vkDestroyInstance exported
- vkGetProcAddr exported and returns valid function pointers for all the VK API entrypoints
- all objects created by ICD can be cast to (VK\_LAYER\_DISPATCH\_TABLE \*\*)
 where the loader will replace the first entry with a pointer to the dispatch table which is
 owned by the loader. This implies three things for ICD drivers:
  1. The ICD must return a pointer for the opaque object handle
  2. This pointer points to a regular C structure with the first entry being a pointer.
  Note: for any C++ ICD's that implement VK objects directly as C++ classes.
  The C++ compiler may put a vtable at offset zero, if your class is virtual.
  In this case use a regular C structure (see below).
  3. The reservedForLoader.loaderMagic member must be initialized with ICD\_LOADER\_MAGIC, as follows:

```
  #include "vkIcd.h"

  struct {
        VK_LOADER_DATA reservedForLoader; // Reserve space for pointer to loader's dispatch table
        myObjectClass myObj;               // Your driver's C++ class
  } vkObj;

  vkObj alloc_icd_obj()
  {
      vkObj *newObj = alloc_obj();
      ...
      // Initialize pointer to loader's dispatch table with ICD_LOADER_MAGIC
      set_loader_magic_value(newObj);
      ...
      return newObj;
  }
```

Additional Notes:

- The ICD may or may not implement a dispatch table.
- ICD entrypoints can be named anything including the offcial vk name such as vkCreateDevice().  However, beware of interposing by dynamic OS library loaders if the offical names are used.  On Linux, if offical names are used, the ICD library must be linked with -Bsymbolic.

