/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
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
 */

#pragma once

#include "vulkan.h"
#include "vk_ext_khr_device_swapchain.h"


// Hooked function prototypes

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSurfacePropertiesKHR(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, VkSurfacePropertiesKHR* pSurfaceProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSurfaceFormatsKHR(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, uint32_t* pCount, VkSurfaceFormatKHR* pSurfaceFormats);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSurfacePresentModesKHR(VkDevice device, const VkSurfaceDescriptionKHR* pSurfaceDescription, uint32_t* pCount, VkPresentModeKHR* pPresentModes);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, VkSwapchainKHR* pSwapchain);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pCount, VkImage* pSwapchainImages);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, uint32_t* pImageIndex);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueuePresentKHR(VkQueue queue, VkPresentInfoKHR* pPresentInfo);
