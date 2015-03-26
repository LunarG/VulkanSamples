/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * XGL
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

#include "glvreplay_xgl_replay.h"

#include "glvreplay_xgl.h"

#include "glvreplay_xgl_write_ppm.h"

#include "glvreplay_main.h"

#include <algorithm>
#include <queue>
glvreplay_settings *g_pReplaySettings;
extern "C" {
#include "glvtrace_xgl_xgl_structs.h"
#include "glvtrace_xgl_xgldbg_structs.h"
#include "glvtrace_xgl_xglwsix11ext_structs.h"
#include "glvtrace_xgl_packet_id.h"
#include "xgl_enum_string_helper.h"
}

#define APP_NAME "glvreplay_xgl"
#define IDI_ICON 101

static const char* g_extensions[] =
{
        "XGL_WSI_WINDOWS",
        "XGL_TIMER_QUEUE",
        "XGL_GPU_TIMESTAMP_CALIBRATION",
        "XGL_DMA_QUEUE",
        "XGL_COMMAND_BUFFER_CONTROL_FLOW",
        "XGL_COPY_OCCLUSION_QUERY_DATA",
        "XGL_ADVANCED_MULTISAMPLING",
        "XGL_BORDER_COLOR_PALETTE"
};

XGL_RESULT xglDisplay::init_xgl(unsigned int gpu_idx)
{
#if 0
    XGL_APPLICATION_INFO appInfo = {};
    appInfo.pAppName = APP_NAME;
    appInfo.pEngineName = "";
    appInfo.apiVersion = XGL_API_VERSION;
    XGL_RESULT res = xglInitAndEnumerateGpus(&appInfo, NULL, XGL_MAX_PHYSICAL_GPUS, &m_gpuCount, m_gpus);
    if ( res == XGL_SUCCESS ) {
        // retrieve the GPU information for all GPUs
        for( uint32_t gpu = 0; gpu < m_gpuCount; gpu++)
        {
            size_t gpuInfoSize = sizeof(m_gpuProps[0]);

            // get the GPU physical properties:
            res = xglGetGpuInfo( m_gpus[gpu], XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, &gpuInfoSize, &m_gpuProps[gpu]);
            if (res != XGL_SUCCESS)
                glv_LogWarn("Failed to retrieve properties for gpu[%d] result %d\n", gpu, res);
        }
        res = XGL_SUCCESS;
    } else if ((gpu_idx + 1) > m_gpuCount) {
        glv_LogError("xglInitAndEnumerate number of gpus does not include requested index: num %d, requested %d\n", m_gpuCount, gpu_idx);
        return -1;
    } else {
        glv_LogError("xglInitAndEnumerate failed\n");
        return res;
    }
    // TODO add multi-gpu support always use gpu[gpu_idx] for now
    // get all extensions supported by this device gpu[gpu_idx]
    // first check if extensions are available and save a list of them
    bool foundWSIExt = false;
    for( int ext = 0; ext < sizeof( extensions ) / sizeof( extensions[0] ); ext++)
    {
        res = xglGetExtensionSupport( m_gpus[gpu_idx], extensions[ext] );
        if (res == XGL_SUCCESS) {
            m_extensions.push_back((char *) extensions[ext]);
            if (!strcmp(extensions[ext], "XGL_WSI_WINDOWS"))
                foundWSIExt = true;
        }
    }
    if (!foundWSIExt) {
        glv_LogError("XGL_WSI_WINDOWS extension not supported by gpu[%d]\n", gpu_idx);
        return XGL_ERROR_INCOMPATIBLE_DEVICE;
    }
    // TODO generalize this: use one universal queue for now
    XGL_DEVICE_QUEUE_CREATE_INFO dqci = {};
    dqci.queueCount = 1;
    dqci.queueType = XGL_QUEUE_UNIVERSAL;
    // create the device enabling validation level 4
    const char * const * extNames = &m_extensions[0];
    XGL_DEVICE_CREATE_INFO info = {};
    info.queueRecordCount = 1;
    info.pRequestedQueues = &dqci;
    info.extensionCount = static_cast <uint32_t> (m_extensions.size());
    info.ppEnabledExtensionNames = extNames;
    info.flags = XGL_DEVICE_CREATE_VALIDATION;
    info.maxValidationLevel = XGL_VALIDATION_LEVEL_4;
    bool32_t xglTrue = XGL_TRUE;
    res = xglDbgSetGlobalOption( XGL_DBG_OPTION_BREAK_ON_ERROR, sizeof( xglTrue ), &xglTrue );
    if (res != XGL_SUCCESS)
        glv_LogWarn("Could not set debug option break on error\n");
    res = xglCreateDevice( m_gpus[0], &info, &m_dev[gpu_idx]);
    return res;
#else
    return XGL_ERROR_INITIALIZATION_FAILED;
#endif
}
int xglDisplay::init(const unsigned int gpu_idx)
{
    //m_gpuIdx = gpu_idx;
#if 0
    XGL_RESULT result = init_xgl(gpu_idx);
    if (result != XGL_SUCCESS) {
        glv_LogError("could not init xgl library");
        return -1;
    } else {
        m_initedXGL = true;
    }
#endif
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;
    xcb_connection_t *pConnection;
    pConnection = xcb_connect(NULL, &scr);
    setup = xcb_get_setup(pConnection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);
    m_pXcbScreen = iter.data;
    m_WsiConnection.pConnection = pConnection;
    m_WsiConnection.root = m_pXcbScreen->root;
#endif
    return 0;
}
xglDisplay::xglDisplay()
    : m_initedXGL(false),
    m_windowWidth(0),
    m_windowHeight(0)
{
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    m_WsiConnection.pConnection = NULL;
    m_WsiConnection.root = 0;
    m_WsiConnection.provider = 0;
    m_pXcbScreen = NULL;
    m_XcbWindow = 0;
#elif defined(WIN32)
    m_windowHandle = NULL;
#endif
}
xglDisplay::~xglDisplay()
{
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    if (m_XcbWindow != 0)
    {
        xcb_destroy_window(m_WsiConnection.pConnection, m_XcbWindow);
    }
    if (m_WsiConnection.pConnection != NULL)
    {
        xcb_disconnect(m_WsiConnection.pConnection);
    }
#endif
}
#if defined(WIN32)
LRESULT WINAPI WindowProcXgl( HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow( window);
            // fall-thru
        case WM_DESTROY:
            PostQuitMessage(0) ;
            return 0L ;
        default:
            return DefWindowProc( window, msg, wp, lp ) ;
    }
}
#endif
int xglDisplay::set_window(glv_window_handle hWindow, unsigned int width, unsigned int height)
{
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    m_XcbWindow = hWindow;
#elif defined(WIN32)
    m_windowHandle = hWindow;
#endif
    m_windowWidth = width;
    m_windowHeight = height;
    return 0;
}

int xglDisplay::create_window(const unsigned int width, const unsigned int height)
{
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)

    uint32_t value_mask, value_list[32];
    m_XcbWindow = xcb_generate_id(m_WsiConnection.pConnection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = m_pXcbScreen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(m_WsiConnection.pConnection,
            XCB_COPY_FROM_PARENT,
            m_XcbWindow, m_WsiConnection.root,
            0, 0, width, height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            m_pXcbScreen->root_visual,
            value_mask, value_list);

    xcb_map_window(m_WsiConnection.pConnection, m_XcbWindow);
    xcb_flush(m_WsiConnection.pConnection);
    return 0;
#elif defined(WIN32)
    // Register Window class
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof( WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProcXgl;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = GetModuleHandle(0);
    wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE( IDI_ICON));
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW);
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = APP_NAME;
    wcex.hIconSm = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_ICON));
    if( !RegisterClassEx( &wcex))
    {
        glv_LogError("Failed to register windows class\n");
        return -1;
    }

    // create the window
    m_windowHandle = CreateWindow(APP_NAME, APP_NAME, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 0, 0,
                          width, height, NULL, NULL, wcex.hInstance, NULL);

    if (m_windowHandle)
    {
        ShowWindow( m_windowHandle, SW_SHOWDEFAULT);
        m_windowWidth = width;
        m_windowHeight = height;
    } else {
        glv_LogError("Failed to create window\n");
        return -1;
    }
    return 0;
#endif
}

void xglDisplay::resize_window(const unsigned int width, const unsigned int height)
{
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    if (width != m_windowWidth || height != m_windowHeight)
    {
        uint32_t values[2];
        values[0] = width;
        values[1] = height;
        xcb_configure_window(m_WsiConnection.pConnection, m_XcbWindow, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        m_windowWidth = width;
        m_windowHeight = height;
    }
#elif defined(WIN32)
    if (width != m_windowWidth || height != m_windowHeight)
    {
        SetWindowPos(get_window_handle(), HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
        m_windowWidth = width;
        m_windowHeight = height;
    }
#endif
}

void xglDisplay::process_event()
{
}
xglReplay::xglReplay(glvreplay_settings *pReplaySettings)
{
    g_pReplaySettings = pReplaySettings;
    m_display = new xglDisplay();
    m_pDSDump = NULL;
    m_pCBDump = NULL;
    m_pGlvSnapshotPrint = NULL;
    if (g_pReplaySettings && g_pReplaySettings->screenshotList) {
        process_screenshot_list(g_pReplaySettings->screenshotList);
    }
}

xglReplay::~xglReplay()
{
    delete m_display;
    glv_platform_close_library(m_xglFuncs.m_libHandle);
}
int xglReplay::init(glv_replay::Display & disp)
{
    int err;
#if defined PLATFORM_LINUX
    void * handle = dlopen("libXGL.so", RTLD_LAZY);
#else
    HMODULE handle = LoadLibrary("xgl.dll" );
#endif

    if (handle == NULL) {
        glv_LogError("Failed to open xgl library.\n");
        return -1;
    }
    m_xglFuncs.init_funcs(handle);
    disp.set_implementation(m_display);
    if ((err = m_display->init(disp.get_gpu())) != 0) {
        glv_LogError("Failed to init XGL display.\n");
        return err;
    }
    if (disp.get_window_handle() == 0)
    {
        if ((err = m_display->create_window(disp.get_width(), disp.get_height())) != 0) {
            glv_LogError("Failed to create Window\n");
            return err;
        }
    }
    else
    {
        if ((err = m_display->set_window(disp.get_window_handle(), disp.get_width(), disp.get_height())) != 0)
        {
            glv_LogError("Failed to set Window\n");
            return err;
        }
    }
    return 0;
}
glv_replay::GLV_REPLAY_RESULT xglReplay::handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn)
{
    glv_replay::GLV_REPLAY_RESULT res = resIn;
    if (resCall != resTrace) {
        glv_LogWarn("Mismatched return from API call (%s) traced result %s, replay result %s\n", entrypointName,
                string_XGL_RESULT((XGL_RESULT)resTrace), string_XGL_RESULT((XGL_RESULT)resCall));
        res = glv_replay::GLV_REPLAY_BAD_RETURN;
    }
#if 0
    if (resCall != XGL_SUCCESS) {
        glv_LogWarn("API call (%s) returned failed result %s\n", entrypointName, string_XGL_RESULT(resCall));
    }
#endif
    return res;
}
void xglReplay::push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char * pMsg)
{
    struct validationMsg msgObj;
    msgObj.validationLevel = validationLevel;
    msgObj.srcObject = srcObject;
    msgObj.location = location;
    msgObj.msgCode = msgCode;
    strncpy(msgObj.msg, pMsg, 256);
    msgObj.msg[255] = '\0';
    m_validationMsgs.push_back(msgObj);
}

glv_replay::GLV_REPLAY_RESULT xglReplay::pop_validation_msgs()
{
    if (m_validationMsgs.size() == 0)
        return glv_replay::GLV_REPLAY_SUCCESS;
    m_validationMsgs.clear();
    return glv_replay::GLV_REPLAY_VALIDATION_ERROR;
}
int xglReplay::dump_validation_data()
{
   if  (m_pDSDump && m_pCBDump)
   {
      m_pDSDump((char *) "pipeline_dump.dot");
      m_pCBDump((char *) "cb_dump.dot");
   }
   if (m_pGlvSnapshotPrint != NULL) { m_pGlvSnapshotPrint(); }
   return 0;
}
void xglFuncs::init_funcs(void * handle)
{
    m_libHandle = handle;
    real_xglCreateInstance = (type_xglCreateInstance)(glv_platform_get_library_entrypoint(handle, "xglCreateInstance"));
    real_xglDestroyInstance = (type_xglDestroyInstance)(glv_platform_get_library_entrypoint(handle, "xglDestroyInstance"));
    real_xglEnumerateGpus = (type_xglEnumerateGpus)(glv_platform_get_library_entrypoint(handle, "xglEnumerateGpus"));
    real_xglGetGpuInfo = (type_xglGetGpuInfo)(glv_platform_get_library_entrypoint(handle, "xglGetGpuInfo"));
    real_xglGetProcAddr = (type_xglGetProcAddr)(glv_platform_get_library_entrypoint(handle, "xglGetProcAddr"));
    real_xglCreateDevice = (type_xglCreateDevice)(glv_platform_get_library_entrypoint(handle, "xglCreateDevice"));
    real_xglDestroyDevice = (type_xglDestroyDevice)(glv_platform_get_library_entrypoint(handle, "xglDestroyDevice"));
    real_xglGetExtensionSupport = (type_xglGetExtensionSupport)(glv_platform_get_library_entrypoint(handle, "xglGetExtensionSupport"));
    real_xglEnumerateLayers = (type_xglEnumerateLayers)(glv_platform_get_library_entrypoint(handle, "xglEnumerateLayers"));
    real_xglGetDeviceQueue = (type_xglGetDeviceQueue)(glv_platform_get_library_entrypoint(handle, "xglGetDeviceQueue"));
    real_xglQueueSubmit = (type_xglQueueSubmit)(glv_platform_get_library_entrypoint(handle, "xglQueueSubmit"));
    real_xglQueueSetGlobalMemReferences = (type_xglQueueSetGlobalMemReferences)(glv_platform_get_library_entrypoint(handle, "xglQueueSetGlobalMemReferences"));
    real_xglQueueWaitIdle = (type_xglQueueWaitIdle)(glv_platform_get_library_entrypoint(handle, "xglQueueWaitIdle"));
    real_xglDeviceWaitIdle = (type_xglDeviceWaitIdle)(glv_platform_get_library_entrypoint(handle, "xglDeviceWaitIdle"));
    real_xglAllocMemory = (type_xglAllocMemory)(glv_platform_get_library_entrypoint(handle, "xglAllocMemory"));
    real_xglFreeMemory = (type_xglFreeMemory)(glv_platform_get_library_entrypoint(handle, "xglFreeMemory"));
    real_xglSetMemoryPriority = (type_xglSetMemoryPriority)(glv_platform_get_library_entrypoint(handle, "xglSetMemoryPriority"));
    real_xglMapMemory = (type_xglMapMemory)(glv_platform_get_library_entrypoint(handle, "xglMapMemory"));
    real_xglUnmapMemory = (type_xglUnmapMemory)(glv_platform_get_library_entrypoint(handle, "xglUnmapMemory"));
    real_xglPinSystemMemory = (type_xglPinSystemMemory)(glv_platform_get_library_entrypoint(handle, "xglPinSystemMemory"));
    real_xglGetMultiGpuCompatibility = (type_xglGetMultiGpuCompatibility)(glv_platform_get_library_entrypoint(handle, "xglGetMultiGpuCompatibility"));
    real_xglOpenSharedMemory = (type_xglOpenSharedMemory)(glv_platform_get_library_entrypoint(handle, "xglOpenSharedMemory"));
    real_xglOpenSharedQueueSemaphore = (type_xglOpenSharedQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglOpenSharedQueueSemaphore"));
    real_xglOpenPeerMemory = (type_xglOpenPeerMemory)(glv_platform_get_library_entrypoint(handle, "xglOpenPeerMemory"));
    real_xglOpenPeerImage = (type_xglOpenPeerImage)(glv_platform_get_library_entrypoint(handle, "xglOpenPeerImage"));
    real_xglDestroyObject = (type_xglDestroyObject)(glv_platform_get_library_entrypoint(handle, "xglDestroyObject"));
    real_xglGetObjectInfo = (type_xglGetObjectInfo)(glv_platform_get_library_entrypoint(handle, "xglGetObjectInfo"));
    real_xglBindObjectMemory = (type_xglBindObjectMemory)(glv_platform_get_library_entrypoint(handle, "xglBindObjectMemory"));
    real_xglBindObjectMemoryRange = (type_xglBindObjectMemoryRange)(glv_platform_get_library_entrypoint(handle, "xglBindObjectMemoryRange"));
    real_xglBindImageMemoryRange = (type_xglBindImageMemoryRange)(glv_platform_get_library_entrypoint(handle, "xglBindImageMemoryRange"));
    real_xglCreateFence = (type_xglCreateFence)(glv_platform_get_library_entrypoint(handle, "xglCreateFence"));
    real_xglGetFenceStatus = (type_xglGetFenceStatus)(glv_platform_get_library_entrypoint(handle, "xglGetFenceStatus"));
    real_xglWaitForFences = (type_xglWaitForFences)(glv_platform_get_library_entrypoint(handle, "xglWaitForFences"));
    real_xglCreateQueueSemaphore = (type_xglCreateQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglCreateQueueSemaphore"));
    real_xglSignalQueueSemaphore = (type_xglSignalQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglSignalQueueSemaphore"));
    real_xglWaitQueueSemaphore = (type_xglWaitQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglWaitQueueSemaphore"));
    real_xglCreateEvent = (type_xglCreateEvent)(glv_platform_get_library_entrypoint(handle, "xglCreateEvent"));
    real_xglGetEventStatus = (type_xglGetEventStatus)(glv_platform_get_library_entrypoint(handle, "xglGetEventStatus"));
    real_xglSetEvent = (type_xglSetEvent)(glv_platform_get_library_entrypoint(handle, "xglSetEvent"));
    real_xglResetEvent = (type_xglResetEvent)(glv_platform_get_library_entrypoint(handle, "xglResetEvent"));
    real_xglCreateQueryPool = (type_xglCreateQueryPool)(glv_platform_get_library_entrypoint(handle, "xglCreateQueryPool"));
    real_xglGetQueryPoolResults = (type_xglGetQueryPoolResults)(glv_platform_get_library_entrypoint(handle, "xglGetQueryPoolResults"));
    real_xglGetFormatInfo = (type_xglGetFormatInfo)(glv_platform_get_library_entrypoint(handle, "xglGetFormatInfo"));
    real_xglCreateBuffer = (type_xglCreateBuffer)(glv_platform_get_library_entrypoint(handle, "xglCreateBuffer"));
    real_xglCreateBufferView = (type_xglCreateBufferView)(glv_platform_get_library_entrypoint(handle, "xglCreateBufferView"));
    real_xglCreateImage = (type_xglCreateImage)(glv_platform_get_library_entrypoint(handle, "xglCreateImage"));
    real_xglSetFastClearColor = (type_xglSetFastClearColor)(glv_platform_get_library_entrypoint(handle, "xglSetFastClearColor"));
    real_xglSetFastClearDepth = (type_xglSetFastClearDepth)(glv_platform_get_library_entrypoint(handle, "xglSetFastClearDepth"));
    real_xglGetImageSubresourceInfo = (type_xglGetImageSubresourceInfo)(glv_platform_get_library_entrypoint(handle, "xglGetImageSubresourceInfo"));
    real_xglCreateImageView = (type_xglCreateImageView)(glv_platform_get_library_entrypoint(handle, "xglCreateImageView"));
    real_xglCreateColorAttachmentView = (type_xglCreateColorAttachmentView)(glv_platform_get_library_entrypoint(handle, "xglCreateColorAttachmentView"));
    real_xglCreateDepthStencilView = (type_xglCreateDepthStencilView)(glv_platform_get_library_entrypoint(handle, "xglCreateDepthStencilView"));
    real_xglCreateShader = (type_xglCreateShader)(glv_platform_get_library_entrypoint(handle, "xglCreateShader"));
    real_xglCreateGraphicsPipeline = (type_xglCreateGraphicsPipeline)(glv_platform_get_library_entrypoint(handle, "xglCreateGraphicsPipeline"));
    real_xglCreateComputePipeline = (type_xglCreateComputePipeline)(glv_platform_get_library_entrypoint(handle, "xglCreateComputePipeline"));
    real_xglStorePipeline = (type_xglStorePipeline)(glv_platform_get_library_entrypoint(handle, "xglStorePipeline"));
    real_xglLoadPipeline = (type_xglLoadPipeline)(glv_platform_get_library_entrypoint(handle, "xglLoadPipeline"));
    real_xglCreatePipelineDelta = (type_xglCreatePipelineDelta)(glv_platform_get_library_entrypoint(handle, "xglCreatePipelineDelta"));
    real_xglCreateSampler = (type_xglCreateSampler)(glv_platform_get_library_entrypoint(handle, "xglCreateSampler"));
    real_xglCreateDescriptorSetLayout = (type_xglCreateDescriptorSetLayout)(glv_platform_get_library_entrypoint(handle, "xglCreateDescriptorSetLayout"));
    real_xglBeginDescriptorRegionUpdate = (type_xglBeginDescriptorRegionUpdate)(glv_platform_get_library_entrypoint(handle, "xglBeginDescriptorRegionUpdate"));
    real_xglEndDescriptorRegionUpdate = (type_xglEndDescriptorRegionUpdate)(glv_platform_get_library_entrypoint(handle, "xglEndDescriptorRegionUpdate"));
    real_xglCreateDescriptorRegion = (type_xglCreateDescriptorRegion)(glv_platform_get_library_entrypoint(handle, "xglCreateDescriptorRegion"));
    real_xglClearDescriptorRegion = (type_xglClearDescriptorRegion)(glv_platform_get_library_entrypoint(handle, "xglClearDescriptorRegion"));
    real_xglAllocDescriptorSets = (type_xglAllocDescriptorSets)(glv_platform_get_library_entrypoint(handle, "xglAllocDescriptorSets"));
    real_xglClearDescriptorSets = (type_xglClearDescriptorSets)(glv_platform_get_library_entrypoint(handle, "xglClearDescriptorSets"));
    real_xglUpdateDescriptors = (type_xglUpdateDescriptors)(glv_platform_get_library_entrypoint(handle, "xglUpdateDescriptors"));
    real_xglCreateDynamicViewportState = (type_xglCreateDynamicViewportState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicViewportState"));
    real_xglCreateDynamicRasterState = (type_xglCreateDynamicRasterState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicRasterState"));
    real_xglCreateDynamicColorBlendState = (type_xglCreateDynamicColorBlendState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicColorBlendState"));
    real_xglCreateDynamicDepthStencilState = (type_xglCreateDynamicDepthStencilState)(glv_platform_get_library_entrypoint(handle, "xglCreateDynamicDepthStencilState"));
    real_xglCreateCommandBuffer = (type_xglCreateCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglCreateCommandBuffer"));
    real_xglBeginCommandBuffer = (type_xglBeginCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglBeginCommandBuffer"));
    real_xglEndCommandBuffer = (type_xglEndCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglEndCommandBuffer"));
    real_xglResetCommandBuffer = (type_xglResetCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglResetCommandBuffer"));
    real_xglCmdBindPipeline = (type_xglCmdBindPipeline)(glv_platform_get_library_entrypoint(handle, "xglCmdBindPipeline"));
    real_xglCmdBindPipelineDelta = (type_xglCmdBindPipelineDelta)(glv_platform_get_library_entrypoint(handle, "xglCmdBindPipelineDelta"));
    real_xglCmdBindDynamicStateObject = (type_xglCmdBindDynamicStateObject)(glv_platform_get_library_entrypoint(handle, "xglCmdBindDynamicStateObject"));
    real_xglCmdBindDescriptorSet = (type_xglCmdBindDescriptorSet)(glv_platform_get_library_entrypoint(handle, "xglCmdBindDescriptorSet"));
    real_xglCmdBindVertexBuffer = (type_xglCmdBindVertexBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdBindVertexBuffer"));
    real_xglCmdBindIndexBuffer = (type_xglCmdBindIndexBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdBindIndexBuffer"));
    real_xglCmdDraw = (type_xglCmdDraw)(glv_platform_get_library_entrypoint(handle, "xglCmdDraw"));
    real_xglCmdDrawIndexed = (type_xglCmdDrawIndexed)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndexed"));
    real_xglCmdDrawIndirect = (type_xglCmdDrawIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndirect"));
    real_xglCmdDrawIndexedIndirect = (type_xglCmdDrawIndexedIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndexedIndirect"));
    real_xglCmdDispatch = (type_xglCmdDispatch)(glv_platform_get_library_entrypoint(handle, "xglCmdDispatch"));
    real_xglCmdDispatchIndirect = (type_xglCmdDispatchIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDispatchIndirect"));
    real_xglCmdCopyBuffer = (type_xglCmdCopyBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyBuffer"));
    real_xglCmdCopyImage = (type_xglCmdCopyImage)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyImage"));
    real_xglCmdCopyBufferToImage = (type_xglCmdCopyBufferToImage)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyBufferToImage"));
    real_xglCmdCopyImageToBuffer = (type_xglCmdCopyImageToBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyImageToBuffer"));
    real_xglCmdCloneImageData = (type_xglCmdCloneImageData)(glv_platform_get_library_entrypoint(handle, "xglCmdCloneImageData"));
    real_xglCmdUpdateBuffer = (type_xglCmdUpdateBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdUpdateBuffer"));
    real_xglCmdFillBuffer = (type_xglCmdFillBuffer)(glv_platform_get_library_entrypoint(handle, "xglCmdFillBuffer"));
    real_xglCmdClearColorImage = (type_xglCmdClearColorImage)(glv_platform_get_library_entrypoint(handle, "xglCmdClearColorImage"));
    real_xglCmdClearColorImageRaw = (type_xglCmdClearColorImageRaw)(glv_platform_get_library_entrypoint(handle, "xglCmdClearColorImageRaw"));
    real_xglCmdClearDepthStencil = (type_xglCmdClearDepthStencil)(glv_platform_get_library_entrypoint(handle, "xglCmdClearDepthStencil"));
    real_xglCmdResolveImage = (type_xglCmdResolveImage)(glv_platform_get_library_entrypoint(handle, "xglCmdResolveImage"));
    real_xglCmdSetEvent = (type_xglCmdSetEvent)(glv_platform_get_library_entrypoint(handle, "xglCmdSetEvent"));
    real_xglCmdResetEvent = (type_xglCmdResetEvent)(glv_platform_get_library_entrypoint(handle, "xglCmdResetEvent"));
    real_xglCmdWaitEvents = (type_xglCmdWaitEvents)(glv_platform_get_library_entrypoint(handle, "xglCmdWaitEvents"));
    real_xglCmdPipelineBarrier = (type_xglCmdPipelineBarrier)(glv_platform_get_library_entrypoint(handle, "xglCmdPipelineBarrier"));
    real_xglCmdBeginQuery = (type_xglCmdBeginQuery)(glv_platform_get_library_entrypoint(handle, "xglCmdBeginQuery"));
    real_xglCmdEndQuery = (type_xglCmdEndQuery)(glv_platform_get_library_entrypoint(handle, "xglCmdEndQuery"));
    real_xglCmdResetQueryPool = (type_xglCmdResetQueryPool)(glv_platform_get_library_entrypoint(handle, "xglCmdResetQueryPool"));
    real_xglCmdWriteTimestamp = (type_xglCmdWriteTimestamp)(glv_platform_get_library_entrypoint(handle, "xglCmdWriteTimestamp"));
    real_xglCmdInitAtomicCounters = (type_xglCmdInitAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdInitAtomicCounters"));
    real_xglCmdLoadAtomicCounters = (type_xglCmdLoadAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdLoadAtomicCounters"));
    real_xglCmdSaveAtomicCounters = (type_xglCmdSaveAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdSaveAtomicCounters"));
    real_xglCreateFramebuffer = (type_xglCreateFramebuffer)(glv_platform_get_library_entrypoint(handle, "xglCreateFramebuffer"));
    real_xglCreateRenderPass = (type_xglCreateRenderPass)(glv_platform_get_library_entrypoint(handle, "xglCreateRenderPass"));
    real_xglCmdBeginRenderPass = (type_xglCmdBeginRenderPass)(glv_platform_get_library_entrypoint(handle, "xglCmdBeginRenderPass"));
    real_xglCmdEndRenderPass = (type_xglCmdEndRenderPass)(glv_platform_get_library_entrypoint(handle, "xglCmdEndRenderPass"));
    real_xglDbgSetValidationLevel = (type_xglDbgSetValidationLevel)(glv_platform_get_library_entrypoint(handle, "xglDbgSetValidationLevel"));
    real_xglDbgRegisterMsgCallback = (type_xglDbgRegisterMsgCallback)(glv_platform_get_library_entrypoint(handle, "xglDbgRegisterMsgCallback"));
    real_xglDbgUnregisterMsgCallback = (type_xglDbgUnregisterMsgCallback)(glv_platform_get_library_entrypoint(handle, "xglDbgUnregisterMsgCallback"));
    real_xglDbgSetMessageFilter = (type_xglDbgSetMessageFilter)(glv_platform_get_library_entrypoint(handle, "xglDbgSetMessageFilter"));
    real_xglDbgSetObjectTag = (type_xglDbgSetObjectTag)(glv_platform_get_library_entrypoint(handle, "xglDbgSetObjectTag"));
    real_xglDbgSetGlobalOption = (type_xglDbgSetGlobalOption)(glv_platform_get_library_entrypoint(handle, "xglDbgSetGlobalOption"));
    real_xglDbgSetDeviceOption = (type_xglDbgSetDeviceOption)(glv_platform_get_library_entrypoint(handle, "xglDbgSetDeviceOption"));
    real_xglCmdDbgMarkerBegin = (type_xglCmdDbgMarkerBegin)(glv_platform_get_library_entrypoint(handle, "xglCmdDbgMarkerBegin"));
    real_xglCmdDbgMarkerEnd = (type_xglCmdDbgMarkerEnd)(glv_platform_get_library_entrypoint(handle, "xglCmdDbgMarkerEnd"));
    real_xglWsiX11AssociateConnection = (type_xglWsiX11AssociateConnection)(glv_platform_get_library_entrypoint(handle, "xglWsiX11AssociateConnection"));
    real_xglWsiX11GetMSC = (type_xglWsiX11GetMSC)(glv_platform_get_library_entrypoint(handle, "xglWsiX11GetMSC"));
    real_xglWsiX11CreatePresentableImage = (type_xglWsiX11CreatePresentableImage)(glv_platform_get_library_entrypoint(handle, "xglWsiX11CreatePresentableImage"));
    real_xglWsiX11QueuePresent = (type_xglWsiX11QueuePresent)(glv_platform_get_library_entrypoint(handle, "xglWsiX11QueuePresent"));
}
#define CHECK_RETURN_VALUE(entrypoint) returnValue = handle_replay_errors(#entrypoint, replayResult, pPacket->result, returnValue);

glv_replay::GLV_REPLAY_RESULT xglReplay::replay(glv_trace_packet_header *packet)
{
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    switch (packet->packet_id)
    {
        case GLV_TPI_XGL_xglApiVersion:
            break;  // nothing to replay on the version packet
        case GLV_TPI_XGL_xglCreateInstance:
        {
            struct_xglCreateInstance* pPacket = (struct_xglCreateInstance*)(packet->pBody);
            XGL_INSTANCE local_pInstance;
            replayResult = m_xglFuncs.real_xglCreateInstance(pPacket->pAppInfo, pPacket->pAllocCb, &local_pInstance);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pInstance, &local_pInstance);
            }
            CHECK_RETURN_VALUE(xglCreateInstance);
            break;
        }
        case GLV_TPI_XGL_xglDestroyInstance:
        {
            struct_xglDestroyInstance* pPacket = (struct_xglDestroyInstance*)(packet->pBody);
            xglDbgUnregisterMsgCallback(g_fpDbgMsgCallback);
            replayResult = m_xglFuncs.real_xglDestroyInstance(remap(pPacket->instance));
            if (replayResult == XGL_SUCCESS)
            {
                // TODO need to handle multiple instances and only clearing maps within an instance.
                // TODO this only works with a single instance used at any given time.
                clear_all_map_handles();
            }
            CHECK_RETURN_VALUE(xglDestroyInstance);
            break;
        }
        case GLV_TPI_XGL_xglEnumerateGpus:
        {
            struct_xglEnumerateGpus* pPacket = (struct_xglEnumerateGpus*)(packet->pBody);
            if (!m_display->m_initedXGL)
            {
                uint32_t gpuCount;
                XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
                uint32_t maxGpus = (pPacket->maxGpus < XGL_MAX_PHYSICAL_GPUS) ? pPacket->maxGpus : XGL_MAX_PHYSICAL_GPUS;
                replayResult = m_xglFuncs.real_xglEnumerateGpus(remap(pPacket->instance), maxGpus, &gpuCount, &gpus[0]);
                CHECK_RETURN_VALUE(xglEnumerateGpus);
                //TODO handle different number of gpus in trace versus replay
                if (gpuCount != *(pPacket->pGpuCount))
                {
                    glv_LogWarn("number of gpus mismatched in replay %u versus trace %u\n", gpuCount, *(pPacket->pGpuCount));
                }
                else if (gpuCount == 0)
                {
                     glv_LogError("xglEnumerateGpus number of gpus is zero\n");
                }
                else
                {
                    glv_LogInfo("Enumerated %d GPUs in the system\n", gpuCount);
                }
                // TODO handle enumeration results in a different order from trace to replay
                for (uint32_t i = 0; i < gpuCount; i++)
                {
                    if (pPacket->pGpus)
                        add_to_map(&(pPacket->pGpus[i]), &(gpus[i]));
                }
            }
            break;
        }
        case GLV_TPI_XGL_xglGetGpuInfo:
        {
            struct_xglGetGpuInfo* pPacket = (struct_xglGetGpuInfo*)(packet->pBody);
            if (!m_display->m_initedXGL)
            {
                switch (pPacket->infoType) {
                case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
                {
                    XGL_PHYSICAL_GPU_PROPERTIES gpuProps;
                    size_t dataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,
                                    (pPacket->pData == NULL) ? NULL : &gpuProps);
                    if (pPacket->pData != NULL)
                    {
                        glv_LogInfo("Replay Gpu Properties\n");
                        glv_LogInfo("Vendor ID %x, Device ID %x, name %s\n",gpuProps.vendorId, gpuProps.deviceId, gpuProps.gpuName);
                        glv_LogInfo("API version %u, Driver version %u, gpu Type %u\n",gpuProps.apiVersion, gpuProps.driverVersion, gpuProps.gpuType);
                    }
                    break;
                }
                case XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:
                {
                    XGL_PHYSICAL_GPU_PERFORMANCE gpuPerfs;
                    size_t dataSize = sizeof(XGL_PHYSICAL_GPU_PERFORMANCE);
                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,
                                    (pPacket->pData == NULL) ? NULL : &gpuPerfs);
                    if (pPacket->pData != NULL)
                    {
                        glv_LogInfo("Replay Gpu Performance\n");
                        glv_LogInfo("Max GPU clock %f, max shader ALUs/clock %f, max texel fetches/clock %f\n",gpuPerfs.maxGpuClock, gpuPerfs.aluPerClock, gpuPerfs.texPerClock);
                        glv_LogInfo("Max primitives/clock %f, Max pixels/clock %f\n",gpuPerfs.primsPerClock, gpuPerfs.pixelsPerClock);
                    }
                    break;
                }
                case XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:
                {
                    XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *pGpuQueue, *pQ;
                    size_t dataSize = sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);
                    size_t numQueues = 1;
                    assert(pPacket->pDataSize);
                    if ((*(pPacket->pDataSize) % dataSize) != 0)
                        glv_LogWarn("xglGetGpuInfo() for GPU_QUEUE_PROPERTIES not an integral data size assuming 1\n");
                    else
                        numQueues = *(pPacket->pDataSize) / dataSize;
                    dataSize = numQueues * dataSize;
                    pQ = static_cast < XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *> (glv_malloc(dataSize));
                    pGpuQueue = pQ;
                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,
                                    (pPacket->pData == NULL) ? NULL : pGpuQueue);
                    if (pPacket->pData != NULL)
                    {
                        for (unsigned int i = 0; i < numQueues; i++)
                        {
                            glv_LogInfo("Replay Gpu Queue Property for index %d, flags %u\n", i, pGpuQueue->queueFlags);
                            glv_LogInfo("Max available count %u, max atomic counters %u, supports timestamps %u\n",pGpuQueue->queueCount, pGpuQueue->maxAtomicCounters, pGpuQueue->supportsTimestamps);
                            pGpuQueue++;
                        }
                    }
                    glv_free(pQ);
                    break;
                }
                default:
                {
                    size_t size = 0;
                    void* pData = NULL;
                    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
                    {
                        size = *pPacket->pDataSize;
                        pData = glv_malloc(*pPacket->pDataSize);
                    }
                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &size, pData);
                    if (replayResult == XGL_SUCCESS)
                    {
                        if (size != *pPacket->pDataSize && pData != NULL)
                        {
                            glv_LogWarn("xglGetGpuInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                        }
                        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                        {
                            glv_LogWarn("xglGetGpuInfo returned differing data contents than the trace file contained.\n");
                        }
                    }
                    glv_free(pData);
                    break;
                }
                };
                CHECK_RETURN_VALUE(xglGetGpuInfo);
            }
            break;
        }
        case GLV_TPI_XGL_xglGetProcAddr:
        {
            struct_xglGetProcAddr* pPacket = (struct_xglGetProcAddr*)(packet->pBody);
            m_xglFuncs.real_xglGetProcAddr(remap(pPacket->gpu), pPacket->pName);
            break;
        }
        case GLV_TPI_XGL_xglCreateDevice:
        {
            struct_xglCreateDevice* pPacket = (struct_xglCreateDevice*)(packet->pBody);
            if (!m_display->m_initedXGL)
            {
                XGL_DEVICE device;
                if (g_xglReplaySettings.debugLevel > 0)
                {
                    XGL_DEVICE_CREATE_INFO cInfo, *ci, *pCreateInfoSaved;
                    unsigned int numLayers = 0;
                    char ** layersStr = get_enableLayers_list(&numLayers);
                    apply_layerSettings_overrides();
                    XGL_LAYER_CREATE_INFO layerInfo;
                    pCreateInfoSaved = (XGL_DEVICE_CREATE_INFO *) pPacket->pCreateInfo;
                    ci = (XGL_DEVICE_CREATE_INFO *) pPacket->pCreateInfo;
                    if (layersStr != NULL && numLayers > 0)
                    {
                        while (ci->pNext != NULL)
                            ci = (XGL_DEVICE_CREATE_INFO *) ci->pNext;
                        ci->pNext = &layerInfo;
                        layerInfo.sType = XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO;
                        layerInfo.pNext = 0;
                        layerInfo.layerCount = numLayers;
                        layerInfo.ppActiveLayerNames = layersStr;
                    }
                    memcpy(&cInfo, pPacket->pCreateInfo, sizeof(XGL_DEVICE_CREATE_INFO));
                    cInfo.flags = pPacket->pCreateInfo->flags | XGL_DEVICE_CREATE_VALIDATION_BIT;
                    cInfo.maxValidationLevel = (XGL_VALIDATION_LEVEL)((g_xglReplaySettings.debugLevel <= 4) ? (unsigned int) XGL_VALIDATION_LEVEL_0 + g_xglReplaySettings.debugLevel : (unsigned int) XGL_VALIDATION_LEVEL_0);
                    pPacket->pCreateInfo = &cInfo;
                    replayResult = m_xglFuncs.real_xglCreateDevice(remap(pPacket->gpu), pPacket->pCreateInfo, &device);
                    // restore the packet for next replay
                    ci->pNext = NULL;
                    pPacket->pCreateInfo = pCreateInfoSaved;
                    release_enableLayer_list(layersStr);
                    if (xglDbgRegisterMsgCallback(g_fpDbgMsgCallback, NULL) != XGL_SUCCESS)
                        glv_LogError("Failed to register xgl callback for replayer error handling\n");
#if !defined(_WIN32)
                    m_pDSDump = (DRAW_STATE_DUMP_DOT_FILE) m_xglFuncs.real_xglGetProcAddr(remap(pPacket->gpu), "drawStateDumpDotFile");
                    m_pCBDump = (DRAW_STATE_DUMP_COMMAND_BUFFER_DOT_FILE) m_xglFuncs.real_xglGetProcAddr(remap(pPacket->gpu), "drawStateDumpCommandBufferDotFile");
                    m_pGlvSnapshotPrint = (GLVSNAPSHOT_PRINT_OBJECTS) m_xglFuncs.real_xglGetProcAddr(remap(pPacket->gpu), "glvSnapshotPrintObjects");
#endif
                }
                else 
                    replayResult = m_xglFuncs.real_xglCreateDevice(remap(pPacket->gpu), pPacket->pCreateInfo, &device);
                CHECK_RETURN_VALUE(xglCreateDevice);
                if (replayResult == XGL_SUCCESS)
                {
                    add_to_map(pPacket->pDevice, &device);
                }
            }
            break;
        }
        case GLV_TPI_XGL_xglDestroyDevice:
        {
            struct_xglDestroyDevice* pPacket = (struct_xglDestroyDevice*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDestroyDevice(remap(pPacket->device));
            if (replayResult == XGL_SUCCESS)
            {
                m_pCBDump = NULL;
                m_pDSDump = NULL;
                m_pGlvSnapshotPrint = NULL;
                rm_from_map(pPacket->device);
                m_display->m_initedXGL = false;
            }
            CHECK_RETURN_VALUE(xglDestroyDevice);
            break;
        }
        case GLV_TPI_XGL_xglGetExtensionSupport:
        {
            struct_xglGetExtensionSupport* pPacket = (struct_xglGetExtensionSupport*)(packet->pBody);
            if (!m_display->m_initedXGL) {
                replayResult = m_xglFuncs.real_xglGetExtensionSupport(remap(pPacket->gpu), pPacket->pExtName);
                CHECK_RETURN_VALUE(xglGetExtensionSupport);
                if (replayResult == XGL_SUCCESS) {
                    for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)
                    {
                        if (!strncmp(g_extensions[ext], pPacket->pExtName, strlen(g_extensions[ext]))) {
                            bool extInList = false;
                            for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {
                                if (!strncmp(m_display->m_extensions[j], g_extensions[ext], strlen(g_extensions[ext])))
                                    extInList = true;
                                break;
                            }
                            if (!extInList)
                                m_display->m_extensions.push_back((char *) g_extensions[ext]);
                            break;
                        }
                    }
                }
            }
            break;
        }
        case GLV_TPI_XGL_xglEnumerateLayers:
        {
            struct_xglEnumerateLayers* pPacket = (struct_xglEnumerateLayers*)(packet->pBody);
            char **bufptr = GLV_NEW_ARRAY(char *, pPacket->maxLayerCount);
            char **ptrLayers = (pPacket->pOutLayers == NULL) ? bufptr : (char **) pPacket->pOutLayers;
            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)
                bufptr[i] = GLV_NEW_ARRAY(char, pPacket->maxStringSize);
            replayResult = m_xglFuncs.real_xglEnumerateLayers(remap(pPacket->gpu), pPacket->maxLayerCount, pPacket->maxStringSize, pPacket->pOutLayerCount, ptrLayers, pPacket->pReserved);
            for (unsigned int i = 0; i < pPacket->maxLayerCount; i++)
                GLV_DELETE(bufptr[i]);
            CHECK_RETURN_VALUE(xglEnumerateLayers);
            break;
        }
        case GLV_TPI_XGL_xglGetDeviceQueue:
        {
            struct_xglGetDeviceQueue* pPacket = (struct_xglGetDeviceQueue*)(packet->pBody);
            XGL_QUEUE local_pQueue;
            replayResult = m_xglFuncs.real_xglGetDeviceQueue(remap(pPacket->device), pPacket->queueType, pPacket->queueIndex, &local_pQueue);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pQueue, &local_pQueue);
            }
            CHECK_RETURN_VALUE(xglGetDeviceQueue);
            break;
        }
        case GLV_TPI_XGL_xglQueueSubmit:
        {
            struct_xglQueueSubmit* pPacket = (struct_xglQueueSubmit*)(packet->pBody);
            XGL_CMD_BUFFER *remappedBuffers = NULL;
            if (pPacket->pCmdBuffers != NULL)
            {
                remappedBuffers = GLV_NEW_ARRAY( XGL_CMD_BUFFER, pPacket->cmdBufferCount);
                for (uint32_t i = 0; i < pPacket->cmdBufferCount; i++)
                {
                    *(remappedBuffers + i) = remap(*(pPacket->pCmdBuffers + i));
                }
            }
            XGL_MEMORY_REF* memRefs = NULL;
            if (pPacket->pMemRefs != NULL)
            {
                memRefs = GLV_NEW_ARRAY(XGL_MEMORY_REF, pPacket->memRefCount);
                memcpy(memRefs, pPacket->pMemRefs, sizeof(XGL_MEMORY_REF) * pPacket->memRefCount);
                for (uint32_t i = 0; i < pPacket->memRefCount; i++)
                {
                    memRefs[i].mem = remap(pPacket->pMemRefs[i].mem);
                }
            }
            replayResult = m_xglFuncs.real_xglQueueSubmit(remap(pPacket->queue), pPacket->cmdBufferCount, remappedBuffers, pPacket->memRefCount,
                memRefs, remap(pPacket->fence));
            GLV_DELETE(remappedBuffers);
            GLV_DELETE(memRefs);
            CHECK_RETURN_VALUE(xglQueueSubmit);
            break;
        }
        case GLV_TPI_XGL_xglQueueSetGlobalMemReferences:
        {
            struct_xglQueueSetGlobalMemReferences* pPacket = (struct_xglQueueSetGlobalMemReferences*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglQueueSetGlobalMemReferences(remap(pPacket->queue), pPacket->memRefCount, pPacket->pMemRefs);
            CHECK_RETURN_VALUE(xglQueueSetGlobalMemReferences);
            break;
        }
        case GLV_TPI_XGL_xglQueueWaitIdle:
        {
            struct_xglQueueWaitIdle* pPacket = (struct_xglQueueWaitIdle*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglQueueWaitIdle(remap(pPacket->queue));
            CHECK_RETURN_VALUE(xglQueueWaitIdle);
            break;
        }
        case GLV_TPI_XGL_xglDeviceWaitIdle:
        {
            struct_xglDeviceWaitIdle* pPacket = (struct_xglDeviceWaitIdle*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDeviceWaitIdle(remap(pPacket->device));
            CHECK_RETURN_VALUE(xglDeviceWaitIdle);
            break;
        }
        case GLV_TPI_XGL_xglAllocMemory:
        {
            struct_xglAllocMemory* pPacket = (struct_xglAllocMemory*)(packet->pBody);
            XGL_GPU_MEMORY local_pMem;
            replayResult = m_xglFuncs.real_xglAllocMemory(remap(pPacket->device), pPacket->pAllocInfo, &local_pMem);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pMem, &local_pMem);
                add_entry_to_mapData(local_pMem, pPacket->pAllocInfo->allocationSize);
            }
            CHECK_RETURN_VALUE(xglAllocMemory);
            break;
        }
        case GLV_TPI_XGL_xglFreeMemory:
        {
            struct_xglFreeMemory* pPacket = (struct_xglFreeMemory*)(packet->pBody);
            XGL_GPU_MEMORY handle = remap(pPacket->mem);
            replayResult = m_xglFuncs.real_xglFreeMemory(handle);
            if (replayResult == XGL_SUCCESS) 
            {
                rm_entry_from_mapData(handle);
                rm_from_map(pPacket->mem);
            }
            CHECK_RETURN_VALUE(xglFreeMemory);
            break;
        }
        case GLV_TPI_XGL_xglSetMemoryPriority:
        {
            struct_xglSetMemoryPriority* pPacket = (struct_xglSetMemoryPriority*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetMemoryPriority(remap(pPacket->mem), pPacket->priority);
            CHECK_RETURN_VALUE(xglSetMemoryPriority);
            break;
        }
        case GLV_TPI_XGL_xglMapMemory:
        {
            struct_xglMapMemory* pPacket = (struct_xglMapMemory*)(packet->pBody);
            XGL_GPU_MEMORY handle = remap(pPacket->mem);
            void* pData;
            replayResult = m_xglFuncs.real_xglMapMemory(handle, pPacket->flags, &pData);
            if (replayResult == XGL_SUCCESS)
                add_mapping_to_mapData(handle, pData);
            CHECK_RETURN_VALUE(xglMapMemory);
            break;
        }
        case GLV_TPI_XGL_xglUnmapMemory:
        {
            struct_xglUnmapMemory* pPacket = (struct_xglUnmapMemory*)(packet->pBody);
            XGL_GPU_MEMORY handle = remap(pPacket->mem);
            rm_mapping_from_mapData(handle, pPacket->pData);  // copies data from packet into memory buffer
            replayResult = m_xglFuncs.real_xglUnmapMemory(handle);
            CHECK_RETURN_VALUE(xglUnmapMemory);
            break;
        }
        case GLV_TPI_XGL_xglPinSystemMemory:
        {
            struct_xglPinSystemMemory* pPacket = (struct_xglPinSystemMemory*)(packet->pBody);
            XGL_GPU_MEMORY local_pMem;
            replayResult = m_xglFuncs.real_xglPinSystemMemory(remap(pPacket->device), pPacket->pSysMem, pPacket->memSize, &local_pMem);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pMem, &local_pMem);
            }
            CHECK_RETURN_VALUE(xglPinSystemMemory);
            break;
        }
        case GLV_TPI_XGL_xglGetMultiGpuCompatibility:
        {
            struct_xglGetMultiGpuCompatibility* pPacket = (struct_xglGetMultiGpuCompatibility*)(packet->pBody);
            XGL_GPU_COMPATIBILITY_INFO cInfo;
            XGL_PHYSICAL_GPU handle0, handle1;
            handle0 = remap(pPacket->gpu0);
            handle1 = remap(pPacket->gpu1);
            replayResult = m_xglFuncs.real_xglGetMultiGpuCompatibility(handle0, handle1, &cInfo);
            CHECK_RETURN_VALUE(xglGetMultiGpuCompatibility);
            break;
        }
        case GLV_TPI_XGL_xglOpenSharedMemory:
        {
            struct_xglOpenSharedMemory* pPacket = (struct_xglOpenSharedMemory*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_GPU_MEMORY local_pMem;
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenSharedMemory(handle, pPacket->pOpenInfo, &local_pMem);
            CHECK_RETURN_VALUE(xglOpenSharedMemory);
            break;
        }
        case GLV_TPI_XGL_xglOpenSharedQueueSemaphore:
        {
            struct_xglOpenSharedQueueSemaphore* pPacket = (struct_xglOpenSharedQueueSemaphore*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_QUEUE_SEMAPHORE local_pSemaphore;
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenSharedQueueSemaphore(handle, pPacket->pOpenInfo, &local_pSemaphore);
            CHECK_RETURN_VALUE(xglOpenSharedQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglOpenPeerMemory:
        {
            struct_xglOpenPeerMemory* pPacket = (struct_xglOpenPeerMemory*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_GPU_MEMORY local_pMem;
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenPeerMemory(handle, pPacket->pOpenInfo, &local_pMem);
            CHECK_RETURN_VALUE(xglOpenPeerMemory);
            break;
        }
        case GLV_TPI_XGL_xglOpenPeerImage:
        {
            struct_xglOpenPeerImage* pPacket = (struct_xglOpenPeerImage*)(packet->pBody);
            XGL_DEVICE handle;
            XGL_GPU_MEMORY local_pMem;
            XGL_IMAGE local_pImage;
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenPeerImage(handle, pPacket->pOpenInfo, &local_pImage, &local_pMem);
            CHECK_RETURN_VALUE(xglOpenPeerImage);
            break;
        }
        case GLV_TPI_XGL_xglDestroyObject:
        {
            struct_xglDestroyObject* pPacket = (struct_xglDestroyObject*)(packet->pBody);
            XGL_OBJECT object = remap(pPacket->object);
            if (object != XGL_NULL_HANDLE)
                replayResult = m_xglFuncs.real_xglDestroyObject(object);
            if (replayResult == XGL_SUCCESS)
                rm_from_map(pPacket->object);
            CHECK_RETURN_VALUE(xglDestroyObject);
            break;
        }
        case GLV_TPI_XGL_xglGetObjectInfo:
        {
            struct_xglGetObjectInfo* pPacket = (struct_xglGetObjectInfo*)(packet->pBody);
            size_t size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
                memcpy(pData, pPacket->pData, *pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglGetObjectInfo(remap(pPacket->object), pPacket->infoType, &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData != NULL)
                {
                    glv_LogWarn("xglGetObjectInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                }
                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                {
                    glv_LogWarn("xglGetObjectInfo returned differing data contents than the trace file contained.\n");
                }
            }
            glv_free(pData);
            CHECK_RETURN_VALUE(xglGetObjectInfo);
            break;
        }
        case GLV_TPI_XGL_xglBindObjectMemory:
        {
            struct_xglBindObjectMemory* pPacket = (struct_xglBindObjectMemory*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBindObjectMemory(remap(pPacket->object), pPacket->allocationIdx, remap(pPacket->mem), pPacket->offset);
            CHECK_RETURN_VALUE(xglBindObjectMemory);
            break;
        }
        case GLV_TPI_XGL_xglBindObjectMemoryRange:
        {
            struct_xglBindObjectMemoryRange* pPacket = (struct_xglBindObjectMemoryRange*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBindObjectMemoryRange(remap(pPacket->object), pPacket->allocationIdx, pPacket->rangeOffset, pPacket->rangeSize, remap(pPacket->mem), pPacket->memOffset);
            CHECK_RETURN_VALUE(xglBindObjectMemoryRange);
            break;
        }
        case GLV_TPI_XGL_xglBindImageMemoryRange:
        {
            struct_xglBindImageMemoryRange* pPacket = (struct_xglBindImageMemoryRange*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBindImageMemoryRange(remap(pPacket->image), pPacket->allocationIdx, pPacket->bindInfo, remap(pPacket->mem), pPacket->memOffset);
            CHECK_RETURN_VALUE(xglBindImageMemoryRange);
            break;
        }
        case GLV_TPI_XGL_xglCreateFence:
        {
            struct_xglCreateFence* pPacket = (struct_xglCreateFence*)(packet->pBody);
            XGL_FENCE local_pFence;
            replayResult = m_xglFuncs.real_xglCreateFence(remap(pPacket->device), pPacket->pCreateInfo, &local_pFence);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pFence, &local_pFence);
            }
            CHECK_RETURN_VALUE(xglCreateFence);
            break;
        }
        case GLV_TPI_XGL_xglGetFenceStatus:
        {
            struct_xglGetFenceStatus* pPacket = (struct_xglGetFenceStatus*)(packet->pBody);
            do {
                replayResult = m_xglFuncs.real_xglGetFenceStatus(remap(pPacket->fence));
            } while (replayResult != pPacket->result  && pPacket->result == XGL_SUCCESS);
            if (pPacket->result != XGL_NOT_READY || replayResult != XGL_SUCCESS)
            CHECK_RETURN_VALUE(xglGetFenceStatus);
            break;
        }
        case GLV_TPI_XGL_xglWaitForFences:
        {
            struct_xglWaitForFences* pPacket = (struct_xglWaitForFences*)(packet->pBody);
            XGL_FENCE *pFence = GLV_NEW_ARRAY(XGL_FENCE, pPacket->fenceCount);
            for (uint32_t i = 0; i < pPacket->fenceCount; i++)
            {
                *(pFence + i) = remap(*(pPacket->pFences + i));
            }
            replayResult = m_xglFuncs.real_xglWaitForFences(remap(pPacket->device), pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);
            GLV_DELETE(pFence);
            CHECK_RETURN_VALUE(xglWaitForFences);
            break;
        }
        case GLV_TPI_XGL_xglCreateQueueSemaphore:
        {
            struct_xglCreateQueueSemaphore* pPacket = (struct_xglCreateQueueSemaphore*)(packet->pBody);
            XGL_QUEUE_SEMAPHORE local_pSemaphore;
            replayResult = m_xglFuncs.real_xglCreateQueueSemaphore(remap(pPacket->device), pPacket->pCreateInfo, &local_pSemaphore);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pSemaphore, &local_pSemaphore);
            }
            CHECK_RETURN_VALUE(xglCreateQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglSignalQueueSemaphore:
        {
            struct_xglSignalQueueSemaphore* pPacket = (struct_xglSignalQueueSemaphore*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSignalQueueSemaphore(remap(pPacket->queue), remap(pPacket->semaphore));
            CHECK_RETURN_VALUE(xglSignalQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglWaitQueueSemaphore:
        {
            struct_xglWaitQueueSemaphore* pPacket = (struct_xglWaitQueueSemaphore*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglWaitQueueSemaphore(remap(pPacket->queue), remap(pPacket->semaphore));
            CHECK_RETURN_VALUE(xglWaitQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglCreateEvent:
        {
            struct_xglCreateEvent* pPacket = (struct_xglCreateEvent*)(packet->pBody);
            XGL_EVENT local_pEvent;
            replayResult = m_xglFuncs.real_xglCreateEvent(remap(pPacket->device), pPacket->pCreateInfo, &local_pEvent);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pEvent, &local_pEvent);
            }
            CHECK_RETURN_VALUE(xglCreateEvent);
            break;
        }
        case GLV_TPI_XGL_xglGetEventStatus:
        {
            struct_xglGetEventStatus* pPacket = (struct_xglGetEventStatus*)(packet->pBody);
            do {
                replayResult = m_xglFuncs.real_xglGetEventStatus(remap(pPacket->event));
            } while ((pPacket->result == XGL_EVENT_SET || pPacket->result == XGL_EVENT_RESET) && replayResult != pPacket->result);
            if (pPacket->result != XGL_NOT_READY || replayResult != XGL_SUCCESS)
            CHECK_RETURN_VALUE(xglGetEventStatus);
            break;
        }
        case GLV_TPI_XGL_xglSetEvent:
        {
            struct_xglSetEvent* pPacket = (struct_xglSetEvent*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetEvent(remap(pPacket->event));
            CHECK_RETURN_VALUE(xglSetEvent);
            break;
        }
        case GLV_TPI_XGL_xglResetEvent:
        {
            struct_xglResetEvent* pPacket = (struct_xglResetEvent*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglResetEvent(remap(pPacket->event));
            CHECK_RETURN_VALUE(xglResetEvent);
            break;
        }
        case GLV_TPI_XGL_xglCreateQueryPool:
        {
            struct_xglCreateQueryPool* pPacket = (struct_xglCreateQueryPool*)(packet->pBody);
            XGL_QUERY_POOL local_pQueryPool;
            replayResult = m_xglFuncs.real_xglCreateQueryPool(remap(pPacket->device), pPacket->pCreateInfo, &local_pQueryPool);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pQueryPool, &local_pQueryPool);
            }
            CHECK_RETURN_VALUE(xglCreateQueryPool);
            break;
        }
        case GLV_TPI_XGL_xglGetQueryPoolResults:
        {
            struct_xglGetQueryPoolResults* pPacket = (struct_xglGetQueryPoolResults*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglGetQueryPoolResults(remap(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount, pPacket->pDataSize, pPacket->pData);
            CHECK_RETURN_VALUE(xglGetQueryPoolResults);
            break;
        }
        case GLV_TPI_XGL_xglGetFormatInfo:
        {
            struct_xglGetFormatInfo* pPacket = (struct_xglGetFormatInfo*)(packet->pBody);
            size_t size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglGetFormatInfo(remap(pPacket->device), pPacket->format, pPacket->infoType, &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData != NULL)
                {
                    glv_LogWarn("xglGetFormatInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                }
                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                {
                    glv_LogWarn("xglGetFormatInfo returned differing data contents than the trace file contained.\n");
                }
            }
            glv_free(pData);
            CHECK_RETURN_VALUE(xglGetFormatInfo);
            break;
        }
        case GLV_TPI_XGL_xglCreateBuffer:
        {
            struct_xglCreateBuffer* pPacket = (struct_xglCreateBuffer*)(packet->pBody);
            XGL_BUFFER local_pBuffer;
            replayResult = m_xglFuncs.real_xglCreateBuffer(remap(pPacket->device), pPacket->pCreateInfo, &local_pBuffer);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pBuffer, &local_pBuffer);
            }
            CHECK_RETURN_VALUE(xglCreateBuffer);
            break;
        }
        case GLV_TPI_XGL_xglCreateBufferView:
        {
            struct_xglCreateBufferView* pPacket = (struct_xglCreateBufferView*)(packet->pBody);
            XGL_BUFFER_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_BUFFER_VIEW_CREATE_INFO));
            createInfo.buffer = remap(pPacket->pCreateInfo->buffer);
            XGL_BUFFER_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateBufferView(remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateBufferView);
            break;
        }
        case GLV_TPI_XGL_xglCreateImage:
        {
            struct_xglCreateImage* pPacket = (struct_xglCreateImage*)(packet->pBody);
            XGL_IMAGE local_pImage;
            replayResult = m_xglFuncs.real_xglCreateImage(remap(pPacket->device), pPacket->pCreateInfo, &local_pImage);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pImage, &local_pImage);
            }
            CHECK_RETURN_VALUE(xglCreateImage);
            break;
        }
        case GLV_TPI_XGL_xglSetFastClearColor:
        {
            struct_xglSetFastClearColor* pPacket = (struct_xglSetFastClearColor*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetFastClearColor(remap(pPacket->image), pPacket->color);
            CHECK_RETURN_VALUE(xglSetFastClearColor);
            break;
        }
        case GLV_TPI_XGL_xglSetFastClearDepth:
        {
            struct_xglSetFastClearDepth* pPacket = (struct_xglSetFastClearDepth*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglSetFastClearDepth(remap(pPacket->image), pPacket->depth);
            CHECK_RETURN_VALUE(xglSetFastClearDepth);
            break;
        }
        case GLV_TPI_XGL_xglGetImageSubresourceInfo:
        {
            struct_xglGetImageSubresourceInfo* pPacket = (struct_xglGetImageSubresourceInfo*)(packet->pBody);
            size_t size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglGetImageSubresourceInfo(remap(pPacket->image), pPacket->pSubresource, pPacket->infoType, &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData != NULL)
                {
                    glv_LogWarn("xglGetImageSubresourceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                }
                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                {
                    glv_LogWarn("xglGetImageSubresourceInfo returned differing data contents than the trace file contained.\n");
                }
            }
            glv_free(pData);
            CHECK_RETURN_VALUE(xglGetImageSubresourceInfo);
            break;
        }
        case GLV_TPI_XGL_xglCreateImageView:
        {
            struct_xglCreateImageView* pPacket = (struct_xglCreateImageView*)(packet->pBody);
            XGL_IMAGE_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_IMAGE_VIEW_CREATE_INFO));
            createInfo.image = remap(pPacket->pCreateInfo->image);
            XGL_IMAGE_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateImageView(remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateImageView);
            break;
        }
        case GLV_TPI_XGL_xglCreateColorAttachmentView:
        {
            struct_xglCreateColorAttachmentView* pPacket = (struct_xglCreateColorAttachmentView*)(packet->pBody);
            XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO));
            createInfo.image = remap(pPacket->pCreateInfo->image);
            XGL_COLOR_ATTACHMENT_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateColorAttachmentView(remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateColorAttachmentView);
            break;
        }
        case GLV_TPI_XGL_xglCreateDepthStencilView:
        {
            struct_xglCreateDepthStencilView* pPacket = (struct_xglCreateDepthStencilView*)(packet->pBody);
            XGL_DEPTH_STENCIL_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO));
            createInfo.image = remap(pPacket->pCreateInfo->image);
            XGL_DEPTH_STENCIL_VIEW local_pView;
            replayResult = m_xglFuncs.real_xglCreateDepthStencilView(remap(pPacket->device), &createInfo, &local_pView);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pView, &local_pView);
            }
            CHECK_RETURN_VALUE(xglCreateDepthStencilView);
            break;
        }
        case GLV_TPI_XGL_xglCreateShader:
        {
            struct_xglCreateShader* pPacket = (struct_xglCreateShader*)(packet->pBody);
            XGL_SHADER local_pShader;
            replayResult = m_xglFuncs.real_xglCreateShader(remap(pPacket->device), pPacket->pCreateInfo, &local_pShader);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pShader, &local_pShader);
            }
            CHECK_RETURN_VALUE(xglCreateShader);
            break;
        }
        case GLV_TPI_XGL_xglCreateGraphicsPipeline:
        {
            struct_xglCreateGraphicsPipeline* pPacket = (struct_xglCreateGraphicsPipeline*)(packet->pBody);
            XGL_GRAPHICS_PIPELINE_CREATE_INFO createInfo;
            struct shaderPair saveShader[10];
            unsigned int idx = 0;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));
            createInfo.lastSetLayout = remap(createInfo.lastSetLayout);
            // Cast to shader type, as those are of primariy interest and all structs in LL have same header w/ sType & pNext
            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;
            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)createInfo.pNext;
            while (XGL_NULL_HANDLE != pPacketNext)
            {
                if (XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO == pNext->sType)
                {
                    saveShader[idx].val = pNext->shader.shader;
                    saveShader[idx++].addr = &(pNext->shader.shader);
                    pNext->shader.shader = remap(pPacketNext->shader.shader);
                }
                pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacketNext->pNext;
                pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;
            }
            XGL_PIPELINE pipeline;
            replayResult = m_xglFuncs.real_xglCreateGraphicsPipeline(remap(pPacket->device), &createInfo, &pipeline);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pPipeline, &pipeline);
            }
            for (unsigned int i = 0; i < idx; i++)
                *(saveShader[i].addr) = saveShader[i].val;
            CHECK_RETURN_VALUE(xglCreateGraphicsPipeline);
            break;
        }
        case GLV_TPI_XGL_xglCreateComputePipeline:
        {
            struct_xglCreateComputePipeline* pPacket = (struct_xglCreateComputePipeline*)(packet->pBody);
            XGL_COMPUTE_PIPELINE_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO));
            createInfo.cs.shader = remap(pPacket->pCreateInfo->cs.shader);
            XGL_PIPELINE local_pPipeline;
            replayResult = m_xglFuncs.real_xglCreateComputePipeline(remap(pPacket->device), &createInfo, &local_pPipeline);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pPipeline, &local_pPipeline);
            }
            CHECK_RETURN_VALUE(xglCreateComputePipeline);
            break;
        }
        case GLV_TPI_XGL_xglStorePipeline:
        {
            struct_xglStorePipeline* pPacket = (struct_xglStorePipeline*)(packet->pBody);
            size_t size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglStorePipeline(remap(pPacket->pipeline), &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData != NULL)
                {
                    glv_LogWarn("xglStorePipeline returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                }
                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                {
                    glv_LogWarn("xglStorePipeline returned differing data contents than the trace file contained.\n");
                }
            }
            glv_free(pData);
            CHECK_RETURN_VALUE(xglStorePipeline);
            break;
        }
        case GLV_TPI_XGL_xglLoadPipeline:
        {
            struct_xglLoadPipeline* pPacket = (struct_xglLoadPipeline*)(packet->pBody);
            XGL_PIPELINE local_pPipeline;
            replayResult = m_xglFuncs.real_xglLoadPipeline(remap(pPacket->device), pPacket->dataSize, pPacket->pData, &local_pPipeline);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pPipeline, &local_pPipeline);
            }
            CHECK_RETURN_VALUE(xglLoadPipeline);
            break;
        }
        case GLV_TPI_XGL_xglCreatePipelineDelta:
        {
            struct_xglCreatePipelineDelta* pPacket = (struct_xglCreatePipelineDelta*)(packet->pBody);
            XGL_PIPELINE_DELTA local_delta;
            replayResult = m_xglFuncs.real_xglCreatePipelineDelta(remap(pPacket->device), pPacket->p1, pPacket->p2, &local_delta);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->delta, &local_delta);
            }
            CHECK_RETURN_VALUE(xglCreatePipelineDelta);
            break;
        }
        case GLV_TPI_XGL_xglCreateSampler:
        {
            struct_xglCreateSampler* pPacket = (struct_xglCreateSampler*)(packet->pBody);
            XGL_SAMPLER local_pSampler;
            replayResult = m_xglFuncs.real_xglCreateSampler(remap(pPacket->device), pPacket->pCreateInfo, &local_pSampler);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pSampler, &local_pSampler);
            }
            CHECK_RETURN_VALUE(xglCreateSampler);
            break;
        }
        case GLV_TPI_XGL_xglCreateDescriptorSetLayout:
        {
            struct_xglCreateDescriptorSetLayout* pPacket = (struct_xglCreateDescriptorSetLayout*)(packet->pBody);
            XGL_SAMPLER saveSampler;
            if (pPacket->pSetLayoutInfoList != NULL) {
                XGL_SAMPLER *pSampler = (XGL_SAMPLER *) &pPacket->pSetLayoutInfoList->immutableSampler;
                saveSampler = pPacket->pSetLayoutInfoList->immutableSampler;
                *pSampler = remap(saveSampler);
            }
            XGL_DESCRIPTOR_SET_LAYOUT setLayout;
            replayResult = m_xglFuncs.real_xglCreateDescriptorSetLayout(remap(pPacket->device), pPacket->stageFlags, pPacket->pSetBindPoints, remap(pPacket->priorSetLayout), pPacket->pSetLayoutInfoList, &setLayout);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pSetLayout, &setLayout);
            }
            if (pPacket->pSetLayoutInfoList != NULL) {
                XGL_SAMPLER *pSampler = (XGL_SAMPLER *) &pPacket->pSetLayoutInfoList->immutableSampler;
                *pSampler = saveSampler;
            }
            CHECK_RETURN_VALUE(xglCreateDescriptorSetLayout);
            break;
        }
        case GLV_TPI_XGL_xglBeginDescriptorRegionUpdate:
        {
            struct_xglBeginDescriptorRegionUpdate* pPacket = (struct_xglBeginDescriptorRegionUpdate*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglBeginDescriptorRegionUpdate(remap(pPacket->device), pPacket->updateMode);
            CHECK_RETURN_VALUE(xglBeginDescriptorRegionUpdate);
            break;
        }
        case GLV_TPI_XGL_xglEndDescriptorRegionUpdate:
        {
            struct_xglEndDescriptorRegionUpdate* pPacket = (struct_xglEndDescriptorRegionUpdate*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglEndDescriptorRegionUpdate(remap(pPacket->device), remap(pPacket->cmd));
            CHECK_RETURN_VALUE(xglEndDescriptorRegionUpdate);
            break;
        }
        case GLV_TPI_XGL_xglCreateDescriptorRegion:
        {
            struct_xglCreateDescriptorRegion* pPacket = (struct_xglCreateDescriptorRegion*)(packet->pBody);
            XGL_DESCRIPTOR_REGION local_pDescriptorRegion;
            replayResult = m_xglFuncs.real_xglCreateDescriptorRegion(remap(pPacket->device), pPacket->regionUsage, pPacket->maxSets, pPacket->pCreateInfo, &local_pDescriptorRegion);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pDescriptorRegion, &local_pDescriptorRegion);
            }
            CHECK_RETURN_VALUE(xglCreateDescriptorRegion);
            break;
        }
        case GLV_TPI_XGL_xglClearDescriptorRegion:
        {
            struct_xglClearDescriptorRegion* pPacket = (struct_xglClearDescriptorRegion*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglClearDescriptorRegion(remap(pPacket->descriptorRegion));
            CHECK_RETURN_VALUE(xglClearDescriptorRegion);
            break;
        }
        case GLV_TPI_XGL_xglAllocDescriptorSets:
        {
            struct_xglAllocDescriptorSets* pPacket = (struct_xglAllocDescriptorSets*)(packet->pBody);
            uint32_t local_pCount;
            XGL_DESCRIPTOR_SET local_pDescriptorSets[100];
            XGL_DESCRIPTOR_SET_LAYOUT localDescSets[100];
            assert(pPacket->count <= 100);
            for (uint32_t i = 0; i < pPacket->count; i++)
            {
                localDescSets[i] = remap(pPacket->pSetLayouts[i]);
            }
            replayResult = m_xglFuncs.real_xglAllocDescriptorSets(remap(pPacket->descriptorRegion), pPacket->setUsage, pPacket->count, localDescSets, local_pDescriptorSets, &local_pCount);
            if (replayResult == XGL_SUCCESS)
            {
                for (uint32_t i = 0; i < local_pCount; i++) {
                    add_to_map(&pPacket->pDescriptorSets[i], &local_pDescriptorSets[i]);
                }
            }
            CHECK_RETURN_VALUE(xglAllocDescriptorSets);
            break;
        }
        case GLV_TPI_XGL_xglClearDescriptorSets:
        {
            struct_xglClearDescriptorSets* pPacket = (struct_xglClearDescriptorSets*)(packet->pBody);
            XGL_DESCRIPTOR_SET localDescSets[100];
            assert(pPacket->count <= 100);
            for (uint32_t i = 0; i < pPacket->count; i++)
            {
                localDescSets[i] = remap(pPacket->pDescriptorSets[i]);
            }
            m_xglFuncs.real_xglClearDescriptorSets(remap(pPacket->descriptorRegion), pPacket->count, localDescSets);
            break;
        }
        case GLV_TPI_XGL_xglUpdateDescriptors:
        {
            struct_xglUpdateDescriptors* pPacket = (struct_xglUpdateDescriptors*)(packet->pBody);
            XGL_UPDATE_SAMPLERS* pUpdateChain = (XGL_UPDATE_SAMPLERS*)pPacket->pUpdateChain;
            std::queue<XGL_SAMPLER> saveSamplers;
            std::queue<XGL_BUFFER_VIEW> saveBufferViews;
            std::queue<XGL_IMAGE_VIEW> saveImageViews;
            std::queue<XGL_DESCRIPTOR_SET> saveDescSets;
            while (pUpdateChain) {
                switch(pUpdateChain->sType)
                {
                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                        for (uint32_t i = 0; i < ((XGL_UPDATE_SAMPLERS*)pUpdateChain)->count; i++) {
                            XGL_SAMPLER* pLocalSampler = (XGL_SAMPLER*) &((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i];
                            saveSamplers.push(*pLocalSampler);
                            *pLocalSampler = remap(((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i]);
                        }
                        break;
                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                    {
                        XGL_UPDATE_SAMPLER_TEXTURES *pUST = (XGL_UPDATE_SAMPLER_TEXTURES *) pUpdateChain;
                        for (uint32_t i = 0; i < pUST->count; i++) {
                            XGL_SAMPLER *pLocalSampler = (XGL_SAMPLER *) &pUST->pSamplerImageViews[i].pSampler;
                            saveSamplers.push(*pLocalSampler);
                            *pLocalSampler = remap(pUST->pSamplerImageViews[i].pSampler);
                            XGL_IMAGE_VIEW *pLocalView = (XGL_IMAGE_VIEW *) &pUST->pSamplerImageViews[i].pImageView->view;
                            saveImageViews.push(*pLocalView);
                            *pLocalView = remap(pUST->pSamplerImageViews[i].pImageView->view);
                        }
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
                    {
                        XGL_UPDATE_IMAGES *pUI = (XGL_UPDATE_IMAGES*) pUpdateChain;
                        for (uint32_t i = 0; i < pUI->count; i++) {
                            XGL_IMAGE_VIEW* pLocalView = (XGL_IMAGE_VIEW*) &pUI->pImageViews[i]->view;
                            saveImageViews.push(*pLocalView);
                            *pLocalView = remap(pUI->pImageViews[i]->view);
                        }
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
                    {
                        XGL_UPDATE_BUFFERS *pUB = (XGL_UPDATE_BUFFERS *) pUpdateChain;
                        for (uint32_t i = 0; i < pUB->count; i++) {
                            XGL_BUFFER_VIEW* pLocalView = (XGL_BUFFER_VIEW*) &pUB->pBufferViews[i]->view;
                            saveBufferViews.push(*pLocalView);
                            *pLocalView = remap(pUB->pBufferViews[i]->view);
                        }
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
                        saveDescSets.push(((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet);
                        ((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet = remap(((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet);
                        break;
                    default:
                        assert(0);
                        break;
                }
                pUpdateChain = (XGL_UPDATE_SAMPLERS*) pUpdateChain->pNext;
            }
            m_xglFuncs.real_xglUpdateDescriptors(remap(pPacket->descriptorSet), pPacket->pUpdateChain);
            pUpdateChain = (XGL_UPDATE_SAMPLERS*) pPacket->pUpdateChain;
            while (pUpdateChain) {
                switch(pUpdateChain->sType)
                {
                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                        for (uint32_t i = 0; i < ((XGL_UPDATE_SAMPLERS*)pUpdateChain)->count; i++) {
                            XGL_SAMPLER* pLocalSampler = (XGL_SAMPLER*) &((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i];
                            *pLocalSampler = saveSamplers.front();
                            saveSamplers.pop();
                        }
                        break;
                    case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
                    {
                        XGL_UPDATE_SAMPLER_TEXTURES *pUST = (XGL_UPDATE_SAMPLER_TEXTURES *) pUpdateChain;
                        for (uint32_t i = 0; i < pUST->count; i++) {
                            XGL_SAMPLER *plocalSampler = (XGL_SAMPLER *) &pUST->pSamplerImageViews[i].pSampler;
                            *plocalSampler = saveSamplers.front();
                            saveSamplers.pop();
                            XGL_IMAGE_VIEW *pLocalView = (XGL_IMAGE_VIEW *) &pUST->pSamplerImageViews[i].pImageView->view;
                            *pLocalView = saveImageViews.front();
                            saveImageViews.pop();
                        }
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
                    {
                        XGL_UPDATE_IMAGES *pUI = (XGL_UPDATE_IMAGES*) pUpdateChain;
                        for (uint32_t i = 0; i < pUI->count; i++) {
                            XGL_IMAGE_VIEW* pLocalView = (XGL_IMAGE_VIEW*) &pUI->pImageViews[i]->view;
                            *pLocalView = saveImageViews.front();
                            saveImageViews.pop();
                        }
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
                    {
                        XGL_UPDATE_BUFFERS *pUB = (XGL_UPDATE_BUFFERS *) pUpdateChain;
                        for (uint32_t i = 0; i < pUB->count; i++) {
                            XGL_BUFFER_VIEW* pLocalView = (XGL_BUFFER_VIEW*) &pUB->pBufferViews[i]->view;
                            *pLocalView = saveBufferViews.front();
                            saveBufferViews.pop();
                        }
                        break;
                    }
                    case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
                        ((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet = saveDescSets.front();
                        saveDescSets.pop();
                        //pFreeMe = (XGL_UPDATE_SAMPLERS*)pLocalUpdateChain;
                        //pLocalUpdateChain = (void*)((XGL_UPDATE_SAMPLERS*)pLocalUpdateChain)->pNext;
                        //free(pFreeMe);
                        break;
                    default:
                        assert(0);
                        break;
                }
                pUpdateChain = (XGL_UPDATE_SAMPLERS*) pUpdateChain->pNext;
            }
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicViewportState:
        {
            struct_xglCreateDynamicViewportState* pPacket = (struct_xglCreateDynamicViewportState*)(packet->pBody);
            XGL_DYNAMIC_VP_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicViewportState(remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicViewportState);
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicRasterState:
        {
            struct_xglCreateDynamicRasterState* pPacket = (struct_xglCreateDynamicRasterState*)(packet->pBody);
            XGL_DYNAMIC_RS_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicRasterState(remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicRasterState);
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicColorBlendState:
        {
            struct_xglCreateDynamicColorBlendState* pPacket = (struct_xglCreateDynamicColorBlendState*)(packet->pBody);
            XGL_DYNAMIC_CB_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicColorBlendState(remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicColorBlendState);
            break;
        }
        case GLV_TPI_XGL_xglCreateDynamicDepthStencilState:
        {
            struct_xglCreateDynamicDepthStencilState* pPacket = (struct_xglCreateDynamicDepthStencilState*)(packet->pBody);
            XGL_DYNAMIC_DS_STATE_OBJECT local_pState;
            replayResult = m_xglFuncs.real_xglCreateDynamicDepthStencilState(remap(pPacket->device), pPacket->pCreateInfo, &local_pState);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &local_pState);
            }
            CHECK_RETURN_VALUE(xglCreateDynamicDepthStencilState);
            break;
        }
        case GLV_TPI_XGL_xglCreateCommandBuffer:
        {
            struct_xglCreateCommandBuffer* pPacket = (struct_xglCreateCommandBuffer*)(packet->pBody);
            XGL_CMD_BUFFER local_pCmdBuffer;
            replayResult = m_xglFuncs.real_xglCreateCommandBuffer(remap(pPacket->device), pPacket->pCreateInfo, &local_pCmdBuffer);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pCmdBuffer, &local_pCmdBuffer);
            }
            CHECK_RETURN_VALUE(xglCreateCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglBeginCommandBuffer:
        {
            struct_xglBeginCommandBuffer* pPacket = (struct_xglBeginCommandBuffer*)(packet->pBody);
            XGL_CMD_BUFFER_BEGIN_INFO* pInfo = (XGL_CMD_BUFFER_BEGIN_INFO*)pPacket->pBeginInfo;
            // assume only zero or one graphics_begin_info in the chain
            XGL_RENDER_PASS savedRP, *pRP;
            XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO *pGInfo = NULL;
            while (pInfo != NULL)
            {

                if (pInfo->sType == XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO)
                {
                    pGInfo = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO *) pInfo;
                    savedRP = pGInfo->renderPass;
                    pRP = &(pGInfo->renderPass);
                    *pRP = remap(pGInfo->renderPass);
                    break;
                }
                pInfo = (XGL_CMD_BUFFER_BEGIN_INFO*) pInfo->pNext;
            }
            replayResult = m_xglFuncs.real_xglBeginCommandBuffer(remap(pPacket->cmdBuffer), pPacket->pBeginInfo);
            if (pGInfo != NULL)
                pGInfo->renderPass = savedRP;
            CHECK_RETURN_VALUE(xglBeginCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglEndCommandBuffer:
        {
            struct_xglEndCommandBuffer* pPacket = (struct_xglEndCommandBuffer*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglEndCommandBuffer(remap(pPacket->cmdBuffer));
            CHECK_RETURN_VALUE(xglEndCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglResetCommandBuffer:
        {
            struct_xglResetCommandBuffer* pPacket = (struct_xglResetCommandBuffer*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglResetCommandBuffer(remap(pPacket->cmdBuffer));
            CHECK_RETURN_VALUE(xglResetCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindPipeline:
        {
            struct_xglCmdBindPipeline* pPacket = (struct_xglCmdBindPipeline*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindPipeline(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, remap(pPacket->pipeline));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindPipelineDelta:
        {
            struct_xglCmdBindPipelineDelta* pPacket = (struct_xglCmdBindPipelineDelta*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindPipelineDelta(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, remap(pPacket->delta));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindDynamicStateObject:
        {
            struct_xglCmdBindDynamicStateObject* pPacket = (struct_xglCmdBindDynamicStateObject*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindDynamicStateObject(remap(pPacket->cmdBuffer), pPacket->stateBindPoint, remap(pPacket->state));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindDescriptorSet:
        {
            struct_xglCmdBindDescriptorSet* pPacket = (struct_xglCmdBindDescriptorSet*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindDescriptorSet(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, remap(pPacket->descriptorSet), pPacket->pUserData);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindVertexBuffer:
        {
            struct_xglCmdBindVertexBuffer* pPacket = (struct_xglCmdBindVertexBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindVertexBuffer(remap(pPacket->cmdBuffer), remap(pPacket->buffer), pPacket->offset, pPacket->binding);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindIndexBuffer:
        {
            struct_xglCmdBindIndexBuffer* pPacket = (struct_xglCmdBindIndexBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdBindIndexBuffer(remap(pPacket->cmdBuffer), remap(pPacket->buffer), pPacket->offset, pPacket->indexType);
            break;
        }
        case GLV_TPI_XGL_xglCmdDraw:
        {
            struct_xglCmdDraw* pPacket = (struct_xglCmdDraw*)(packet->pBody);
            m_xglFuncs.real_xglCmdDraw(remap(pPacket->cmdBuffer), pPacket->firstVertex, pPacket->vertexCount, pPacket->firstInstance, pPacket->instanceCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexed:
        {
            struct_xglCmdDrawIndexed* pPacket = (struct_xglCmdDrawIndexed*)(packet->pBody);
            m_xglFuncs.real_xglCmdDrawIndexed(remap(pPacket->cmdBuffer), pPacket->firstIndex, pPacket->indexCount, pPacket->vertexOffset, pPacket->firstInstance, pPacket->instanceCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndirect:
        {
            struct_xglCmdDrawIndirect* pPacket = (struct_xglCmdDrawIndirect*)(packet->pBody);
            m_xglFuncs.real_xglCmdDrawIndirect(remap(pPacket->cmdBuffer), remap(pPacket->buffer), pPacket->offset, pPacket->count, pPacket->stride);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexedIndirect:
        {
            struct_xglCmdDrawIndexedIndirect* pPacket = (struct_xglCmdDrawIndexedIndirect*)(packet->pBody);
            m_xglFuncs.real_xglCmdDrawIndexedIndirect(remap(pPacket->cmdBuffer), remap(pPacket->buffer), pPacket->offset, pPacket->count, pPacket->stride);
            break;
        }
        case GLV_TPI_XGL_xglCmdDispatch:
        {
            struct_xglCmdDispatch* pPacket = (struct_xglCmdDispatch*)(packet->pBody);
            m_xglFuncs.real_xglCmdDispatch(remap(pPacket->cmdBuffer), pPacket->x, pPacket->y, pPacket->z);
            break;
        }
        case GLV_TPI_XGL_xglCmdDispatchIndirect:
        {
            struct_xglCmdDispatchIndirect* pPacket = (struct_xglCmdDispatchIndirect*)(packet->pBody);
            m_xglFuncs.real_xglCmdDispatchIndirect(remap(pPacket->cmdBuffer), remap(pPacket->buffer), pPacket->offset);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyBuffer:
        {
            struct_xglCmdCopyBuffer* pPacket = (struct_xglCmdCopyBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyBuffer(remap(pPacket->cmdBuffer), remap(pPacket->srcBuffer), remap(pPacket->destBuffer), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyImage:
        {
            struct_xglCmdCopyImage* pPacket = (struct_xglCmdCopyImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyImage(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), remap(pPacket->destImage), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyBufferToImage:
        {
            struct_xglCmdCopyBufferToImage* pPacket = (struct_xglCmdCopyBufferToImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyBufferToImage(remap(pPacket->cmdBuffer), remap(pPacket->srcBuffer), remap(pPacket->destImage), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyImageToBuffer:
        {
            struct_xglCmdCopyImageToBuffer* pPacket = (struct_xglCmdCopyImageToBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdCopyImageToBuffer(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), remap(pPacket->destBuffer), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCloneImageData:
        {
            struct_xglCmdCloneImageData* pPacket = (struct_xglCmdCloneImageData*)(packet->pBody);
            m_xglFuncs.real_xglCmdCloneImageData(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), pPacket->srcImageLayout, remap(pPacket->destImage), pPacket->destImageLayout);
            break;
        }
        case GLV_TPI_XGL_xglCmdUpdateBuffer:
        {
            struct_xglCmdUpdateBuffer* pPacket = (struct_xglCmdUpdateBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdUpdateBuffer(remap(pPacket->cmdBuffer), remap(pPacket->destBuffer), pPacket->destOffset, pPacket->dataSize, pPacket->pData);
            break;
        }
        case GLV_TPI_XGL_xglCmdFillBuffer:
        {
            struct_xglCmdFillBuffer* pPacket = (struct_xglCmdFillBuffer*)(packet->pBody);
            m_xglFuncs.real_xglCmdFillBuffer(remap(pPacket->cmdBuffer), remap(pPacket->destBuffer), pPacket->destOffset, pPacket->fillSize, pPacket->data);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearColorImage:
        {
            struct_xglCmdClearColorImage* pPacket = (struct_xglCmdClearColorImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdClearColorImage(remap(pPacket->cmdBuffer), remap(pPacket->image), pPacket->color, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearColorImageRaw:
        {
            struct_xglCmdClearColorImageRaw* pPacket = (struct_xglCmdClearColorImageRaw*)(packet->pBody);
            m_xglFuncs.real_xglCmdClearColorImageRaw(remap(pPacket->cmdBuffer), remap(pPacket->image), pPacket->color, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearDepthStencil:
        {
            struct_xglCmdClearDepthStencil* pPacket = (struct_xglCmdClearDepthStencil*)(packet->pBody);
            m_xglFuncs.real_xglCmdClearDepthStencil(remap(pPacket->cmdBuffer), remap(pPacket->image), pPacket->depth, pPacket->stencil, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdResolveImage:
        {
            struct_xglCmdResolveImage* pPacket = (struct_xglCmdResolveImage*)(packet->pBody);
            m_xglFuncs.real_xglCmdResolveImage(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), remap(pPacket->destImage), pPacket->rectCount, pPacket->pRects);
            break;
        }
        case GLV_TPI_XGL_xglCmdSetEvent:
        {
            struct_xglCmdSetEvent* pPacket = (struct_xglCmdSetEvent*)(packet->pBody);
            m_xglFuncs.real_xglCmdSetEvent(remap(pPacket->cmdBuffer), remap(pPacket->event), pPacket->pipeEvent);
            break;
        }
        case GLV_TPI_XGL_xglCmdResetEvent:
        {
            struct_xglCmdResetEvent* pPacket = (struct_xglCmdResetEvent*)(packet->pBody);
            m_xglFuncs.real_xglCmdResetEvent(remap(pPacket->cmdBuffer), remap(pPacket->event));
            break;
        }
        case GLV_TPI_XGL_xglCmdWaitEvents:
        {
            struct_xglCmdWaitEvents* pPacket = (struct_xglCmdWaitEvents*)(packet->pBody);
            XGL_EVENT saveEvent[100];
            uint32_t idx, numRemapBuf=0, numRemapImg=0;
            assert(pPacket->pWaitInfo && pPacket->pWaitInfo->eventCount <= 100);
            for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++)
            {
                XGL_EVENT *pEvent = (XGL_EVENT *) &(pPacket->pWaitInfo->pEvents[idx]);
                saveEvent[idx] = pPacket->pWaitInfo->pEvents[idx];
                *pEvent = remap(pPacket->pWaitInfo->pEvents[idx]);
            }

            XGL_BUFFER saveBuf[100];
            XGL_IMAGE saveImg[100];
            for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++)
            {
                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
                assert(pNext);
                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
                    assert(numRemapBuf < 100);
                    saveBuf[numRemapBuf++] = pNextBuf->buffer;
                    pNextBuf->buffer = remap(pNextBuf->buffer);
                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
                    assert(numRemapImg < 100);
                    saveImg[numRemapImg++] = pNextImg->image;
                    pNextImg->image = remap(pNextImg->image);
                }
            }
            m_xglFuncs.real_xglCmdWaitEvents(remap(pPacket->cmdBuffer), pPacket->pWaitInfo);
            for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++) {
                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
                    pNextBuf->buffer = saveBuf[idx];
                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
                    pNextImg->image = saveImg[idx];
                }
            }
            for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++) {
                XGL_EVENT *pEvent = (XGL_EVENT *) &(pPacket->pWaitInfo->pEvents[idx]);
                *pEvent = saveEvent[idx];
            }
            break;
        }
        case GLV_TPI_XGL_xglCmdPipelineBarrier:
        {
            struct_xglCmdPipelineBarrier* pPacket = (struct_xglCmdPipelineBarrier*)(packet->pBody);
            uint32_t idx, numRemapBuf=0, numRemapImg=0;
            XGL_BUFFER saveBuf[100];
            XGL_IMAGE saveImg[100];
            for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++)
            {
                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
                assert(pNext);
                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
                    assert(numRemapBuf < 100);
                    saveBuf[numRemapBuf++] = pNextBuf->buffer;
                    pNextBuf->buffer = remap(pNextBuf->buffer);
                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
                    assert(numRemapImg < 100);
                    saveImg[numRemapImg++] = pNextImg->image;
                    pNextImg->image = remap(pNextImg->image);
                }
            }
            m_xglFuncs.real_xglCmdPipelineBarrier(remap(pPacket->cmdBuffer), pPacket->pBarrier);
            for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++) {
                XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
                if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
                    XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
                    pNextBuf->buffer = saveBuf[idx];
                } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
                    XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
                    pNextImg->image = saveImg[idx];
                }
            }
            break;
        }
        case GLV_TPI_XGL_xglCmdBeginQuery:
        {
            struct_xglCmdBeginQuery* pPacket = (struct_xglCmdBeginQuery*)(packet->pBody);
            m_xglFuncs.real_xglCmdBeginQuery(remap(pPacket->cmdBuffer), remap(pPacket->queryPool), pPacket->slot, pPacket->flags);
            break;
        }
        case GLV_TPI_XGL_xglCmdEndQuery:
        {
            struct_xglCmdEndQuery* pPacket = (struct_xglCmdEndQuery*)(packet->pBody);
            m_xglFuncs.real_xglCmdEndQuery(remap(pPacket->cmdBuffer), remap(pPacket->queryPool), pPacket->slot);
            break;
        }
        case GLV_TPI_XGL_xglCmdResetQueryPool:
        {
            struct_xglCmdResetQueryPool* pPacket = (struct_xglCmdResetQueryPool*)(packet->pBody);
            m_xglFuncs.real_xglCmdResetQueryPool(remap(pPacket->cmdBuffer), remap(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdWriteTimestamp:
        {
            struct_xglCmdWriteTimestamp* pPacket = (struct_xglCmdWriteTimestamp*)(packet->pBody);
            m_xglFuncs.real_xglCmdWriteTimestamp(remap(pPacket->cmdBuffer), pPacket->timestampType, remap(pPacket->destBuffer), pPacket->destOffset);
            break;
        }
        case GLV_TPI_XGL_xglCmdInitAtomicCounters:
        {
            struct_xglCmdInitAtomicCounters* pPacket = (struct_xglCmdInitAtomicCounters*)(packet->pBody);
            m_xglFuncs.real_xglCmdInitAtomicCounters(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, pPacket->pData);
            break;
        }
        case GLV_TPI_XGL_xglCmdLoadAtomicCounters:
        {
            struct_xglCmdLoadAtomicCounters* pPacket = (struct_xglCmdLoadAtomicCounters*)(packet->pBody);
            m_xglFuncs.real_xglCmdLoadAtomicCounters(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, remap(pPacket->srcBuffer), pPacket->srcOffset);
            break;
        }
        case GLV_TPI_XGL_xglCmdSaveAtomicCounters:
        {
            struct_xglCmdSaveAtomicCounters* pPacket = (struct_xglCmdSaveAtomicCounters*)(packet->pBody);
            m_xglFuncs.real_xglCmdSaveAtomicCounters(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, remap(pPacket->destBuffer), pPacket->destOffset);
            break;
        }
        case GLV_TPI_XGL_xglCreateFramebuffer:
        {
            struct_xglCreateFramebuffer* pPacket = (struct_xglCreateFramebuffer*)(packet->pBody);
            XGL_FRAMEBUFFER_CREATE_INFO *pInfo = (XGL_FRAMEBUFFER_CREATE_INFO *) pPacket->pCreateInfo;
            XGL_COLOR_ATTACHMENT_BIND_INFO *pColorAttachments, *pSavedColor = (XGL_COLOR_ATTACHMENT_BIND_INFO*)pInfo->pColorAttachments;
            bool allocatedColorAttachments = false;
            if (pSavedColor != NULL)
            {
                allocatedColorAttachments = true;
                pColorAttachments = GLV_NEW_ARRAY(XGL_COLOR_ATTACHMENT_BIND_INFO, pInfo->colorAttachmentCount);
                memcpy(pColorAttachments, pSavedColor, sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO) * pInfo->colorAttachmentCount);
                for (uint32_t i = 0; i < pInfo->colorAttachmentCount; i++)
                {
                    pColorAttachments[i].view = remap(pInfo->pColorAttachments[i].view);
                }
                pInfo->pColorAttachments = pColorAttachments;
            }
            // remap depth stencil target
            const XGL_DEPTH_STENCIL_BIND_INFO *pSavedDS = pInfo->pDepthStencilAttachment;
            XGL_DEPTH_STENCIL_BIND_INFO depthTarget;
            if (pSavedDS != NULL)
            {
                memcpy(&depthTarget, pSavedDS, sizeof(XGL_DEPTH_STENCIL_BIND_INFO));
                depthTarget.view = remap(pSavedDS->view);
                pInfo->pDepthStencilAttachment = &depthTarget;
            }
            XGL_FRAMEBUFFER local_framebuffer;
            replayResult = m_xglFuncs.real_xglCreateFramebuffer(remap(pPacket->device), pPacket->pCreateInfo, &local_framebuffer);
            pInfo->pColorAttachments = pSavedColor;
            pInfo->pDepthStencilAttachment = pSavedDS;
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pFramebuffer, &local_framebuffer);
            }
            if (allocatedColorAttachments)
            {
                GLV_DELETE((void*)pColorAttachments);
            }
            CHECK_RETURN_VALUE(xglCreateFramebuffer);
            break;
        }
        case GLV_TPI_XGL_xglCreateRenderPass:
        {
            struct_xglCreateRenderPass* pPacket = (struct_xglCreateRenderPass*)(packet->pBody);
            XGL_RENDER_PASS_CREATE_INFO *pInfo = (XGL_RENDER_PASS_CREATE_INFO *) pPacket->pCreateInfo;
            // remap framebuffer
            XGL_FRAMEBUFFER savedFB = 0, *pFB = &(pInfo->framebuffer);
            if (*pFB != NULL)
            {
                savedFB = pInfo->framebuffer;
                *pFB = remap(pInfo->framebuffer);
            }
            XGL_RENDER_PASS local_renderpass;
            replayResult = m_xglFuncs.real_xglCreateRenderPass(remap(pPacket->device), pPacket->pCreateInfo, &local_renderpass);
            if (*pFB != NULL)
                pInfo->framebuffer = savedFB;
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pRenderPass, &local_renderpass);
            }
            CHECK_RETURN_VALUE(xglCreateRenderPass);
            break;
        }
        case GLV_TPI_XGL_xglCmdBeginRenderPass:
        {
            struct_xglCmdBeginRenderPass* pPacket = (struct_xglCmdBeginRenderPass*)(packet->pBody);
            m_xglFuncs.real_xglCmdBeginRenderPass(remap(pPacket->cmdBuffer), remap(pPacket->renderPass));
            break;
        }
        case GLV_TPI_XGL_xglCmdEndRenderPass:
        {
            struct_xglCmdEndRenderPass* pPacket = (struct_xglCmdEndRenderPass*)(packet->pBody);
            m_xglFuncs.real_xglCmdEndRenderPass(remap(pPacket->cmdBuffer), remap(pPacket->renderPass));
            break;
        }
        case GLV_TPI_XGL_xglDbgSetValidationLevel:
        {
            struct_xglDbgSetValidationLevel* pPacket = (struct_xglDbgSetValidationLevel*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetValidationLevel(remap(pPacket->device), pPacket->validationLevel);
            CHECK_RETURN_VALUE(xglDbgSetValidationLevel);
            break;
        }
        case GLV_TPI_XGL_xglDbgRegisterMsgCallback:
        {
            // Just eating these calls as no way to restore dbg func ptr.
            break;
        }
        case GLV_TPI_XGL_xglDbgUnregisterMsgCallback:
        {
            // Just eating these calls as no way to restore dbg func ptr.
            break;
        }
        case GLV_TPI_XGL_xglDbgSetMessageFilter:
        {
            struct_xglDbgSetMessageFilter* pPacket = (struct_xglDbgSetMessageFilter*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetMessageFilter(remap(pPacket->device), pPacket->msgCode, pPacket->filter);
            CHECK_RETURN_VALUE(xglDbgSetMessageFilter);
            break;
        }
        case GLV_TPI_XGL_xglDbgSetObjectTag:
        {
            struct_xglDbgSetObjectTag* pPacket = (struct_xglDbgSetObjectTag*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetObjectTag(remap(pPacket->object), pPacket->tagSize, pPacket->pTag);
            CHECK_RETURN_VALUE(xglDbgSetObjectTag);
            break;
        }
        case GLV_TPI_XGL_xglDbgSetGlobalOption:
        {
            struct_xglDbgSetGlobalOption* pPacket = (struct_xglDbgSetGlobalOption*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetGlobalOption(pPacket->dbgOption, pPacket->dataSize, pPacket->pData);
            CHECK_RETURN_VALUE(xglDbgSetGlobalOption);
            break;
        }
        case GLV_TPI_XGL_xglDbgSetDeviceOption:
        {
            struct_xglDbgSetDeviceOption* pPacket = (struct_xglDbgSetDeviceOption*)(packet->pBody);
            replayResult = m_xglFuncs.real_xglDbgSetDeviceOption(remap(pPacket->device), pPacket->dbgOption, pPacket->dataSize, pPacket->pData);
            CHECK_RETURN_VALUE(xglDbgSetDeviceOption);
            break;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerBegin:
        {
            struct_xglCmdDbgMarkerBegin* pPacket = (struct_xglCmdDbgMarkerBegin*)(packet->pBody);
            m_xglFuncs.real_xglCmdDbgMarkerBegin(remap(pPacket->cmdBuffer), pPacket->pMarker);
            break;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerEnd:
        {
            struct_xglCmdDbgMarkerEnd* pPacket = (struct_xglCmdDbgMarkerEnd*)(packet->pBody);
            m_xglFuncs.real_xglCmdDbgMarkerEnd(remap(pPacket->cmdBuffer));
            break;
        }
        case GLV_TPI_XGL_xglWsiX11AssociateConnection:
        {
            struct_xglWsiX11AssociateConnection* pPacket = (struct_xglWsiX11AssociateConnection*)(packet->pBody);
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
            //associate with the replayers Wsi connection rather than tracers
            replayResult = m_xglFuncs.real_xglWsiX11AssociateConnection(remap(pPacket->gpu), &(m_display->m_WsiConnection));
#elif defined(WIN32)
            //TBD
            replayResult = XGL_SUCCESS;
#endif
            CHECK_RETURN_VALUE(xglWsiX11AssociateConnection);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11GetMSC:
        {
            struct_xglWsiX11GetMSC* pPacket = (struct_xglWsiX11GetMSC*)(packet->pBody);
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
            xcb_window_t window = m_display->m_XcbWindow;
            replayResult = m_xglFuncs.real_xglWsiX11GetMSC(remap(pPacket->device), window, pPacket->crtc, pPacket->pMsc);
#elif defined(WIN32)
            //TBD
            replayResult = XGL_SUCCESS;
#else
#endif
            CHECK_RETURN_VALUE(xglWsiX11GetMSC);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11CreatePresentableImage:
        {
            struct_xglWsiX11CreatePresentableImage* pPacket = (struct_xglWsiX11CreatePresentableImage*)(packet->pBody);
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
            XGL_IMAGE img;
            XGL_GPU_MEMORY mem;
            m_display->imageHeight.push_back(pPacket->pCreateInfo->extent.height);
            m_display->imageWidth.push_back(pPacket->pCreateInfo->extent.width);
            replayResult = m_xglFuncs.real_xglWsiX11CreatePresentableImage(remap(pPacket->device), pPacket->pCreateInfo, &img, &mem);
            if (replayResult == XGL_SUCCESS)
            {
                if (pPacket->pImage != NULL)
                    add_to_map(pPacket->pImage, &img);
                if(pPacket->pMem != NULL)
                    add_to_map(pPacket->pMem, &mem);
                m_display->imageHandles.push_back(img);
                m_display->imageMemory.push_back(mem);
            }
#elif defined(WIN32)
            //TBD
            replayResult = XGL_SUCCESS;
#endif
            CHECK_RETURN_VALUE(xglWsiX11CreatePresentableImage);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11QueuePresent:
        {
            struct_xglWsiX11QueuePresent* pPacket = (struct_xglWsiX11QueuePresent*)(packet->pBody);
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
            XGL_WSI_X11_PRESENT_INFO pInfo;
            std::vector<int>::iterator it;
            memcpy(&pInfo, pPacket->pPresentInfo, sizeof(XGL_WSI_X11_PRESENT_INFO));
            pInfo.srcImage = remap(pPacket->pPresentInfo->srcImage);
            // use replayers Xcb window
            pInfo.destWindow = m_display->m_XcbWindow;
            replayResult = m_xglFuncs.real_xglWsiX11QueuePresent(remap(pPacket->queue), &pInfo, remap(pPacket->fence));
            it = std::find(m_screenshotFrames.begin(), m_screenshotFrames.end(), m_display->m_frameNumber);
            if (it != m_screenshotFrames.end())
            {
                for(unsigned int i=0; i<m_display->imageHandles.size(); i++)
                {
                    if (m_display->imageHandles[i] == pInfo.srcImage)
                    {
                        char frameName[32];
                        sprintf(frameName, "%d",m_display->m_frameNumber);
                        glvWritePPM(frameName, m_display->imageWidth[i], m_display->imageHeight[i],
                            m_display->imageHandles[i], m_display->imageMemory[i], &m_xglFuncs);
                        break;
                    }
                }
            }
#elif defined(WIN32)
            //TBD
            replayResult = XGL_SUCCESS;
#endif
            m_display->m_frameNumber++;
            CHECK_RETURN_VALUE(xglWsiX11QueuePresent);
            break;
        }
        default:
            glv_LogWarn("Unrecognized packet_id %u, skipping\n", packet->packet_id);
            returnValue = glv_replay::GLV_REPLAY_INVALID_ID;
            break;
    }
    return returnValue;
}
