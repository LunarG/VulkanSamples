/*
 *
 * Copyright (C) 2015 Valve Corporation
 * All Rights Reserved
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
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#pragma once

#include "vkreplay_vkreplay.h"

class vkDisplay: public vktrace_replay::DisplayImp {
friend class vkReplay;
public:
    vkDisplay();
    ~vkDisplay();
    int init(const unsigned int gpu_idx);
    int set_window(vktrace_window_handle hWindow, unsigned int width, unsigned int height);
    int create_window(const unsigned int width, const unsigned int height);
    void resize_window(const unsigned int width, const unsigned int height);
    void process_event();
    VkSurfaceDescriptionWindowKHR* get_surface_description() { return &m_SurfaceDescription; };
    // VK_DEVICE get_device() { return m_dev[m_gpuIdx];}
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    xcb_window_t get_window_handle() { return m_XcbWindow; }
    xcb_connection_t* get_connection_handle() { return m_pXcbConnection; }
    xcb_screen_t* get_screen_handle() { return m_pXcbScreen; }
#elif defined(WIN32)
    HWND get_window_handle() { return m_windowHandle; }
	HINSTANCE get_connection_handle() { return m_connection; }
#endif
private:
    VkResult init_vk(const unsigned int gpu_idx);
    bool m_initedVK;
    VkSurfaceDescriptionWindowKHR m_SurfaceDescription;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    xcb_connection_t *m_pXcbConnection;
    xcb_screen_t *m_pXcbScreen;
    xcb_window_t m_XcbWindow;
    VkPlatformHandleXcbKHR m_XcbPlatformHandle;
#elif defined(WIN32)
    HWND m_windowHandle;
	HINSTANCE m_connection;
#endif
    unsigned int m_windowWidth;
    unsigned int m_windowHeight;
    unsigned int m_frameNumber;
    std::vector<VkExtent2D> imageExtents;
    std::vector<VkImage> imageHandles;
    std::vector<VkDeviceMemory> imageMemory;
    std::vector<VkDevice> imageDevice;
#if 0
    VK_DEVICE m_dev[VK_MAX_PHYSICAL_GPUS];
    uint32_t m_gpuCount;
    unsigned int m_gpuIdx;
    VK_PHYSICAL_GPU m_gpus[VK_MAX_PHYSICAL_GPUS];
    VK_PHYSICAL_GPU_PROPERTIES m_gpuProps[VK_MAX_PHYSICAL_GPUS];
#endif
    std::vector<char *>m_extensions;
};
