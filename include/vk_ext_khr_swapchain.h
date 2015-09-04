//
// File: vk_ext_khr_swapchain.h
//
/*
** Copyright (c) 2015 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#ifndef __VK_EXT_KHR_SWAPCHAIN_H__
#define __VK_EXT_KHR_SWAPCHAIN_H__

#include "vulkan.h"

#define VK_EXT_KHR_SWAPCHAIN_REVISION         17
#define VK_EXT_KHR_SWAPCHAIN_EXTENSION_NUMBER 1
#define VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME   "VK_EXT_KHR_swapchain"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// ------------------------------------------------------------------------------------------------
// Objects

// ------------------------------------------------------------------------------------------------
// Enumeration constants

#define VK_EXT_KHR_SWAPCHAIN_ENUM(type,id)    ((type)((int)0xc0000000 - VK_EXT_KHR_SWAPCHAIN_EXTENSION_NUMBER * -1024 + (id)))
#define VK_EXT_KHR_SWAPCHAIN_ENUM_POSITIVE(type,id)    ((type)((int)0x40000000 + (VK_EXT_KHR_SWAPCHAIN_EXTENSION_NUMBER - 1) * 1024 + (id)))

// Extend VkStructureType enum with extension specific constants
#define VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_KHR            VK_EXT_KHR_SWAPCHAIN_ENUM(VkStructureType, 0)

// ------------------------------------------------------------------------------------------------
// Enumerations

typedef enum {
    VK_SURFACE_TRANSFORM_NONE_KHR = 0,
    VK_SURFACE_TRANSFORM_ROT90_KHR = 1,
    VK_SURFACE_TRANSFORM_ROT180_KHR = 2,
    VK_SURFACE_TRANSFORM_ROT270_KHR = 3,
    VK_SURFACE_TRANSFORM_HMIRROR_KHR = 4,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT90_KHR = 5,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT180_KHR = 6,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT270_KHR = 7,
    VK_SURFACE_TRANSFORM_INHERIT_KHR = 8,
} VkSurfaceTransformKHR;

typedef enum {
    VK_SURFACE_TRANSFORM_NONE_BIT_KHR = 0x00000001,
    VK_SURFACE_TRANSFORM_ROT90_BIT_KHR = 0x00000002,
    VK_SURFACE_TRANSFORM_ROT180_BIT_KHR = 0x00000004,
    VK_SURFACE_TRANSFORM_ROT270_BIT_KHR = 0x00000008,
    VK_SURFACE_TRANSFORM_HMIRROR_BIT_KHR = 0x00000010,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT90_BIT_KHR = 0x00000020,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT180_BIT_KHR = 0x00000040,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT270_BIT_KHR = 0x00000080,
    VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR = 0x00000100,
} VkSurfaceTransformFlagBitsKHR;
typedef VkFlags VkSurfaceTransformFlagsKHR;

typedef enum {
    VK_PLATFORM_WIN32_KHR = 0,
    VK_PLATFORM_X11_KHR = 1,
    VK_PLATFORM_XCB_KHR = 2,
    VK_PLATFORM_ANDROID_KHR = 3,
    VK_PLATFORM_WAYLAND_KHR = 4,
    VK_PLATFORM_MIR_KHR = 5,
    VK_PLATFORM_BEGIN_RANGE_KHR = VK_PLATFORM_WIN32_KHR,
    VK_PLATFORM_END_RANGE_KHR = VK_PLATFORM_MIR_KHR,
    VK_PLATFORM_NUM_KHR = (VK_PLATFORM_MIR_KHR - VK_PLATFORM_WIN32_KHR + 1),
    VK_PLATFORM_MAX_ENUM_KHR = 0x7FFFFFFF
} VkPlatformKHR;

// ------------------------------------------------------------------------------------------------
// Flags

// ------------------------------------------------------------------------------------------------
// Structures

// Placeholder structure header for the different types of surface description structures
typedef struct {
    VkStructureType                          sType;             // Can be any of the VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_XXX_KHR constants
    const void*                              pNext;             // Pointer to next structure
} VkSurfaceDescriptionKHR;

// Surface description structure for a native platform window surface
typedef struct {
    VkStructureType                         sType;              // Must be VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_KHR
    const void*                             pNext;              // Pointer to next structure
    VkPlatformKHR                           platform;           // e.g. VK_PLATFORM_*_KHR
    void*                                   pPlatformHandle;
    void*                                   pPlatformWindow;
} VkSurfaceDescriptionWindowKHR;

// pPlatformHandle points to this struct when platform is VK_PLATFORM_X11_KHR
#ifdef _X11_XLIB_H_
typedef struct {
    Display*                                 dpy;               // Display connection to an X server
    Window                                   root;              // To identify the X screen
} VkPlatformHandleX11KHR;
#endif /* _X11_XLIB_H_ */

// pPlatformHandle points to this struct when platform is VK_PLATFORM_XCB_KHR
#ifdef __XCB_H__
typedef struct {
    xcb_connection_t*                        connection;        // XCB connection to an X server
    xcb_window_t                             root;              // To identify the X screen
} VkPlatformHandleXcbKHR;
#endif /* __XCB_H__ */

// ------------------------------------------------------------------------------------------------
// Function types

typedef VkResult (VKAPI *PFN_vkGetPhysicalDeviceSurfaceSupportKHR)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, const VkSurfaceDescriptionKHR* pSurfaceDescription, VkBool32* pSupported);

// ------------------------------------------------------------------------------------------------
// Function prototypes

#ifdef VK_PROTOTYPES

VkResult VKAPI vkGetPhysicalDeviceSurfaceSupportKHR(
    VkPhysicalDevice                        physicalDevice,
    uint32_t                                queueFamilyIndex,
    const VkSurfaceDescriptionKHR*          pSurfaceDescription,
    VkBool32*                               pSupported);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VK_EXT_KHR_SWAPCHAIN_H__
