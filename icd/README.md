This sample driver implementation provide multiple subcomponents required to build and test an Installable Client Driver (ICD):
- [Common Infrastructure](common)
- [Implementation for Intel GPUs](intel)
- [*Sample Driver Tests*](../tests)
    - Now includes Golden images to verify xgl_render_tests rendering.

common/ provides helper and utility functions, as well as all XGL entry points
except xglInitAndEnumerateGpus.  Hardware drivers are required to provide that
function, and to embed a "XGL_LAYER_DISPATCH_TABLE *" as the first member of
XGL_PHYSICAL_GPU and all XGL_BASE_OBJECT.

Thread safety

 We have these static variables

  - common/icd.c:static struct icd icd;
  - intel/gpu.c:static struct intel_gpu *intel_gpus;

 They require that there is no other thread calling the ICD when these
 functions are called

  - xglInitAndEnumerateGpus
  - xglDbgRegisterMsgCallback
  - xglDbgUnregisterMsgCallback
  - xglDbgSetGlobalOption
