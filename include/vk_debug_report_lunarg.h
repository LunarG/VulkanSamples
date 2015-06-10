//
// File: vk_debug_report_lunarg.h
//
/*
 * Vulkan
 *
 * Copyright (C) 2015 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *   Jon Ashburn <jon@lunarg.com>
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 */

#ifndef __VK_DEBUG_REPORT_LUNARG_H__
#define __VK_DEBUG_REPORT_LUNARG_H__

#include "vulkan.h"

#define VK_DEBUG_REPORT_EXTENSION_NUMBER 2
#define VK_DEBUG_REPORT_EXTENSION_VERSION 0x10
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
***************************************************************************************************
*   DebugReport Vulkan Extension API
***************************************************************************************************
*/

#define DEBUG_REPORT_EXTENSION_NAME "DEBUG_REPORT"

VK_DEFINE_NONDISP_SUBCLASS_HANDLE(VkDbgMsgCallback, VkObject)

// ------------------------------------------------------------------------------------------------
// Enumerations

typedef enum VkDbgReportFlags_
{
    VK_DBG_REPORT_INFO_BIT       = VK_BIT(0),
    VK_DBG_REPORT_WARN_BIT       = VK_BIT(1),
    VK_DBG_REPORT_PERF_WARN_BIT  = VK_BIT(2),
    VK_DBG_REPORT_ERROR_BIT      = VK_BIT(3),
} VkDbgReportFlags;

#define VK_DEBUG_REPORT_ENUM_EXTEND(type, id)    ((type)(VK_DEBUG_REPORT_EXTENSION_NUMBER * -1000 + (id)))

#define VK_OBJECT_TYPE_MSG_CALLBACK VK_DEBUG_REPORT_ENUM_EXTEND(VkObjectType, 0)
// ------------------------------------------------------------------------------------------------
// Vulkan function pointers

typedef void (*PFN_vkDbgMsgCallback)(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData);

// ------------------------------------------------------------------------------------------------
// API functions

typedef VkResult (VKAPI *PFN_vkDbgCreateMsgCallback)(VkInstance instance, VkFlags msgFlags, const PFN_vkDbgMsgCallback pfnMsgCallback, const void* pUserData, VkDbgMsgCallback* pMsgCallback);
typedef VkResult (VKAPI *PFN_vkDbgDestroyMsgCallback)(VkInstance instance, VkDbgMsgCallback msgCallback);
typedef void (VKAPI *PFN_vkDbgStringCallback)(VkFlags msgFlags, VkObjectType objType, VkObject srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData);
typedef void (VKAPI *PFN_vkDbgStdioCallback)(VkFlags msgFlags, VkObjectType objType, VkObject srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData);
typedef void (VKAPI *PFN_vkDbgBreakCallback)(VkFlags msgFlags, VkObjectType objType, VkObject srcObject, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData);

#ifdef VK_PROTOTYPES

// DebugReport extension entrypoints
VkResult VKAPI vkDbgCreateMsgCallback(
    VkInstance                          instance,
    VkFlags                             msgFlags,
    const PFN_vkDbgMsgCallback          pfnMsgCallback,
    void*                               pUserData,
    VkDbgMsgCallback*                   pMsgCallback);

VkResult VKAPI vkDbgDestroyMsgCallback(
    VkInstance                          instance,
    VkDbgMsgCallback                    msgCallback);

// DebugReport utility callback functions
void VKAPI vkDbgStringCallback(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData);

void VKAPI vkDbgStdioCallback(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData);

void VKAPI vkDbgBreakCallback(
    VkFlags                             msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VK_DEBUG_REPORT_LUNARG_H__
