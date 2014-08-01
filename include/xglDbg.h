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

typedef XGL_VOID (XGLAPI *XGL_DBG_MSG_CALLBACK_FUNCTION)(
    XGL_DBG_MSG_TYPE     msgType,
    XGL_VALIDATION_LEVEL validationLevel,
    XGL_BASE_OBJECT      srcObject,
    XGL_SIZE             location,
    XGL_INT              msgCode,
    const XGL_CHAR*      pMsg,
    XGL_VOID*            pUserData);

// Debug functions

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
