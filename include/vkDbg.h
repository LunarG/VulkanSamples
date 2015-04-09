#ifndef __VKDBG_H__
#define __VKDBG_H__

#include <vulkan.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef enum _VK_DBG_MSG_TYPE
{
    VK_DBG_MSG_UNKNOWN      = 0x0,
    VK_DBG_MSG_ERROR        = 0x1,
    VK_DBG_MSG_WARNING      = 0x2,
    VK_DBG_MSG_PERF_WARNING = 0x3,

    VK_DBG_MSG_TYPE_BEGIN_RANGE = VK_DBG_MSG_UNKNOWN,
    VK_DBG_MSG_TYPE_END_RANGE   = VK_DBG_MSG_PERF_WARNING,
    VK_NUM_DBG_MSG_TYPE         = (VK_DBG_MSG_TYPE_END_RANGE - VK_DBG_MSG_TYPE_BEGIN_RANGE + 1),
} VK_DBG_MSG_TYPE;

typedef enum _VK_DBG_MSG_FILTER
{
    VK_DBG_MSG_FILTER_NONE     = 0x0,
    VK_DBG_MSG_FILTER_REPEATED = 0x1,
    VK_DBG_MSG_FILTER_ALL      = 0x2,

    VK_DBG_MSG_FILTER_BEGIN_RANGE = VK_DBG_MSG_FILTER_NONE,
    VK_DBG_MSG_FILTER_END_RANGE   = VK_DBG_MSG_FILTER_ALL,
    VK_NUM_DBG_MSG_FILTER         = (VK_DBG_MSG_FILTER_END_RANGE - VK_DBG_MSG_FILTER_BEGIN_RANGE + 1),
} VK_DBG_MSG_FILTER;

typedef enum _VK_DBG_GLOBAL_OPTION
{
    VK_DBG_OPTION_DEBUG_ECHO_ENABLE = 0x0,
    VK_DBG_OPTION_BREAK_ON_ERROR    = 0x1,
    VK_DBG_OPTION_BREAK_ON_WARNING  = 0x2,

    VK_DBG_GLOBAL_OPTION_BEGIN_RANGE = VK_DBG_OPTION_DEBUG_ECHO_ENABLE,
    VK_DBG_GLOBAL_OPTION_END_RANGE   = VK_DBG_OPTION_BREAK_ON_WARNING,
    VK_NUM_DBG_GLOBAL_OPTION         = (VK_DBG_GLOBAL_OPTION_END_RANGE - VK_DBG_GLOBAL_OPTION_BEGIN_RANGE + 1),
} VK_DBG_GLOBAL_OPTION;

typedef enum _VK_DBG_DEVICE_OPTION
{
    VK_DBG_OPTION_DISABLE_PIPELINE_LOADS      = 0x0,
    VK_DBG_OPTION_FORCE_OBJECT_MEMORY_REQS    = 0x1,
    VK_DBG_OPTION_FORCE_LARGE_IMAGE_ALIGNMENT = 0x2,

    VK_DBG_DEVICE_OPTION_BEGIN_RANGE = VK_DBG_OPTION_DISABLE_PIPELINE_LOADS,
    VK_DBG_DEVICE_OPTION_END_RANGE   = VK_DBG_OPTION_FORCE_LARGE_IMAGE_ALIGNMENT,
    VK_NUM_DBG_DEVICE_OPTION         = (VK_DBG_DEVICE_OPTION_END_RANGE - VK_DBG_DEVICE_OPTION_BEGIN_RANGE + 1),
} VK_DBG_DEVICE_OPTION;

typedef enum _VK_DBG_OBJECT_TYPE
{
    VK_DBG_OBJECT_UNKNOWN                = 0x00,
    VK_DBG_OBJECT_DEVICE                 = 0x01,
    VK_DBG_OBJECT_QUEUE                  = 0x02,
    VK_DBG_OBJECT_GPU_MEMORY             = 0x03,
    VK_DBG_OBJECT_IMAGE                  = 0x04,
    VK_DBG_OBJECT_IMAGE_VIEW             = 0x05,
    VK_DBG_OBJECT_COLOR_TARGET_VIEW      = 0x06,
    VK_DBG_OBJECT_DEPTH_STENCIL_VIEW     = 0x07,
    VK_DBG_OBJECT_SHADER                 = 0x08,
    VK_DBG_OBJECT_GRAPHICS_PIPELINE      = 0x09,
    VK_DBG_OBJECT_COMPUTE_PIPELINE       = 0x0a,
    VK_DBG_OBJECT_SAMPLER                = 0x0b,
    VK_DBG_OBJECT_DESCRIPTOR_SET         = 0x0c,
    VK_DBG_OBJECT_VIEWPORT_STATE         = 0x0d,
    VK_DBG_OBJECT_RASTER_STATE           = 0x0e,
    VK_DBG_OBJECT_MSAA_STATE             = 0x0f,
    VK_DBG_OBJECT_COLOR_BLEND_STATE      = 0x10,
    VK_DBG_OBJECT_DEPTH_STENCIL_STATE    = 0x11,
    VK_DBG_OBJECT_CMD_BUFFER             = 0x12,
    VK_DBG_OBJECT_FENCE                  = 0x13,
    VK_DBG_OBJECT_SEMAPHORE              = 0x14,
    VK_DBG_OBJECT_EVENT                  = 0x15,
    VK_DBG_OBJECT_QUERY_POOL             = 0x16,
    VK_DBG_OBJECT_SHARED_GPU_MEMORY      = 0x17,
    VK_DBG_OBJECT_SHARED_SEMAPHORE       = 0x18,
    VK_DBG_OBJECT_PEER_GPU_MEMORY        = 0x19,
    VK_DBG_OBJECT_PEER_IMAGE             = 0x1a,
    VK_DBG_OBJECT_PINNED_GPU_MEMORY      = 0x1b,
    VK_DBG_OBJECT_INTERNAL_GPU_MEMORY    = 0x1c,
    VK_DBG_OBJECT_FRAMEBUFFER            = 0x1d,
    VK_DBG_OBJECT_RENDER_PASS            = 0x1e,

    VK_DBG_OBJECT_INSTANCE,
    VK_DBG_OBJECT_BUFFER,
    VK_DBG_OBJECT_BUFFER_VIEW,
    VK_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT,
    VK_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT_CHAIN,
    VK_DBG_OBJECT_DESCRIPTOR_POOL,

    VK_DBG_OBJECT_TYPE_BEGIN_RANGE = VK_DBG_OBJECT_UNKNOWN,
    VK_DBG_OBJECT_TYPE_END_RANGE   = VK_DBG_OBJECT_DESCRIPTOR_POOL,
    VK_NUM_DBG_OBJECT_TYPE         = (VK_DBG_OBJECT_TYPE_END_RANGE - VK_DBG_OBJECT_TYPE_BEGIN_RANGE + 1),
} VK_DBG_OBJECT_TYPE;

typedef void (VKAPI *VK_DBG_MSG_CALLBACK_FUNCTION)(
    VK_DBG_MSG_TYPE     msgType,
    VK_VALIDATION_LEVEL validationLevel,
    VK_BASE_OBJECT      srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData);

// Debug functions
typedef VK_RESULT (VKAPI *vkDbgSetValidationLevelType)(VK_DEVICE device, VK_VALIDATION_LEVEL validationLevel);
typedef VK_RESULT (VKAPI *vkDbgRegisterMsgCallbackType)(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData);
typedef VK_RESULT (VKAPI *vkDbgUnregisterMsgCallbackType)(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
typedef VK_RESULT (VKAPI *vkDbgSetMessageFilterType)(VK_DEVICE device, int32_t msgCode, VK_DBG_MSG_FILTER filter);
typedef VK_RESULT (VKAPI *vkDbgSetObjectTagType)(VK_BASE_OBJECT object, size_t tagSize, const void* pTag);
typedef VK_RESULT (VKAPI *vkDbgSetGlobalOptionType)(VK_INSTANCE instance, VK_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData);
typedef VK_RESULT (VKAPI *vkDbgSetDeviceOptionType)(VK_DEVICE device, VK_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData);
typedef void (VKAPI *vkCmdDbgMarkerBeginType)(VK_CMD_BUFFER cmdBuffer, const char* pMarker);
typedef void (VKAPI *vkCmdDbgMarkerEndType)(VK_CMD_BUFFER cmdBuffer);

#ifdef VK_PROTOTYPES
VK_RESULT VKAPI vkDbgSetValidationLevel(
    VK_DEVICE           device,
    VK_VALIDATION_LEVEL validationLevel);

VK_RESULT VKAPI vkDbgRegisterMsgCallback(
    VK_INSTANCE                  instance,
    VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback,
    void*                         pUserData);

VK_RESULT VKAPI vkDbgUnregisterMsgCallback(
    VK_INSTANCE                  instance,
    VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);

VK_RESULT VKAPI vkDbgSetMessageFilter(
    VK_DEVICE         device,
    int32_t            msgCode,
    VK_DBG_MSG_FILTER filter);

VK_RESULT VKAPI vkDbgSetObjectTag(
    VK_BASE_OBJECT object,
    size_t          tagSize,
    const void*     pTag);

VK_RESULT VKAPI vkDbgSetGlobalOption(
    VK_INSTANCE          instance,
    VK_DBG_GLOBAL_OPTION dbgOption,
    size_t                dataSize,
    const void*           pData);

VK_RESULT VKAPI vkDbgSetDeviceOption(
    VK_DEVICE            device,
    VK_DBG_DEVICE_OPTION dbgOption,
    size_t                dataSize,
    const void*           pData);

void VKAPI vkCmdDbgMarkerBegin(
    VK_CMD_BUFFER  cmdBuffer,
    const char*     pMarker);

void VKAPI vkCmdDbgMarkerEnd(
    VK_CMD_BUFFER  cmdBuffer);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // __VKDBG_H__
