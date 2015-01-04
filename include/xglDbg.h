#ifndef __XGLDBG_H__
#define __XGLDBG_H__

#include <xgl.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef enum _XGL_DBG_MSG_TYPE
{
    XGL_DBG_MSG_UNKNOWN      = 0x0,
    XGL_DBG_MSG_ERROR        = 0x1,
    XGL_DBG_MSG_WARNING      = 0x2,
    XGL_DBG_MSG_PERF_WARNING = 0x3,

    XGL_DBG_MSG_TYPE_BEGIN_RANGE = XGL_DBG_MSG_UNKNOWN,
    XGL_DBG_MSG_TYPE_END_RANGE   = XGL_DBG_MSG_PERF_WARNING,
    XGL_NUM_DBG_MSG_TYPE         = (XGL_DBG_MSG_TYPE_END_RANGE - XGL_DBG_MSG_TYPE_BEGIN_RANGE + 1),
} XGL_DBG_MSG_TYPE;

typedef enum _XGL_DBG_MSG_FILTER
{
    XGL_DBG_MSG_FILTER_NONE     = 0x0,
    XGL_DBG_MSG_FILTER_REPEATED = 0x1,
    XGL_DBG_MSG_FILTER_ALL      = 0x2,

    XGL_DBG_MSG_FILTER_BEGIN_RANGE = XGL_DBG_MSG_FILTER_NONE,
    XGL_DBG_MSG_FILTER_END_RANGE   = XGL_DBG_MSG_FILTER_ALL,
    XGL_NUM_DBG_MSG_FILTER         = (XGL_DBG_MSG_FILTER_END_RANGE - XGL_DBG_MSG_FILTER_BEGIN_RANGE + 1),
} XGL_DBG_MSG_FILTER;

typedef enum _XGL_DBG_GLOBAL_OPTION
{
    XGL_DBG_OPTION_DEBUG_ECHO_ENABLE = 0x0,
    XGL_DBG_OPTION_BREAK_ON_ERROR    = 0x1,
    XGL_DBG_OPTION_BREAK_ON_WARNING  = 0x2,

    XGL_DBG_GLOBAL_OPTION_BEGIN_RANGE = XGL_DBG_OPTION_DEBUG_ECHO_ENABLE,
    XGL_DBG_GLOBAL_OPTION_END_RANGE   = XGL_DBG_OPTION_BREAK_ON_WARNING,
    XGL_NUM_DBG_GLOBAL_OPTION         = (XGL_DBG_GLOBAL_OPTION_END_RANGE - XGL_DBG_GLOBAL_OPTION_BEGIN_RANGE + 1),
} XGL_DBG_GLOBAL_OPTION;

typedef enum _XGL_DBG_DEVICE_OPTION
{
    XGL_DBG_OPTION_DISABLE_PIPELINE_LOADS      = 0x0,
    XGL_DBG_OPTION_FORCE_OBJECT_MEMORY_REQS    = 0x1,
    XGL_DBG_OPTION_FORCE_LARGE_IMAGE_ALIGNMENT = 0x2,

    XGL_DBG_DEVICE_OPTION_BEGIN_RANGE = XGL_DBG_OPTION_DISABLE_PIPELINE_LOADS,
    XGL_DBG_DEVICE_OPTION_END_RANGE   = XGL_DBG_OPTION_FORCE_LARGE_IMAGE_ALIGNMENT,
    XGL_NUM_DBG_DEVICE_OPTION         = (XGL_DBG_DEVICE_OPTION_END_RANGE - XGL_DBG_DEVICE_OPTION_BEGIN_RANGE + 1),
} XGL_DBG_DEVICE_OPTION;

typedef enum _XGL_DBG_OBJECT_TYPE
{
    XGL_DBG_OBJECT_UNKNOWN                = 0x00,
    XGL_DBG_OBJECT_DEVICE                 = 0x01,
    XGL_DBG_OBJECT_QUEUE                  = 0x02,
    XGL_DBG_OBJECT_GPU_MEMORY             = 0x03,
    XGL_DBG_OBJECT_IMAGE                  = 0x04,
    XGL_DBG_OBJECT_IMAGE_VIEW             = 0x05,
    XGL_DBG_OBJECT_COLOR_TARGET_VIEW      = 0x06,
    XGL_DBG_OBJECT_DEPTH_STENCIL_VIEW     = 0x07,
    XGL_DBG_OBJECT_SHADER                 = 0x08,
    XGL_DBG_OBJECT_GRAPHICS_PIPELINE      = 0x09,
    XGL_DBG_OBJECT_COMPUTE_PIPELINE       = 0x0a,
    XGL_DBG_OBJECT_SAMPLER                = 0x0b,
    XGL_DBG_OBJECT_DESCRIPTOR_SET         = 0x0c,
    XGL_DBG_OBJECT_VIEWPORT_STATE         = 0x0d,
    XGL_DBG_OBJECT_RASTER_STATE           = 0x0e,
    XGL_DBG_OBJECT_MSAA_STATE             = 0x0f,
    XGL_DBG_OBJECT_COLOR_BLEND_STATE      = 0x10,
    XGL_DBG_OBJECT_DEPTH_STENCIL_STATE    = 0x11,
    XGL_DBG_OBJECT_CMD_BUFFER             = 0x12,
    XGL_DBG_OBJECT_FENCE                  = 0x13,
    XGL_DBG_OBJECT_QUEUE_SEMAPHORE        = 0x14,
    XGL_DBG_OBJECT_EVENT                  = 0x15,
    XGL_DBG_OBJECT_QUERY_POOL             = 0x16,
    XGL_DBG_OBJECT_SHARED_GPU_MEMORY      = 0x17,
    XGL_DBG_OBJECT_SHARED_QUEUE_SEMAPHORE = 0x18,
    XGL_DBG_OBJECT_PEER_GPU_MEMORY        = 0x19,
    XGL_DBG_OBJECT_PEER_IMAGE             = 0x1a,
    XGL_DBG_OBJECT_PINNED_GPU_MEMORY      = 0x1b,
    XGL_DBG_OBJECT_INTERNAL_GPU_MEMORY    = 0x1c,
    XGL_DBG_OBJECT_FRAMEBUFFER            = 0x1d,
    XGL_DBG_OBJECT_RENDER_PASS            = 0x1e,

    XGL_DBG_OBJECT_BUFFER,
    XGL_DBG_OBJECT_BUFFER_VIEW,
    XGL_DBG_OBJECT_DESCRIPTOR_SET_LAYOUT,
    XGL_DBG_OBJECT_DESCRIPTOR_REGION,

    XGL_DBG_OBJECT_TYPE_BEGIN_RANGE = XGL_DBG_OBJECT_UNKNOWN,
    XGL_DBG_OBJECT_TYPE_END_RANGE   = XGL_DBG_OBJECT_RENDER_PASS,
    XGL_NUM_DBG_OBJECT_TYPE         = (XGL_DBG_OBJECT_TYPE_END_RANGE - XGL_DBG_OBJECT_TYPE_BEGIN_RANGE + 1),
} XGL_DBG_OBJECT_TYPE;

typedef XGL_VOID (XGLAPI *XGL_DBG_MSG_CALLBACK_FUNCTION)(
    XGL_DBG_MSG_TYPE     msgType,
    XGL_VALIDATION_LEVEL validationLevel,
    XGL_BASE_OBJECT      srcObject,
    XGL_SIZE             location,
    XGL_INT              msgCode,
    const XGL_CHAR*      pMsg,
    XGL_VOID*            pUserData);

// Debug functions
typedef XGL_RESULT (XGLAPI *xglDbgSetValidationLevelType)(XGL_DEVICE device, XGL_VALIDATION_LEVEL validationLevel);
typedef XGL_RESULT (XGLAPI *xglDbgRegisterMsgCallbackType)(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData);
typedef XGL_RESULT (XGLAPI *xglDbgUnregisterMsgCallbackType)(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
typedef XGL_RESULT (XGLAPI *xglDbgSetMessageFilterType)(XGL_DEVICE device, XGL_INT msgCode, XGL_DBG_MSG_FILTER filter);
typedef XGL_RESULT (XGLAPI *xglDbgSetObjectTagType)(XGL_BASE_OBJECT object, XGL_SIZE tagSize, const XGL_VOID* pTag);
typedef XGL_RESULT (XGLAPI *xglDbgSetGlobalOptionType)(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData);
typedef XGL_RESULT (XGLAPI *xglDbgSetDeviceOptionType)(XGL_DEVICE device, XGL_DBG_DEVICE_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData);
typedef XGL_VOID (XGLAPI *xglCmdDbgMarkerBeginType)(XGL_CMD_BUFFER cmdBuffer, const XGL_CHAR* pMarker);
typedef XGL_VOID (XGLAPI *xglCmdDbgMarkerEndType)(XGL_CMD_BUFFER cmdBuffer);

XGL_RESULT XGLAPI xglDbgSetValidationLevel(
    XGL_DEVICE           device,
    XGL_VALIDATION_LEVEL validationLevel);

XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(
    XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback,
    XGL_VOID*                     pUserData);

XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(
    XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);

XGL_RESULT XGLAPI xglDbgSetMessageFilter(
    XGL_DEVICE         device,
    XGL_INT            msgCode,
    XGL_DBG_MSG_FILTER filter);

XGL_RESULT XGLAPI xglDbgSetObjectTag(
    XGL_BASE_OBJECT object,
    XGL_SIZE        tagSize,
    const XGL_VOID* pTag);

XGL_RESULT XGLAPI xglDbgSetGlobalOption(
    XGL_DBG_GLOBAL_OPTION dbgOption,
    XGL_SIZE              dataSize,
    const XGL_VOID*       pData);

XGL_RESULT XGLAPI xglDbgSetDeviceOption(
    XGL_DEVICE            device,
    XGL_DBG_DEVICE_OPTION dbgOption,
    XGL_SIZE              dataSize,
    const XGL_VOID*       pData);

XGL_VOID XGLAPI xglCmdDbgMarkerBegin(
    XGL_CMD_BUFFER  cmdBuffer,
    const XGL_CHAR* pMarker);

XGL_VOID XGLAPI xglCmdDbgMarkerEnd(
    XGL_CMD_BUFFER  cmdBuffer);

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // __XGLDBG_H__
