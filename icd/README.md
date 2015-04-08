This sample driver implementation provide multiple subcomponents required to build and test an Installable Client Driver (ICD):
- [Common Infrastructure](common)
- [Implementation for Intel GPUs](intel)
- [Null driver](nulldrv)
- [*Sample Driver Tests*](../tests)
    - Now includes Golden images to verify vk_render_tests rendering.

common/ provides helper and utility functions, as well as all VK entry points
except vkInitAndEnumerateGpus.  Hardware drivers are required to provide that
function, and to embed a "VK_LAYER_DISPATCH_TABLE *" as the first member of
VK_PHYSICAL_GPU and all VK_BASE_OBJECT.

Thread safety

 We have these static variables

  - common/icd.c:static struct icd icd;
  - intel/gpu.c:static struct intel_gpu *intel_gpus;

 They require that there is no other thread calling the ICD when these
 functions are called

  - vkInitAndEnumerateGpus
  - vkDbgRegisterMsgCallback
  - vkDbgUnregisterMsgCallback
  - vkDbgSetGlobalOption
