#ifndef __VKDBG_H__
#define __VKDBG_H__

#include <vulkan.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

typedef enum VkValidationLevel_
{
    VK_VALIDATION_LEVEL_0                                   = 0x00000000,
    VK_VALIDATION_LEVEL_1                                   = 0x00000001,
    VK_VALIDATION_LEVEL_2                                   = 0x00000002,
    VK_VALIDATION_LEVEL_3                                   = 0x00000003,
    VK_VALIDATION_LEVEL_4                                   = 0x00000004,

    VK_VALIDATION_LEVEL_BEGIN_RANGE                         = VK_VALIDATION_LEVEL_0,
    VK_VALIDATION_LEVEL_END_RANGE                           = VK_VALIDATION_LEVEL_4,
    VK_NUM_VALIDATION_LEVEL                                 = (VK_VALIDATION_LEVEL_END_RANGE - VK_VALIDATION_LEVEL_BEGIN_RANGE + 1),

    VK_MAX_ENUM(VkValidationLevel)
} VkValidationLevel;

typedef enum _VK_DBG_MSG_TYPE
{
    VK_DBG_REPORT_INFO_BIT      = 0x0,
    VK_DBG_REPORT_ERROR_BIT        = 0x1,
    VK_DBG_MSG_WARNING      = 0x2,
    VK_DBG_MSG_PERF_WARNING = 0x3,

    VK_DBG_MSG_TYPE_BEGIN_RANGE = VK_DBG_REPORT_INFO_BIT,
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

typedef void (VKAPI *VK_DBG_MSG_CALLBACK_FUNCTION)(
    VK_DBG_MSG_TYPE      msgType,
    VkValidationLevel    validationLevel,
    VkObject             srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData);

// Debug functions
typedef VkResult (VKAPI *PFN_vkDbgSetValidationLevel)(VkDevice device, VkValidationLevel validationLevel);
typedef VkResult (VKAPI *PFN_vkDbgRegisterMsgCallback)(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData);
typedef VkResult (VKAPI *PFN_vkDbgUnregisterMsgCallback)(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
typedef VkResult (VKAPI *PFN_vkDbgSetMessageFilter)(VkDevice device, int32_t msgCode, VK_DBG_MSG_FILTER filter);
typedef VkResult (VKAPI *PFN_vkDbgSetObjectTag)(VkDevice device, VkObject object, size_t tagSize, const void* pTag);
typedef VkResult (VKAPI *PFN_vkDbgSetGlobalOption)(VkInstance instance, VK_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData);
typedef VkResult (VKAPI *PFN_vkDbgSetDeviceOption)(VkDevice device, VK_DBG_DEVICE_OPTION dbgOption, size_t dataSize, const void* pData);
typedef void (VKAPI *PFN_vkCmdDbgMarkerBegin)(VkCmdBuffer cmdBuffer, const char* pMarker);
typedef void (VKAPI *PFN_vkCmdDbgMarkerEnd)(VkCmdBuffer cmdBuffer);

#ifdef VK_PROTOTYPES
VkResult VKAPI vkDbgSetValidationLevel(
    VkDevice           device,
    VkValidationLevel validationLevel);

VkResult VKAPI vkDbgRegisterMsgCallback(
    VkInstance                  instance,
    VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback,
    void*                         pUserData);

VkResult VKAPI vkDbgUnregisterMsgCallback(
    VkInstance                  instance,
    VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);

VkResult VKAPI vkDbgSetMessageFilter(
    VkDevice         device,
    int32_t            msgCode,
    VK_DBG_MSG_FILTER filter);

VkResult VKAPI vkDbgSetObjectTag(
    VkDevice        device,
    VkObject        object,
    size_t          tagSize,
    const void*     pTag);

VkResult VKAPI vkDbgSetGlobalOption(
    VkInstance          instance,
    VK_DBG_GLOBAL_OPTION dbgOption,
    size_t                dataSize,
    const void*           pData);

VkResult VKAPI vkDbgSetDeviceOption(
    VkDevice            device,
    VK_DBG_DEVICE_OPTION dbgOption,
    size_t                dataSize,
    const void*           pData);

void VKAPI vkCmdDbgMarkerBegin(
    VkCmdBuffer  cmdBuffer,
    const char*     pMarker);

void VKAPI vkCmdDbgMarkerEnd(
    VkCmdBuffer  cmdBuffer);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
}; // extern "C"
#endif // __cplusplus

#endif // __VKDBG_H__
