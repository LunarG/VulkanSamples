/**************************************************************************
 *
 * Copyright 2014 Lunarg, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#pragma once

extern "C"{
#include "vktrace_platform.h"
}

#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include <xcb/xcb.h>
typedef xcb_window_t vktrace_window_handle;
#elif defined(WIN32)
typedef HWND vktrace_window_handle;
#endif

/* classes to abstract the display and initialization of rendering API for presenting
 * framebuffers for display into a window on the screen or else fullscreen.
 * Uses Bridge design pattern.
 */
namespace vktrace_replay {

class DisplayImp {
public:
    virtual ~DisplayImp() {}
    virtual int init(const unsigned int gpu_idx) = 0;
    virtual int set_window(vktrace_window_handle hWindow, unsigned int width, unsigned int height) = 0;
    virtual int create_window(const unsigned int width, const unsigned int height) = 0;
    virtual void process_event() = 0;
};

class Display {
public:
    Display()
        : m_imp(NULL),
        m_width(0),
        m_height(0),
        m_gpu(0),
        m_fullscreen(false),
        m_hWindow(0)
    {

    }

    Display(const unsigned int width, const unsigned int height, const unsigned int gpu, const bool fullscreen) :
        m_imp(NULL),
        m_width(width),
        m_height(height),
        m_gpu(gpu),
        m_fullscreen(fullscreen),
        m_hWindow(0)
    {
    }

    Display(vktrace_window_handle hWindow, unsigned int width, unsigned int height) :
        m_imp(NULL),
        m_width(width),
        m_height(height),
        m_gpu(0),
        m_fullscreen(false),
        m_hWindow(hWindow)
    {
    }

    virtual ~Display()
    {
    }

    void set_implementation(DisplayImp & disp)
    {
        m_imp = & disp;
    }
    void set_implementation(DisplayImp * disp)
    {
        m_imp = disp;
    }
    int init()
    {
        if (m_imp)
            return m_imp->init(m_gpu);
        else
            return -1;
    }
    void process_event()
    {
        if(m_imp)
            m_imp->process_event();
    }
    unsigned int get_gpu()
    {
        return m_gpu;
    }
    unsigned int get_width()
    {
        return m_width;
    }
    unsigned int get_height()
    {
        return m_height;
    }

    vktrace_window_handle get_window_handle()
    {
        return m_hWindow;
    }

private:
    DisplayImp *m_imp;
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_gpu;
    bool m_fullscreen;
    vktrace_window_handle m_hWindow;
};

}   // namespace vktrace_replay
