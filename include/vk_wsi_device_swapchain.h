//
// File: vk_wsi_device_swapchain.h
//
/*
** Copyright (c) 2014 The Khronos Group Inc.
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

#ifndef __VK_WSI_DEVICE_SWAPCHAIN_H__
#define __VK_WSI_DEVICE_SWAPCHAIN_H__

#include "vulkan.h"

#define VK_WSI_DEVICE_SWAPCHAIN_REVISION            45
#define VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NUMBER    2
#define VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NAME      "VK_WSI_device_swapchain"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// ------------------------------------------------------------------------------------------------
// Objects

VK_DEFINE_NONDISP_HANDLE(VkSwapChainWSI);

// ------------------------------------------------------------------------------------------------
// Enumeration constants

#define VK_WSI_DEVICE_SWAPCHAIN_ENUM(type,id)    ((type)((int)0xc0000000 - VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NUMBER * -1024 + (id)))
#define VK_WSI_DEVICE_SWAPCHAIN_ENUM_POSITIVE(type,id)    ((type)((int)0x40000000 + (VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NUMBER - 1) * 1024 + (id)))

// Extend VkStructureType enum with extension specific constants
#define VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI VK_WSI_DEVICE_SWAPCHAIN_ENUM(VkStructureType, 0)
#define VK_STRUCTURE_TYPE_PRESENT_INFO_WSI VK_WSI_DEVICE_SWAPCHAIN_ENUM(VkStructureType, 1)

// Extend VkImageLayout enum with extension specific constants
#define VK_IMAGE_LAYOUT_PRESENT_SOURCE_WSI VK_WSI_DEVICE_SWAPCHAIN_ENUM(VkImageLayout, 2)

// Extend VkResult enum with extension specific constants
//  Return codes for successful operation execution
#define VK_SUBOPTIMAL_WSI           VK_WSI_DEVICE_SWAPCHAIN_ENUM_POSITIVE(VkResult, 3)
//  Error codes
#define VK_ERROR_OUT_OF_DATE_WSI    VK_WSI_DEVICE_SWAPCHAIN_ENUM(VkResult, 4)

// ------------------------------------------------------------------------------------------------
// Enumerations

typedef enum VkSurfaceTransformWSI_
{
    VK_SURFACE_TRANSFORM_NONE_WSI = 0,
    VK_SURFACE_TRANSFORM_ROT90_WSI = 1,
    VK_SURFACE_TRANSFORM_ROT180_WSI = 2,
    VK_SURFACE_TRANSFORM_ROT270_WSI = 3,
    VK_SURFACE_TRANSFORM_HMIRROR_WSI = 4,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT90_WSI = 5,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT180_WSI = 6,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT270_WSI = 7,
    VK_SURFACE_TRANSFORM_INHERIT_WSI = 8,
} VkSurfaceTransformWSI;

typedef enum VkSurfaceTransformFlagBitsWSI_
{
    VK_SURFACE_TRANSFORM_NONE_BIT_WSI = 0x00000001,
    VK_SURFACE_TRANSFORM_ROT90_BIT_WSI = 0x00000002,
    VK_SURFACE_TRANSFORM_ROT180_BIT_WSI = 0x00000004,
    VK_SURFACE_TRANSFORM_ROT270_BIT_WSI = 0x00000008,
    VK_SURFACE_TRANSFORM_HMIRROR_BIT_WSI = 0x00000010,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT90_BIT_WSI = 0x00000020,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT180_BIT_WSI = 0x00000040,
    VK_SURFACE_TRANSFORM_HMIRROR_ROT270_BIT_WSI = 0x00000080,
    VK_SURFACE_TRANSFORM_INHERIT_BIT_WSI = 0x00000100,
} VkSurfaceTransformFlagBitsWSI;
typedef VkFlags VkSurfaceTransformFlagsWSI;

typedef enum VkPresentModeWSI_
{
    VK_PRESENT_MODE_IMMEDIATE_WSI = 0,
    VK_PRESENT_MODE_MAILBOX_WSI = 1,
    VK_PRESENT_MODE_FIFO_WSI = 2,
    VK_PRESENT_MODE_BEGIN_RANGE_WSI = VK_PRESENT_MODE_IMMEDIATE_WSI,
    VK_PRESENT_MODE_END_RANGE_WSI = VK_PRESENT_MODE_FIFO_WSI,
    VK_PRESENT_MODE_NUM = (VK_PRESENT_MODE_FIFO_WSI - VK_PRESENT_MODE_IMMEDIATE_WSI + 1),
    VK_PRESENT_MODE_MAX_ENUM_WSI = 0x7FFFFFFF
} VkPresentModeWSI;

typedef enum VkColorSpaceWSI_
{
    VK_COLORSPACE_SRGB_NONLINEAR_WSI = 0x00000000,
    VK_COLORSPACE_MAX_ENUM_WSI = 0x7FFFFFFF
} VkColorSpaceWSI;

// ------------------------------------------------------------------------------------------------
// Flags

// ------------------------------------------------------------------------------------------------
// Structures

typedef struct VkSurfacePropertiesWSI_
{
    uint32_t                                minImageCount;      // Supported minimum number of images for the surface
    uint32_t                                maxImageCount;      // Supported maximum number of images for the surface, 0 for unlimited

    VkExtent2D                              currentExtent;      // Current image width and height for the surface, (-1, -1) if undefined.
    VkExtent2D                              minImageExtent;     // Supported minimum image width and height for the surface
    VkExtent2D                              maxImageExtent;     // Supported maximum image width and height for the surface

    VkSurfaceTransformFlagsWSI              supportedTransforms;// 1 or more bits representing the transforms supported
    VkSurfaceTransformWSI                   currentTransform;   // The surface's current transform relative to the device's natural orientation.

    uint32_t                                maxImageArraySize;  // Supported maximum number of image layers for the surface

    VkImageUsageFlags                       supportedUsageFlags;// Supported image usage flags for the surface
} VkSurfacePropertiesWSI;

typedef struct VkSurfaceFormatWSI_
{
    VkFormat                                format;             // Supported pair of rendering format 
    VkColorSpaceWSI                         colorSpace;         // and colorspace for the surface
} VkSurfaceFormatWSI;

typedef struct VkSwapChainCreateInfoWSI_
{
    VkStructureType                          sType;             // Must be VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI
    const void*                              pNext;             // Pointer to next structure

    const VkSurfaceDescriptionWSI*           pSurfaceDescription;// describes the swap chain's target surface

    uint32_t                                 minImageCount;     // Minimum number of presentation images the application needs
    VkFormat                                 imageFormat;       // Format of the presentation images
    VkColorSpaceWSI                          imageColorSpace;   // Colorspace of the presentation images
    VkExtent2D                               imageExtent;       // Dimensions of the presentation images
    VkImageUsageFlags                        imageUsageFlags;   // Bits indicating how the presentation images will be used
    VkSurfaceTransformWSI                    preTransform;      // The transform, relative to the device's natural orientation, applied to the image content prior to presentation
    uint32_t                                 imageArraySize;    // Determines the number of views for multiview/stereo presentation

    VkSharingMode                            sharingMode;       // Sharing mode used for the presentation images
    uint32_t                                 queueFamilyCount;  // Number of queue families having access to the images in case of concurrent sharing mode
    const uint32_t*                          pQueueFamilyIndices; // Array of queue family indices having access to the images in case of concurrent sharing mode

    VkPresentModeWSI                         presentMode;       // Which presentation mode to use for presents on this swap chain.

    VkSwapChainWSI                           oldSwapChain;      // Existing swap chain to replace, if any.

    VkBool32                                 clipped;           // Specifies whether presentable images may be affected by window clip regions.
} VkSwapChainCreateInfoWSI;

typedef struct VkPresentInfoWSI_
{
    VkStructureType                          sType;             // Must be VK_STRUCTURE_TYPE_PRESENT_INFO_WSI
    const void*                              pNext;             // Pointer to next structure
    uint32_t                                 swapChainCount;    // Number of swap chains to present in this call
    const VkSwapChainWSI*                    swapChains;        // Swap chains to present an image from.
    const uint32_t*                          imageIndices;      // Indices of which swapChain images to present
} VkPresentInfoWSI;

// ------------------------------------------------------------------------------------------------
// Function types

typedef VkResult (VKAPI *PFN_vkGetSurfacePropertiesWSI)(VkDevice device, const VkSurfaceDescriptionWSI* pSurfaceDescription, VkSurfacePropertiesWSI* pSurfaceProperties);
typedef VkResult (VKAPI *PFN_vkGetSurfaceFormatsWSI)(VkDevice device, const VkSurfaceDescriptionWSI* pSurfaceDescription, uint32_t* pCount, VkSurfaceFormatWSI* pSurfaceFormats);
typedef VkResult (VKAPI *PFN_vkGetSurfacePresentModesWSI)(VkDevice device, const VkSurfaceDescriptionWSI* pSurfaceDescription, uint32_t* pCount, VkPresentModeWSI* pPresentModes);
typedef VkResult (VKAPI *PFN_vkCreateSwapChainWSI)(VkDevice device, const VkSwapChainCreateInfoWSI* pCreateInfo, VkSwapChainWSI* pSwapChain);
typedef VkResult (VKAPI *PFN_vkDestroySwapChainWSI)(VkDevice device, VkSwapChainWSI swapChain);
typedef VkResult (VKAPI *PFN_vkGetSwapChainImagesWSI)(VkDevice device, VkSwapChainWSI swapChain, uint32_t* pCount, VkImage* pSwapChainImages);
typedef VkResult (VKAPI *PFN_vkAcquireNextImageWSI)(VkDevice device, VkSwapChainWSI swapChain, uint64_t timeout, VkSemaphore semaphore, uint32_t* pImageIndex);
typedef VkResult (VKAPI *PFN_vkQueuePresentWSI)(VkQueue queue, VkPresentInfoWSI* pPresentInfo);

// ------------------------------------------------------------------------------------------------
// Function prototypes

#ifdef VK_PROTOTYPES

VkResult VKAPI vkGetSurfacePropertiesWSI(
    VkDevice                                 device,
    const VkSurfaceDescriptionWSI*           pSurfaceDescription,
    VkSurfacePropertiesWSI*                  pSurfaceProperties);

VkResult VKAPI vkGetSurfaceFormatsWSI(
    VkDevice                                 device,
    const VkSurfaceDescriptionWSI*           pSurfaceDescription,
    uint32_t*                                pCount,
    VkSurfaceFormatWSI*                      pSurfaceFormats);

VkResult VKAPI vkGetSurfacePresentModesWSI(
    VkDevice                                 device,
    const VkSurfaceDescriptionWSI*           pSurfaceDescription,
    uint32_t*                                pCount,
    VkPresentModeWSI*                        pPresentModes);

VkResult VKAPI vkCreateSwapChainWSI(
    VkDevice                                 device,
    const VkSwapChainCreateInfoWSI*          pCreateInfo,
    VkSwapChainWSI*                          pSwapChain);

VkResult VKAPI vkDestroySwapChainWSI(
    VkDevice                                 device,
    VkSwapChainWSI                           swapChain);

VkResult VKAPI vkGetSwapChainImagesWSI(
    VkDevice                                 device,
    VkSwapChainWSI                           swapChain,
    uint32_t*                                pCount,
    VkImage*                                 pSwapChainImages);

VkResult VKAPI vkAcquireNextImageWSI(
    VkDevice                                 device,
    VkSwapChainWSI                           swapChain,
    uint64_t                                 timeout,
    VkSemaphore                              semaphore,
    uint32_t*                                pImageIndex);

VkResult VKAPI vkQueuePresentWSI(
    VkQueue                                  queue,
    VkPresentInfoWSI*                        pPresentInfo);

#endif // VK_PROTOTYPES

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __VK_WSI_SWAPCHAIN_H__
