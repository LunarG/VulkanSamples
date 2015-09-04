//
// File: vk_ext_khr_device_swapchain.h
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

#ifndef __VK_EXT_KHR_DEVICE_SWAPCHAIN_H__
#define __VK_EXT_KHR_DEVICE_SWAPCHAIN_H__

#include "vulkan.h"

#define VK_EXT_KHR_DEVICE_SWAPCHAIN_REVISION         51
#define VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NUMBER 2
#define VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME   "VK_EXT_KHR_device_swapchain"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// ------------------------------------------------------------------------------------------------
// Objects

VK_DEFINE_NONDISP_HANDLE(VkSwapchainKHR);

// ------------------------------------------------------------------------------------------------
// Enumeration constants

#define VK_EXT_KHR_DEVICE_SWAPCHAIN_ENUM(type,id)    ((type)((int)0xc0000000 - VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NUMBER * -1024 + (id)))
#define VK_EXT_KHR_DEVICE_SWAPCHAIN_ENUM_POSITIVE(type,id)    ((type)((int)0x40000000 + (VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NUMBER - 1) * 1024 + (id)))

// Extend VkStructureType enum with extension specific constants
#define VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_KHR VK_EXT_KHR_DEVICE_SWAPCHAIN_ENUM(VkStructureType, 0)
#define VK_STRUCTURE_TYPE_PRESENT_INFO_KHR VK_EXT_KHR_DEVICE_SWAPCHAIN_ENUM(VkStructureType, 1)

// Extend VkImageLayout enum with extension specific constants
#define VK_IMAGE_LAYOUT_PRESENT_SOURCE_KHR VK_EXT_KHR_DEVICE_SWAPCHAIN_ENUM(VkImageLayout, 2)

// Extend VkResult enum with extension specific constants
//  Return codes for successful operation execution
#define VK_SUBOPTIMAL_KHR           VK_EXT_KHR_DEVICE_SWAPCHAIN_ENUM_POSITIVE(VkResult, 3)
//  Error codes
#define VK_ERROR_OUT_OF_DATE_KHR    VK_EXT_KHR_DEVICE_SWAPCHAIN_ENUM(VkResult, 4)

// ------------------------------------------------------------------------------------------------
// Enumerations

typedef enum {
    VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
    VK_PRESENT_MODE_MAILBOX_KHR = 1,
    VK_PRESENT_MODE_FIFO_KHR = 2,
    VK_PRESENT_MODE_BEGIN_RANGE_KHR = VK_PRESENT_MODE_IMMEDIATE_KHR,
    VK_PRESENT_MODE_END_RANGE_KHR = VK_PRESENT_MODE_FIFO_KHR,
    VK_PRESENT_MODE_NUM = (VK_PRESENT_MODE_FIFO_KHR - VK_PRESENT_MODE_IMMEDIATE_KHR + 1),
    VK_PRESENT_MODE_MAX_ENUM_KHR = 0x7FFFFFFF
} VkPresentModeKHR;

typedef enum {
    VK_COLORSPACE_SRGB_NONLINEAR_KHR = 0x00000000,
    VK_COLORSPACE_NUM = (VK_COLORSPACE_SRGB_NONLINEAR_KHR - VK_COLORSPACE_SRGB_NONLINEAR_KHR + 1),
    VK_COLORSPACE_MAX_ENUM_KHR = 0x7FFFFFFF
} VkColorSpaceKHR;

// ------------------------------------------------------------------------------------------------
// Flags

// ------------------------------------------------------------------------------------------------
// Structures

typedef struct {
    uint32_t                                minImageCount;      // Supported minimum number of images for the surface
    uint32_t                                maxImageCount;      // Supported maximum number of images for the surface, 0 for unlimited

    VkExtent2D                              currentExtent;      // Current image width and height for the surface, (-1, -1) if undefined
    VkExtent2D                              minImageExtent;     // Supported minimum image width and height for the surface
    VkExtent2D                              maxImageExtent;     // Supported maximum image width and height for the surface

    VkSurfaceTransformFlagsKHR              supportedTransforms;// 1 or more bits representing the transforms supported
    VkSurfaceTransformKHR                   currentTransform;   // The surface's current transform relative to the device's natural orientation

    uint32_t                                maxImageArraySize;  // Supported maximum number of image layers for the surface

    VkImageUsageFlags                       supportedUsageFlags;// Supported image usage flags for the surface
} VkSurfacePropertiesKHR;

typedef struct {
    VkFormat                                format;             // Supported pair of rendering format
    VkColorSpaceKHR                         colorSpace;         // and colorspace for the surface
} VkSurfaceFormatKHR;

typedef struct {
    VkStructureType                          sType;             // Must be VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_KHR
    const void*                              pNext;             // Pointer to next structure

    const VkSurfaceDescriptionKHR*           pSurfaceDescription;// describes the swap chain's target surface

    uint32_t                                 minImageCount;     // Minimum number of presentation images the application needs
    VkFormat                                 imageFormat;       // Format of the presentation images
    VkColorSpaceKHR                          imageColorSpace;   // Colorspace of the presentation images
    VkExtent2D                               imageExtent;       // Dimensions of the presentation images
    VkImageUsageFlags                        imageUsageFlags;   // Bits indicating how the presentation images will be used
    VkSurfaceTransformKHR                    preTransform;      // The transform, relative to the device's natural orientation, applied to the image content prior to presentation
    uint32_t                                 imageArraySize;    // Determines the number of views for multiview/stereo presentation

    VkSharingMode                            sharingMode;       // Sharing mode used for the presentation images
    uint32_t                                 queueFamilyCount;  // Number of queue families having access to the images in case of concurrent sharing mode
    const uint32_t*                          pQueueFamilyIndices; // Array of queue family indices having access to the images in case of concurrent sharing mode

    VkPresentModeKHR                         presentMode;       // Which presentation mode to use for presents on this swap chain

    VkSwapchainKHR                           oldSwapchain;      // Existing swap chain to replace, if any

    VkBool32                                 clipped;           // Specifies whether presentable images may be affected by window clip regions
} VkSwapchainCreateInfoKHR;

typedef struct {
    VkStructureType                          sType;             // Must be VK_STRUCTURE_TYPE_PRESENT_INFO_KHR
    const void*                              pNext;             // Pointer to next structure
    uint32_t                                 swapchainCount;    // Number of swap chains to present in this call
    const VkSwapchainKHR*                    swapchains;        // Swap chains to present an image from
    const uint32_t*                          imageIndices;      // Indices of which swapchain images to present
} VkPresentInfoKHR;

// ------------------------------------------------------------------------------------------------
// Function types

typedef VkResult (VKAPI *PFN_vkGetSurfacePropertiesKHR)(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, VkSurfacePropertiesKHR* pSurfaceProperties);
typedef VkResult (VKAPI *PFN_vkGetSurfaceFormatsKHR)(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, uint32_t* pCount, VkSurfaceFormatKHR* pSurfaceFormats);
typedef VkResult (VKAPI *PFN_vkGetSurfacePresentModesKHR)(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, uint32_t* pCount, VkPresentModeKHR* pPresentModes);
typedef VkResult (VKAPI *PFN_vkCreateSwapchainKHR)(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, VkSwapchainKHR* pSwapchain);
typedef VkResult (VKAPI *PFN_vkDestroySwapchainKHR)(VkDevice device, VkSwapchainKHR swapchain);
typedef VkResult (VKAPI *PFN_vkGetSwapchainImagesKHR)(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pCount, VkImage* pSwapchainImages);
typedef VkResult (VKAPI *PFN_vkAcquireNextImageKHR)(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, uint32_t* pImageIndex);
typedef VkResult (VKAPI *PFN_vkQueuePresentKHR)(VkQueue queue, VkPresentInfoKHR* pPresentInfo);

// ------------------------------------------------------------------------------------------------
// Function prototypes

#ifdef VK_PROTOTYPES

VkResult VKAPI vkGetSurfacePropertiesKHR(
    VkDevice                                 device,
    const VkSurfaceDescriptionKHR*           pSurfaceDescription,
    VkSurfacePropertiesKHR*                  pSurfaceProperties);

VkResult VKAPI vkGetSurfaceFormatsKHR(
    VkDevice                                 device,
    const VkSurfaceDescriptionKHR*           pSurfaceDescription,
    uint32_t*                                pCount,
    VkSurfaceFormatKHR*                      pSurfaceFormats);

VkResult VKAPI vkGetSurfacePresentModesKHR(
    VkDevice                                 device,
    const VkSurfaceDescriptionKHR*           pSurfaceDescription,
    uint32_t*                                pCount,
    VkPresentModeKHR*                        pPresentModes);

VkResult VKAPI vkCreateSwapchainKHR(
    VkDevice                                 device,
    const VkSwapchainCreateInfoKHR*          pCreateInfo,
    VkSwapchainKHR*                          pSwapchain);

VkResult VKAPI vkDestroySwapchainKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain);

VkResult VKAPI vkGetSwapchainImagesKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain,
    uint32_t*                                pCount,
    VkImage*                                 pSwapchainImages);

VkResult VKAPI vkAcquireNextImageKHR(
    VkDevice                                 device,
    VkSwapchainKHR                           swapchain,
    uint64_t                                 timeout,
    VkSemaphore                              semaphore,
    uint32_t*                                pImageIndex);

VkResult VKAPI vkQueuePresentKHR(
    VkQueue                                  queue,
    VkPresentInfoKHR*                        pPresentInfo);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VK_EXT_KHR_SWAPCHAIN_H__
