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
#include "glvreplay_xgl_replay.h"


extern "C" {
#include "glvtrace_xgl_xgl_structs.h"
#include "glvtrace_xgl_xgldbg_structs.h"
#include "glvtrace_xgl_xglwsix11ext_structs.h"
#include "glvtrace_xgl_packet_id.h"
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
        for( XGL_UINT32 gpu = 0; gpu < m_gpuCount; gpu++)
        {
            XGL_SIZE gpuInfoSize = sizeof(m_gpuProps[0]);

            // get the GPU physical properties:
            res = xglGetGpuInfo( m_gpus[gpu], XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES, &gpuInfoSize, &m_gpuProps[gpu]);
            if (res != XGL_SUCCESS)
                glv_LogWarn("Failed to retrieve properties for gpu[%d] result %d\n", gpu, res);
        }
        res = XGL_SUCCESS;
    } else if ((gpu_idx + 1) > m_gpuCount) {
        glv_LogError("xglInitAndEnumerate number of gpus doesn't include requested index: num %d, requested %d\n", m_gpuCount, gpu_idx);
        return -1;
    } else {
        glv_LogError("xglInitAndEnumerate failed");
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
            m_extensions.push_back((XGL_CHAR *) extensions[ext]);
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
    const XGL_CHAR * const * extNames = &m_extensions[0];
    XGL_DEVICE_CREATE_INFO info = {};
    info.queueRecordCount = 1;
    info.pRequestedQueues = &dqci;
    info.extensionCount = static_cast <XGL_UINT> (m_extensions.size());
    info.ppEnabledExtensionNames = extNames;
    info.flags = XGL_DEVICE_CREATE_VALIDATION;
    info.maxValidationLevel = XGL_VALIDATION_LEVEL_4;
    XGL_BOOL xglTrue = XGL_TRUE;
    res = xglDbgSetGlobalOption( XGL_DBG_OPTION_BREAK_ON_ERROR, sizeof( xglTrue ), &xglTrue );
    if (res != XGL_SUCCESS)
        glv_LogWarn("Couldn't set debug option break on error\n");
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
        glv_LogError("couldn't init xgl library");
        return -1;
    } else {
        m_initedXGL = true;
    }
#endif
#if defined(PLATFORM_LINUX)

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
#if defined(WIN32)
    m_windowHandle = NULL;
#elif defined(PLATFORM_LINUX)
    m_WsiConnection.pConnection = NULL;
    m_WsiConnection.root = 0;
    m_WsiConnection.provider = 0;
    m_pXcbScreen = NULL;
    m_XcbWindow = 0;
#endif
}

xglDisplay::~xglDisplay()
{
#ifdef PLATFORM_LINUX
    xcb_destroy_window(m_WsiConnection.pConnection, m_XcbWindow);
    xcb_disconnect(m_WsiConnection.pConnection);
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
#if defined(WIN32)
    m_windowHandle = hWindow;
#endif
    m_windowWidth = width;
    m_windowHeight = height;
    return 0;
}

int xglDisplay::create_window(const unsigned int width, const unsigned int height)
{
#if defined(WIN32)
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
#elif defined(PLATFORM_LINUX)

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
#endif
}

void xglDisplay::resize_window(const unsigned int width, const unsigned int height)
{
#if defined(WIN32)
    if (width != m_windowWidth || height != m_windowHeight)
    {
        SetWindowPos(get_window_handle(), HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
        m_windowWidth = width;
        m_windowHeight = height;
    }
#elif defined(PLATFORM_LINUX)
    if (width != m_windowWidth || height != m_windowHeight)
    {
        uint32_t values[2];
        values[0] = width;
        values[1] = height;
        xcb_configure_window(m_WsiConnection.pConnection, m_XcbWindow, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        m_windowWidth = width;
        m_windowHeight = height;
    }
#endif
}

void xglDisplay::process_event()
{
}

xglReplay::xglReplay(unsigned int debugLevel)
{
    m_display = new xglDisplay();
    m_debugLevel = debugLevel;
}

xglReplay::~xglReplay()
{
    delete m_display;
    glv_platform_close_library(m_xglFuncs.m_libHandle);
}

int xglReplay::init(glv_replay::Display & disp)
{
    int err;
#if defined _WIN64
    HMODULE handle = LoadLibrary("xgl64.dll" );
#elif defined _WIN32
    HMODULE handle = LoadLibrary("xgl32.dll" );
#elif defined PLATFORM_LINUX
    void * handle = dlopen("libXGL.so", RTLD_LAZY);
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

void xglReplay::copy_mem_remap_range_struct(XGL_VIRTUAL_MEMORY_REMAP_RANGE *outRange, const XGL_VIRTUAL_MEMORY_REMAP_RANGE *inRange)
{
    outRange->pageCount = inRange->pageCount;
    outRange->realStartPage = inRange->realStartPage;
    outRange->realMem = remap(inRange->realMem);
    outRange->virtualStartPage = inRange->virtualStartPage;
    outRange->virtualMem = remap(inRange->virtualMem);
}

glv_replay::GLV_REPLAY_RESULT xglReplay::handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn)
{
    glv_replay::GLV_REPLAY_RESULT res = resIn;
    if (resCall != resTrace) {
        glv_LogWarn("Mismatched return from API call (%s) traced result %s, replay result %s\n", entrypointName,
                string_XGL_RESULT(resTrace), string_XGL_RESULT(resCall));
        res = glv_replay::GLV_REPLAY_BAD_RETURN;
    }

#if 0
    if (resCall != XGL_SUCCESS) {
        glv_LogWarn("API call (%s) returned failed result %s\n", entrypointName, string_XGL_RESULT(resCall));
    }
#endif
    return res;
}

void xglFuncs::init_funcs(void * handle)
{
    m_libHandle = handle;
    real_xglInitAndEnumerateGpus = (type_xglInitAndEnumerateGpus)(glv_platform_get_library_entrypoint(handle, "xglInitAndEnumerateGpus"));
    real_xglGetGpuInfo = (type_xglGetGpuInfo)(glv_platform_get_library_entrypoint(handle, "xglGetGpuInfo"));
    real_xglCreateDevice = (type_xglCreateDevice)(glv_platform_get_library_entrypoint(handle, "xglCreateDevice"));
    real_xglDestroyDevice = (type_xglDestroyDevice)(glv_platform_get_library_entrypoint(handle, "xglDestroyDevice"));
    real_xglGetExtensionSupport = (type_xglGetExtensionSupport)(glv_platform_get_library_entrypoint(handle, "xglGetExtensionSupport"));
    real_xglGetDeviceQueue = (type_xglGetDeviceQueue)(glv_platform_get_library_entrypoint(handle, "xglGetDeviceQueue"));
    real_xglQueueSubmit = (type_xglQueueSubmit)(glv_platform_get_library_entrypoint(handle, "xglQueueSubmit"));
    real_xglQueueSetGlobalMemReferences = (type_xglQueueSetGlobalMemReferences)(glv_platform_get_library_entrypoint(handle, "xglQueueSetGlobalMemReferences"));
    real_xglQueueWaitIdle = (type_xglQueueWaitIdle)(glv_platform_get_library_entrypoint(handle, "xglQueueWaitIdle"));
    real_xglDeviceWaitIdle = (type_xglDeviceWaitIdle)(glv_platform_get_library_entrypoint(handle, "xglDeviceWaitIdle"));
    real_xglGetMemoryHeapCount = (type_xglGetMemoryHeapCount)(glv_platform_get_library_entrypoint(handle, "xglGetMemoryHeapCount"));
    real_xglGetMemoryHeapInfo = (type_xglGetMemoryHeapInfo)(glv_platform_get_library_entrypoint(handle, "xglGetMemoryHeapInfo"));
    real_xglAllocMemory = (type_xglAllocMemory)(glv_platform_get_library_entrypoint(handle, "xglAllocMemory"));
    real_xglFreeMemory = (type_xglFreeMemory)(glv_platform_get_library_entrypoint(handle, "xglFreeMemory"));
    real_xglSetMemoryPriority = (type_xglSetMemoryPriority)(glv_platform_get_library_entrypoint(handle, "xglSetMemoryPriority"));
    real_xglMapMemory = (type_xglMapMemory)(glv_platform_get_library_entrypoint(handle, "xglMapMemory"));
    real_xglUnmapMemory = (type_xglUnmapMemory)(glv_platform_get_library_entrypoint(handle, "xglUnmapMemory"));
    real_xglPinSystemMemory = (type_xglPinSystemMemory)(glv_platform_get_library_entrypoint(handle, "xglPinSystemMemory"));
    real_xglRemapVirtualMemoryPages = (type_xglRemapVirtualMemoryPages)(glv_platform_get_library_entrypoint(handle, "xglRemapVirtualMemoryPages"));
    real_xglGetMultiGpuCompatibility = (type_xglGetMultiGpuCompatibility)(glv_platform_get_library_entrypoint(handle, "xglGetMultiGpuCompatibility"));
    real_xglOpenSharedMemory = (type_xglOpenSharedMemory)(glv_platform_get_library_entrypoint(handle, "xglOpenSharedMemory"));
    real_xglOpenSharedQueueSemaphore = (type_xglOpenSharedQueueSemaphore)(glv_platform_get_library_entrypoint(handle, "xglOpenSharedQueueSemaphore"));
    real_xglOpenPeerMemory = (type_xglOpenPeerMemory)(glv_platform_get_library_entrypoint(handle, "xglOpenPeerMemory"));
    real_xglOpenPeerImage = (type_xglOpenPeerImage)(glv_platform_get_library_entrypoint(handle, "xglOpenPeerImage"));
    real_xglDestroyObject = (type_xglDestroyObject)(glv_platform_get_library_entrypoint(handle, "xglDestroyObject"));
    real_xglGetObjectInfo = (type_xglGetObjectInfo)(glv_platform_get_library_entrypoint(handle, "xglGetObjectInfo"));
    real_xglBindObjectMemory = (type_xglBindObjectMemory)(glv_platform_get_library_entrypoint(handle, "xglBindObjectMemory"));
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
    real_xglCreateImage = (type_xglCreateImage)(glv_platform_get_library_entrypoint(handle, "xglCreateImage"));
    real_xglGetImageSubresourceInfo = (type_xglGetImageSubresourceInfo)(glv_platform_get_library_entrypoint(handle, "xglGetImageSubresourceInfo"));
    real_xglCreateImageView = (type_xglCreateImageView)(glv_platform_get_library_entrypoint(handle, "xglCreateImageView"));
    real_xglCreateColorAttachmentView = (type_xglCreateColorAttachmentView)(glv_platform_get_library_entrypoint(handle, "xglCreateColorAttachmentView"));
    real_xglCreateDepthStencilView = (type_xglCreateDepthStencilView)(glv_platform_get_library_entrypoint(handle, "xglCreateDepthStencilView"));
    real_xglCreateShader = (type_xglCreateShader)(glv_platform_get_library_entrypoint(handle, "xglCreateShader"));
    real_xglCreateGraphicsPipeline = (type_xglCreateGraphicsPipeline)(glv_platform_get_library_entrypoint(handle, "xglCreateGraphicsPipeline"));
    real_xglCreateComputePipeline = (type_xglCreateComputePipeline)(glv_platform_get_library_entrypoint(handle, "xglCreateComputePipeline"));
    real_xglStorePipeline = (type_xglStorePipeline)(glv_platform_get_library_entrypoint(handle, "xglStorePipeline"));
    real_xglCreatePipelineDelta = (type_xglCreatePipelineDelta)(glv_platform_get_library_entrypoint(handle, "xglCreatePipelineDelta"));
    real_xglLoadPipeline = (type_xglLoadPipeline)(glv_platform_get_library_entrypoint(handle, "xglLoadPipeline"));
    real_xglCreateSampler = (type_xglCreateSampler)(glv_platform_get_library_entrypoint(handle, "xglCreateSampler"));
    real_xglCreateDescriptorSet = (type_xglCreateDescriptorSet)(glv_platform_get_library_entrypoint(handle, "xglCreateDescriptorSet"));
    real_xglBeginDescriptorSetUpdate = (type_xglBeginDescriptorSetUpdate)(glv_platform_get_library_entrypoint(handle, "xglBeginDescriptorSetUpdate"));
    real_xglEndDescriptorSetUpdate = (type_xglEndDescriptorSetUpdate)(glv_platform_get_library_entrypoint(handle, "xglEndDescriptorSetUpdate"));
    real_xglAttachSamplerDescriptors = (type_xglAttachSamplerDescriptors)(glv_platform_get_library_entrypoint(handle, "xglAttachSamplerDescriptors"));
    real_xglAttachImageViewDescriptors = (type_xglAttachImageViewDescriptors)(glv_platform_get_library_entrypoint(handle, "xglAttachImageViewDescriptors"));
    real_xglAttachMemoryViewDescriptors = (type_xglAttachMemoryViewDescriptors)(glv_platform_get_library_entrypoint(handle, "xglAttachMemoryViewDescriptors"));
    real_xglAttachNestedDescriptors = (type_xglAttachNestedDescriptors)(glv_platform_get_library_entrypoint(handle, "xglAttachNestedDescriptors"));
    real_xglClearDescriptorSetSlots = (type_xglClearDescriptorSetSlots)(glv_platform_get_library_entrypoint(handle, "xglClearDescriptorSetSlots"));
    real_xglCreateViewportState = (type_xglCreateViewportState)(glv_platform_get_library_entrypoint(handle, "xglCreateViewportState"));
    real_xglCreateRasterState = (type_xglCreateRasterState)(glv_platform_get_library_entrypoint(handle, "xglCreateRasterState"));
    real_xglCreateMsaaState = (type_xglCreateMsaaState)(glv_platform_get_library_entrypoint(handle, "xglCreateMsaaState"));
    real_xglCreateColorBlendState = (type_xglCreateColorBlendState)(glv_platform_get_library_entrypoint(handle, "xglCreateColorBlendState"));
    real_xglCreateDepthStencilState = (type_xglCreateDepthStencilState)(glv_platform_get_library_entrypoint(handle, "xglCreateDepthStencilState"));
    real_xglCreateCommandBuffer = (type_xglCreateCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglCreateCommandBuffer"));
    real_xglBeginCommandBuffer = (type_xglBeginCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglBeginCommandBuffer"));
    real_xglEndCommandBuffer = (type_xglEndCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglEndCommandBuffer"));
    real_xglResetCommandBuffer = (type_xglResetCommandBuffer)(glv_platform_get_library_entrypoint(handle, "xglResetCommandBuffer"));
    real_xglCmdBindPipeline = (type_xglCmdBindPipeline)(glv_platform_get_library_entrypoint(handle, "xglCmdBindPipeline"));
    real_xglCmdBindPipelineDelta = (type_xglCmdBindPipelineDelta)(glv_platform_get_library_entrypoint(handle, "xglCmdBindPipelineDelta"));
    real_xglCmdBindStateObject = (type_xglCmdBindStateObject)(glv_platform_get_library_entrypoint(handle, "xglCmdBindStateObject"));
    real_xglCmdBindDescriptorSet = (type_xglCmdBindDescriptorSet)(glv_platform_get_library_entrypoint(handle, "xglCmdBindDescriptorSet"));
    real_xglCmdBindDynamicMemoryView = (type_xglCmdBindDynamicMemoryView)(glv_platform_get_library_entrypoint(handle, "xglCmdBindDynamicMemoryView"));
    real_xglCmdBindIndexData = (type_xglCmdBindIndexData)(glv_platform_get_library_entrypoint(handle, "xglCmdBindIndexData"));
    real_xglCmdBindAttachments = (type_xglCmdBindAttachments)(glv_platform_get_library_entrypoint(handle, "xglCmdBindAttachments"));
    real_xglCmdPrepareMemoryRegions = (type_xglCmdPrepareMemoryRegions)(glv_platform_get_library_entrypoint(handle, "xglCmdPrepareMemoryRegions"));
    real_xglCmdPrepareImages = (type_xglCmdPrepareImages)(glv_platform_get_library_entrypoint(handle, "xglCmdPrepareImages"));
    real_xglCmdDraw = (type_xglCmdDraw)(glv_platform_get_library_entrypoint(handle, "xglCmdDraw"));
    real_xglCmdDrawIndexed = (type_xglCmdDrawIndexed)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndexed"));
    real_xglCmdDrawIndirect = (type_xglCmdDrawIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndirect"));
    real_xglCmdDrawIndexedIndirect = (type_xglCmdDrawIndexedIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDrawIndexedIndirect"));
    real_xglCmdDispatch = (type_xglCmdDispatch)(glv_platform_get_library_entrypoint(handle, "xglCmdDispatch"));
    real_xglCmdDispatchIndirect = (type_xglCmdDispatchIndirect)(glv_platform_get_library_entrypoint(handle, "xglCmdDispatchIndirect"));
    real_xglCmdCopyMemory = (type_xglCmdCopyMemory)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyMemory"));
    real_xglCmdCopyImage = (type_xglCmdCopyImage)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyImage"));
    real_xglCmdCopyMemoryToImage = (type_xglCmdCopyMemoryToImage)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyMemoryToImage"));
    real_xglCmdCopyImageToMemory = (type_xglCmdCopyImageToMemory)(glv_platform_get_library_entrypoint(handle, "xglCmdCopyImageToMemory"));
    real_xglCmdCloneImageData = (type_xglCmdCloneImageData)(glv_platform_get_library_entrypoint(handle, "xglCmdCloneImageData"));
    real_xglCmdUpdateMemory = (type_xglCmdUpdateMemory)(glv_platform_get_library_entrypoint(handle, "xglCmdUpdateMemory"));
    real_xglCmdFillMemory = (type_xglCmdFillMemory)(glv_platform_get_library_entrypoint(handle, "xglCmdFillMemory"));
    real_xglCmdClearColorImage = (type_xglCmdClearColorImage)(glv_platform_get_library_entrypoint(handle, "xglCmdClearColorImage"));
    real_xglCmdClearColorImageRaw = (type_xglCmdClearColorImageRaw)(glv_platform_get_library_entrypoint(handle, "xglCmdClearColorImageRaw"));
    real_xglCmdClearDepthStencil = (type_xglCmdClearDepthStencil)(glv_platform_get_library_entrypoint(handle, "xglCmdClearDepthStencil"));
    real_xglCmdResolveImage = (type_xglCmdResolveImage)(glv_platform_get_library_entrypoint(handle, "xglCmdResolveImage"));
    real_xglCmdSetEvent = (type_xglCmdSetEvent)(glv_platform_get_library_entrypoint(handle, "xglCmdSetEvent"));
    real_xglCmdResetEvent = (type_xglCmdResetEvent)(glv_platform_get_library_entrypoint(handle, "xglCmdResetEvent"));
    real_xglCmdMemoryAtomic = (type_xglCmdMemoryAtomic)(glv_platform_get_library_entrypoint(handle, "xglCmdMemoryAtomic"));
    real_xglCmdBeginQuery = (type_xglCmdBeginQuery)(glv_platform_get_library_entrypoint(handle, "xglCmdBeginQuery"));
    real_xglCmdEndQuery = (type_xglCmdEndQuery)(glv_platform_get_library_entrypoint(handle, "xglCmdEndQuery"));
    real_xglCmdResetQueryPool = (type_xglCmdResetQueryPool)(glv_platform_get_library_entrypoint(handle, "xglCmdResetQueryPool"));
    real_xglCmdWriteTimestamp = (type_xglCmdWriteTimestamp)(glv_platform_get_library_entrypoint(handle, "xglCmdWriteTimestamp"));
    real_xglCmdInitAtomicCounters = (type_xglCmdInitAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdInitAtomicCounters"));
    real_xglCmdLoadAtomicCounters = (type_xglCmdLoadAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdLoadAtomicCounters"));
    real_xglCmdSaveAtomicCounters = (type_xglCmdSaveAtomicCounters)(glv_platform_get_library_entrypoint(handle, "xglCmdSaveAtomicCounters"));

    // debug entrypoints
    real_xglDbgSetValidationLevel = (type_xglDbgSetValidationLevel)(glv_platform_get_library_entrypoint(handle, "xglDbgSetValidationLevel"));
    real_xglDbgRegisterMsgCallback = (type_xglDbgRegisterMsgCallback)(glv_platform_get_library_entrypoint(handle, "xglDbgRegisterMsgCallback"));
    real_xglDbgUnregisterMsgCallback = (type_xglDbgUnregisterMsgCallback)(glv_platform_get_library_entrypoint(handle, "xglDbgUnregisterMsgCallback"));
    real_xglDbgSetMessageFilter = (type_xglDbgSetMessageFilter)(glv_platform_get_library_entrypoint(handle, "xglDbgSetMessageFilter"));
    real_xglDbgSetObjectTag = (type_xglDbgSetObjectTag)(glv_platform_get_library_entrypoint(handle, "xglDbgSetObjectTag"));
    real_xglDbgSetGlobalOption = (type_xglDbgSetGlobalOption)(glv_platform_get_library_entrypoint(handle, "xglDbgSetGlobalOption"));
    real_xglDbgSetDeviceOption = (type_xglDbgSetDeviceOption)(glv_platform_get_library_entrypoint(handle, "xglDbgSetDeviceOption"));
    real_xglCmdDbgMarkerBegin = (type_xglCmdDbgMarkerBegin)(glv_platform_get_library_entrypoint(handle, "xglCmdDbgMarkerBegin"));
    real_xglCmdDbgMarkerEnd = (type_xglCmdDbgMarkerEnd)(glv_platform_get_library_entrypoint(handle, "xglCmdDbgMarkerEnd"));

    // WsiX11Ext entrypoints
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
        case GLV_TPI_XGL_xglInitAndEnumerateGpus:
        {
            struct_xglInitAndEnumerateGpus* pPacket;
            pPacket = interpret_body_as_xglInitAndEnumerateGpus(packet);

            // skip this call if XGL already inited
            if (!m_display->m_initedXGL)
            {
                XGL_UINT gpuCount;
                XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
                XGL_UINT maxGpus = (pPacket->maxGpus < XGL_MAX_PHYSICAL_GPUS) ? pPacket->maxGpus : XGL_MAX_PHYSICAL_GPUS;
                replayResult = m_xglFuncs.real_xglInitAndEnumerateGpus(pPacket->pAppInfo, pPacket->pAllocCb, maxGpus, &gpuCount, &gpus[0]);
                CHECK_RETURN_VALUE(xglInitAndEnumerateGpus);
                //TODO handle different number of gpus in trace versus replay
                if (gpuCount != *(pPacket->pGpuCount))
                {
                    glv_LogWarn("number of gpus mismatched in replay %u versus trace %u\n", gpuCount, *(pPacket->pGpuCount));
                }
                else if (gpuCount == 0)
                {
                     glv_LogError("xglInitAndEnumerateGpus number of gpus is zero\n");
                }
                else
                {
                    glv_LogInfo("Enumerated %d GPUs in the system\n", gpuCount);
                }
                clear_all_map_handles();
                // TODO handle enumeration results in a different order from trace to replay
                for (XGL_UINT i = 0; i < gpuCount; i++)
                {
                    if (pPacket->pGpus)
                        add_to_map(&(pPacket->pGpus[i]), &(gpus[i]));
                }
            }
            //TODO check gpu handles for consistency with trace  gpus using xglGetGPUInfo
            break;
        }
        case GLV_TPI_XGL_xglGetGpuInfo:
        {
            struct_xglGetGpuInfo * pPacket;
            pPacket = interpret_body_as_xglGetGpuInfo(packet);

            // skip this call if xgl inited
            if (!m_display->m_initedXGL)
            {
                switch (pPacket->infoType) {
                case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
                {
                    XGL_PHYSICAL_GPU_PROPERTIES gpuProps;
                    XGL_SIZE dataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
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
                    XGL_SIZE dataSize = sizeof(XGL_PHYSICAL_GPU_PERFORMANCE);
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
                    XGL_SIZE dataSize = sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);
                    XGL_SIZE numQueues = 1;
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
                    XGL_SIZE size = 0;
                    void* pData = NULL;
                    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
                    {
                        size = *pPacket->pDataSize;
                        pData = glv_malloc(*pPacket->pDataSize);
                    }
                    replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &size, pData);
                    if (replayResult == XGL_SUCCESS)
                    {
                        if (size != *pPacket->pDataSize && pData == NULL)
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
        case GLV_TPI_XGL_xglCreateDevice:
        {
            struct_xglCreateDevice * pPacket;
            pPacket = interpret_body_as_xglCreateDevice(packet);
            if (!m_display->m_initedXGL)
            {
                XGL_DEVICE device;
                XGL_DEVICE_CREATE_INFO cInfo;

                if (m_debugLevel > 0)
                {
                    memcpy(&cInfo, pPacket->pCreateInfo, sizeof(XGL_DEVICE_CREATE_INFO));
                    cInfo.flags = pPacket->pCreateInfo->flags | XGL_DEVICE_CREATE_VALIDATION_BIT;
                    cInfo.maxValidationLevel = (XGL_VALIDATION_LEVEL)((m_debugLevel <= 4) ? XGL_VALIDATION_LEVEL_0 + m_debugLevel : XGL_VALIDATION_LEVEL_0);
                    pPacket->pCreateInfo = &cInfo;
                }
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
            struct_xglDestroyDevice * pPacket;
            pPacket = interpret_body_as_xglDestroyDevice(packet);
            replayResult = m_xglFuncs.real_xglDestroyDevice(remap(pPacket->device));
            if (replayResult == XGL_SUCCESS)
            {
                rm_from_map(pPacket->device);
                m_display->m_initedXGL = false;
            }
            CHECK_RETURN_VALUE(xglDestroyDevice);
            break;
        }
        case GLV_TPI_XGL_xglGetExtensionSupport:
        {
            struct_xglGetExtensionSupport * pPacket;
            pPacket = interpret_body_as_xglGetExtensionSupport(packet);
            if (!m_display->m_initedXGL) {
                replayResult = m_xglFuncs.real_xglGetExtensionSupport(remap(pPacket->gpu), pPacket->pExtName);
                CHECK_RETURN_VALUE(xglGetExtensionSupport);
                if (replayResult == XGL_SUCCESS) {
                    for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)
                    {
                        if (!strncmp((const char *)g_extensions[ext], (const char *)pPacket->pExtName, strlen(g_extensions[ext]))) {
                            bool extInList = false;
                            for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {
                                if (!strncmp((const char *)m_display->m_extensions[j], (const char *)g_extensions[ext], strlen(g_extensions[ext])))
                                    extInList = true;
                                break;
                            }
                            if (!extInList)
                                m_display->m_extensions.push_back((XGL_CHAR *) g_extensions[ext]);
                            break;
                        }
                    }
                }
            }
            break;
        }
        case GLV_TPI_XGL_xglGetDeviceQueue:
        {
            struct_xglGetDeviceQueue *pPacket;
            pPacket = interpret_body_as_xglGetDeviceQueue(packet);
            XGL_QUEUE q;
            replayResult = m_xglFuncs.real_xglGetDeviceQueue(remap(pPacket->device), pPacket->queueType, pPacket->queueIndex, &q);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pQueue, &q);
            }
            CHECK_RETURN_VALUE(xglGetDeviceQueue);
            break;
        }
        case GLV_TPI_XGL_xglQueueSubmit:
        {
            struct_xglQueueSubmit *pPacket;
            pPacket = interpret_body_as_xglQueueSubmit(packet);
            XGL_CMD_BUFFER *remappedBuffers = NULL;
            if (pPacket->pCmdBuffers != NULL)
            {
                remappedBuffers = GLV_NEW_ARRAY( XGL_CMD_BUFFER, pPacket->cmdBufferCount);
                for (XGL_UINT i = 0; i < pPacket->cmdBufferCount; i++)
                {
                    *(remappedBuffers + i) = remap(*(pPacket->pCmdBuffers + i));
                }
            }

            XGL_MEMORY_REF* memRefs = NULL;
            if (pPacket->pMemRefs != NULL)
            {
                memRefs = GLV_NEW_ARRAY(XGL_MEMORY_REF, pPacket->memRefCount);
                memcpy(memRefs, pPacket->pMemRefs, sizeof(XGL_MEMORY_REF) * pPacket->memRefCount);
                for (XGL_UINT i = 0; i < pPacket->memRefCount; i++)
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
            struct_xglQueueSetGlobalMemReferences *pPacket;
            pPacket = interpret_body_as_xglQueueSetGlobalMemReferences(packet);
            replayResult = m_xglFuncs.real_xglQueueSetGlobalMemReferences(remap(pPacket->queue), pPacket->memRefCount, pPacket->pMemRefs);
            CHECK_RETURN_VALUE(xglQueueSetGlobalMemReferences);
            break;
        }
        case GLV_TPI_XGL_xglQueueWaitIdle:
        {
            struct_xglQueueWaitIdle *pPacket;
            pPacket = interpret_body_as_xglQueueWaitIdle(packet);
            replayResult = m_xglFuncs.real_xglQueueWaitIdle(remap(pPacket->queue));
            CHECK_RETURN_VALUE(xglQueueWaitIdle);
            break;
        }
        case GLV_TPI_XGL_xglDeviceWaitIdle:
        {
            struct_xglDeviceWaitIdle *pPacket;
            pPacket = interpret_body_as_xglDeviceWaitIdle(packet);
            replayResult = m_xglFuncs.real_xglDeviceWaitIdle(remap(pPacket->device));
            CHECK_RETURN_VALUE(xglDeviceWaitIdle);
            break;
        }
        case GLV_TPI_XGL_xglGetMemoryHeapCount:
        {
            struct_xglGetMemoryHeapCount *pPacket;
            XGL_UINT count;
            pPacket = interpret_body_as_xglGetMemoryHeapCount(packet);
            replayResult = m_xglFuncs.real_xglGetMemoryHeapCount(remap(pPacket->device), &count);
            if (count < 1 || count >= XGL_MAX_MEMORY_HEAPS)
                glv_LogError("xglGetMemoryHeapCount returned bad value count = %u\n", count);
            CHECK_RETURN_VALUE(xglGetMemoryHeapCount);
            break;
        }
        case GLV_TPI_XGL_xglGetMemoryHeapInfo:
        {
            // TODO handle case where traced heap count, ids and properties do not match replay heaps
            struct_xglGetMemoryHeapInfo *pPacket;
            XGL_SIZE dataSize = sizeof(XGL_MEMORY_HEAP_PROPERTIES);
            pPacket = interpret_body_as_xglGetMemoryHeapInfo(packet);
            // TODO check returned properties match queried properties if this makes sense
            if (pPacket->heapId >= XGL_MAX_MEMORY_HEAPS)
            {
                glv_LogError("xglGetMemoryHeapInfo bad heapid (%d) skipping packet\n");
                break;
            }
            replayResult = m_xglFuncs.real_xglGetMemoryHeapInfo(remap(pPacket->device), pPacket->heapId, pPacket->infoType, &dataSize,
                                               static_cast <XGL_VOID *> (&(m_heapProps[pPacket->heapId])));
            if (dataSize != sizeof(XGL_MEMORY_HEAP_PROPERTIES))
                glv_LogError("xglGetMemoryHeapInfo returned bad size = %u\n", dataSize);
            CHECK_RETURN_VALUE(xglGetMemoryHeapCount);
            break;
        }
        case GLV_TPI_XGL_xglAllocMemory:
        {
            struct_xglAllocMemory *pPacket;
            XGL_GPU_MEMORY handle;
            pPacket = interpret_body_as_xglAllocMemory(packet);
            replayResult = m_xglFuncs.real_xglAllocMemory(remap(pPacket->device), pPacket->pAllocInfo, &handle);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pMem, &handle);
                add_entry_to_mapData(handle, pPacket->pAllocInfo->allocationSize);
            }
            CHECK_RETURN_VALUE(xglAllocMemory);
            break;
        }
        case GLV_TPI_XGL_xglFreeMemory:
        {
            struct_xglFreeMemory *pPacket;
            pPacket = interpret_body_as_xglFreeMemory(packet);
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
            struct_xglSetMemoryPriority *pPacket;
            pPacket = interpret_body_as_xglSetMemoryPriority(packet);
            replayResult = m_xglFuncs.real_xglSetMemoryPriority(remap(pPacket->mem), pPacket->priority);
            CHECK_RETURN_VALUE(xglSetMemoryPriority);
            break;
        }
        case GLV_TPI_XGL_xglMapMemory:
        {
            struct_xglMapMemory *pPacket;
            pPacket = interpret_body_as_xglMapMemory(packet);
            XGL_GPU_MEMORY handle = remap(pPacket->mem);
            XGL_VOID* pData;
            replayResult = m_xglFuncs.real_xglMapMemory(handle, pPacket->flags, &pData);
            if (replayResult == XGL_SUCCESS)
               add_mapping_to_mapData(handle, pData);
            CHECK_RETURN_VALUE(xglMapMemory);
            break;
        }
        case GLV_TPI_XGL_xglUnmapMemory:
        {
            struct_xglUnmapMemory *pPacket;
            pPacket = interpret_body_as_xglUnmapMemory(packet);
            XGL_GPU_MEMORY handle = remap(pPacket->mem);
            rm_mapping_from_mapData(handle, pPacket->pData);  // copies data from packet into memory buffer
            replayResult = m_xglFuncs.real_xglUnmapMemory(handle);
            CHECK_RETURN_VALUE(xglUnmapMemory);
            break;
        }
        case GLV_TPI_XGL_xglPinSystemMemory:
        {
            struct_xglPinSystemMemory *pPacket;
            pPacket = interpret_body_as_xglPinSystemMemory(packet);
            XGL_GPU_MEMORY memHandle;
            replayResult = m_xglFuncs.real_xglPinSystemMemory(remap(pPacket->device), pPacket->pSysMem, pPacket->memSize, &memHandle);
            if (replayResult == XGL_SUCCESS)
                add_to_map(pPacket->pMem, &memHandle);
            CHECK_RETURN_VALUE(xglPinSystemMemory);
            break;
        }
        case GLV_TPI_XGL_xglRemapVirtualMemoryPages:
        {
            struct_xglRemapVirtualMemoryPages *pPacket;
            pPacket = interpret_body_as_xglRemapVirtualMemoryPages(packet);
            XGL_VIRTUAL_MEMORY_REMAP_RANGE *pRemappedRanges = GLV_NEW_ARRAY( XGL_VIRTUAL_MEMORY_REMAP_RANGE, pPacket->rangeCount);
            for (XGL_UINT i = 0; i < pPacket->rangeCount; i++)
            {
                copy_mem_remap_range_struct(pRemappedRanges + i, (pPacket->pRanges + i));
            }
            XGL_QUEUE_SEMAPHORE *pRemappedPreSema = GLV_NEW_ARRAY(XGL_QUEUE_SEMAPHORE, pPacket->preWaitSemaphoreCount);
            for (XGL_UINT i = 0; i < pPacket->preWaitSemaphoreCount; i++)
            {
                *(pRemappedPreSema + i) = *(pPacket->pPreWaitSemaphores + i);
            }
            XGL_QUEUE_SEMAPHORE *pRemappedPostSema = GLV_NEW_ARRAY(XGL_QUEUE_SEMAPHORE, pPacket->postSignalSemaphoreCount);
            for (XGL_UINT i = 0; i < pPacket->postSignalSemaphoreCount; i++)
            {
                *(pRemappedPostSema + i) = *(pPacket->pPostSignalSemaphores + i);
            }
            replayResult = m_xglFuncs.real_xglRemapVirtualMemoryPages(remap(pPacket->device), pPacket->rangeCount, pRemappedRanges, pPacket->preWaitSemaphoreCount,
                                                     pPacket->pPreWaitSemaphores, pPacket->postSignalSemaphoreCount, pPacket->pPostSignalSemaphores);
            GLV_DELETE(pRemappedRanges);
            GLV_DELETE(pRemappedPreSema);
            GLV_DELETE(pRemappedPostSema);
            CHECK_RETURN_VALUE(xglRemapVirtualMemoryPages);
            break;
        }
        case GLV_TPI_XGL_xglGetMultiGpuCompatibility:
        {
            struct_xglGetMultiGpuCompatibility *pPacket;
            XGL_GPU_COMPATIBILITY_INFO cInfo;
            XGL_PHYSICAL_GPU handle0, handle1;
            pPacket = interpret_body_as_xglGetMultiGpuCompatibility(packet);
            handle0 = remap(pPacket->gpu0);
            handle1 = remap(pPacket->gpu1);
            replayResult = m_xglFuncs.real_xglGetMultiGpuCompatibility(handle0, handle1, &cInfo);
            CHECK_RETURN_VALUE(xglGetMultiGpuCompatibility);
            break;
        }
        case GLV_TPI_XGL_xglOpenSharedMemory:
        {
            struct_xglOpenSharedMemory *pPacket;
            XGL_DEVICE handle;
            XGL_GPU_MEMORY mem;
            pPacket = interpret_body_as_xglOpenSharedMemory(packet);
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenSharedMemory(handle, pPacket->pOpenInfo, &mem);
            CHECK_RETURN_VALUE(xglOpenSharedMemory);
            break;
        }
        case GLV_TPI_XGL_xglOpenSharedQueueSemaphore:
        {
            struct_xglOpenSharedQueueSemaphore *pPacket;
            XGL_DEVICE handle;
            XGL_QUEUE_SEMAPHORE sema;
            pPacket = interpret_body_as_xglOpenSharedQueueSemaphore(packet);
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenSharedQueueSemaphore(handle, pPacket->pOpenInfo, &sema);
            CHECK_RETURN_VALUE(xglOpenSharedQueueSemaphore);
            break;
        }
        case GLV_TPI_XGL_xglOpenPeerMemory:
        {
            struct_xglOpenPeerMemory *pPacket;
            XGL_DEVICE handle;
            XGL_GPU_MEMORY mem;
            pPacket = interpret_body_as_xglOpenPeerMemory(packet);
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenPeerMemory(handle, pPacket->pOpenInfo, &mem);
            CHECK_RETURN_VALUE(xglOpenPeerMemory);
            break;
        }
        case GLV_TPI_XGL_xglOpenPeerImage:
        {
            struct_xglOpenPeerImage *pPacket;
            XGL_DEVICE handle;
            XGL_GPU_MEMORY mem;
            XGL_IMAGE img;
            pPacket = interpret_body_as_xglOpenPeerImage(packet);
            handle = remap(pPacket->device);
            replayResult = m_xglFuncs.real_xglOpenPeerImage(handle, pPacket->pOpenInfo, &img, &mem);
            CHECK_RETURN_VALUE(xglOpenPeerImage);
            break;
        }
        case GLV_TPI_XGL_xglDestroyObject:
        {
            struct_xglDestroyObject *pPacket;
            pPacket = interpret_body_as_xglDestroyObject(packet);
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
            struct_xglGetObjectInfo *pPacket;
            pPacket = interpret_body_as_xglGetObjectInfo(packet);

            XGL_SIZE size = 0;
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
                if (size != *pPacket->pDataSize && pData == NULL)
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
            struct_xglBindObjectMemory *pPacket;
            pPacket = interpret_body_as_xglBindObjectMemory(packet);
            replayResult = m_xglFuncs.real_xglBindObjectMemory(remap(pPacket->object), remap(pPacket->mem), pPacket->offset);
            CHECK_RETURN_VALUE(xglBindObjectInfo);
            break;
        }
        case  GLV_TPI_XGL_xglCreateFence:
        {
            struct_xglCreateFence* pPacket = interpret_body_as_xglCreateFence(packet);
            XGL_FENCE fence;
            replayResult = m_xglFuncs.real_xglCreateFence(remap(pPacket->device), pPacket->pCreateInfo, &fence);
            if (replayResult == XGL_SUCCESS)
                add_to_map(pPacket->pFence, &fence);
            CHECK_RETURN_VALUE(xglCreateFence);
            break;
        }
        case  GLV_TPI_XGL_xglGetFenceStatus:
        {
            struct_xglGetFenceStatus* pPacket = interpret_body_as_xglGetFenceStatus(packet);
            replayResult = m_xglFuncs.real_xglGetFenceStatus(remap(pPacket->fence));
            CHECK_RETURN_VALUE(xglGetFenceStatus);
            break;
        }
        case  GLV_TPI_XGL_xglWaitForFences:
        {
            struct_xglWaitForFences* pPacket = interpret_body_as_xglWaitForFences(packet);
            XGL_FENCE *pFence = GLV_NEW_ARRAY(XGL_FENCE, pPacket->fenceCount);
            for (XGL_UINT i = 0; i < pPacket->fenceCount; i++)
            {
                *(pFence + i) = remap(*(pPacket->pFences + i));
            }
            replayResult = m_xglFuncs.real_xglWaitForFences(remap(pPacket->device), pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);
            GLV_DELETE(pFence);
            CHECK_RETURN_VALUE(xglWaitForFences);
            break;
        }
        case  GLV_TPI_XGL_xglCreateQueueSemaphore:
        {
            struct_xglCreateQueueSemaphore* pPacket = interpret_body_as_xglCreateQueueSemaphore(packet);
            XGL_QUEUE_SEMAPHORE sema;
            replayResult = m_xglFuncs.real_xglCreateQueueSemaphore(remap(pPacket->device), pPacket->pCreateInfo, &sema);
            if (replayResult == XGL_SUCCESS)
                add_to_map(pPacket->pSemaphore, &sema);
            CHECK_RETURN_VALUE(xglCreateQueueSemaphore);
            break;
        }
        case  GLV_TPI_XGL_xglSignalQueueSemaphore:
        {
            struct_xglSignalQueueSemaphore* pPacket = interpret_body_as_xglSignalQueueSemaphore(packet);
            replayResult = m_xglFuncs.real_xglSignalQueueSemaphore(remap(pPacket->queue), remap(pPacket->semaphore));
            CHECK_RETURN_VALUE(xglSignalQueueSemaphore);
            break;
        }
        case  GLV_TPI_XGL_xglWaitQueueSemaphore:
        {
            struct_xglWaitQueueSemaphore* pPacket = interpret_body_as_xglWaitQueueSemaphore(packet);
            replayResult = m_xglFuncs.real_xglWaitQueueSemaphore(remap(pPacket->queue), remap(pPacket->semaphore));
            CHECK_RETURN_VALUE(xglWaitQueueSemaphore);
            break;
        }
        case  GLV_TPI_XGL_xglCreateEvent:
        {
            struct_xglCreateEvent* pPacket = interpret_body_as_xglCreateEvent(packet);
            XGL_EVENT event;
            replayResult = m_xglFuncs.real_xglCreateEvent(remap(pPacket->device), pPacket->pCreateInfo, &event);
            if (replayResult == XGL_SUCCESS)
                add_to_map(pPacket->pEvent, &event);
            CHECK_RETURN_VALUE(xglCreateEvent);
            break;
        }
        case  GLV_TPI_XGL_xglGetEventStatus:
        {
            struct_xglGetEventStatus* pPacket = interpret_body_as_xglGetEventStatus(packet);
            replayResult = m_xglFuncs.real_xglGetEventStatus(remap(pPacket->event));
            if (replayResult != pPacket->result)
            {
                glv_LogWarn("Mismatched return from xglGetEventStatus() traced result %s, replay result %s\n",
                string_XGL_RESULT(pPacket->result), string_XGL_RESULT(replayResult));
                returnValue = glv_replay::GLV_REPLAY_BAD_RETURN;
            }

            if (replayResult != XGL_EVENT_SET && replayResult != XGL_EVENT_RESET)
            {
                glv_LogWarn("xglGetEventStatus() returned failed result %s\n", string_XGL_RESULT(replayResult));
                returnValue = glv_replay::GLV_REPLAY_CALL_ERROR;
            }
            break;
        }
        case  GLV_TPI_XGL_xglSetEvent:
        {
            struct_xglSetEvent* pPacket = interpret_body_as_xglSetEvent(packet);
            replayResult = m_xglFuncs.real_xglSetEvent(remap(pPacket->event));
            CHECK_RETURN_VALUE(xglSetEvent);
            break;
        }
        case  GLV_TPI_XGL_xglResetEvent:
        {
            struct_xglResetEvent* pPacket = interpret_body_as_xglResetEvent(packet);
            replayResult = m_xglFuncs.real_xglResetEvent(remap(pPacket->event));
            CHECK_RETURN_VALUE(xglResetEvent);
            break;
        }
        case  GLV_TPI_XGL_xglCreateQueryPool:
        {
            struct_xglCreateQueryPool* pPacket = interpret_body_as_xglCreateQueryPool(packet);
            XGL_QUERY_POOL query;
            replayResult = m_xglFuncs.real_xglCreateQueryPool(remap(pPacket->device), pPacket->pCreateInfo, &query);
            if (replayResult == XGL_SUCCESS)
                add_to_map(pPacket->pQueryPool, &query);
            CHECK_RETURN_VALUE(xglCreateQueryPool);
            break;
        }
        case  GLV_TPI_XGL_xglGetQueryPoolResults:
        {
            struct_xglGetQueryPoolResults* pPacket = interpret_body_as_xglGetQueryPoolResults(packet);
            replayResult = m_xglFuncs.real_xglGetQueryPoolResults(remap(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount, pPacket->pDataSize, pPacket->pData);
            CHECK_RETURN_VALUE(xglGetQueryPoolResults);
            break;
        }
        case GLV_TPI_XGL_xglGetFormatInfo:
        {
            struct_xglGetFormatInfo* pPacket = interpret_body_as_xglGetFormatInfo(packet);
            XGL_SIZE size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglGetFormatInfo(remap(pPacket->device), pPacket->format, pPacket->infoType, &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData == NULL)
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
        case GLV_TPI_XGL_xglCreateImage:
        {
            struct_xglCreateImage* pPacket = interpret_body_as_xglCreateImage(packet);
            XGL_IMAGE image;
            replayResult = m_xglFuncs.real_xglCreateImage(remap(pPacket->device), pPacket->pCreateInfo, &image);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pImage, &image);
            }
            CHECK_RETURN_VALUE(xglCreateImage);
            break;
        }
        case GLV_TPI_XGL_xglGetImageSubresourceInfo:
        {
            struct_xglGetImageSubresourceInfo* pPacket = interpret_body_as_xglGetImageSubresourceInfo(packet);
            XGL_SIZE size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglGetImageSubresourceInfo(remap(pPacket->image), pPacket->pSubresource, pPacket->infoType, &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData == NULL)
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
            struct_xglCreateImageView* pPacket = interpret_body_as_xglCreateImageView(packet);
            XGL_IMAGE_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_IMAGE_VIEW_CREATE_INFO));
            createInfo.image = remap(pPacket->pCreateInfo->image);
            XGL_IMAGE_VIEW imageView;
            replayResult = m_xglFuncs.real_xglCreateImageView(remap(pPacket->device), &createInfo, &imageView);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pView, &imageView);
            }
            CHECK_RETURN_VALUE(xglCreateImageView);
            break;
        }
        case GLV_TPI_XGL_xglCreateColorAttachmentView:
        {
            struct_xglCreateColorAttachmentView* pPacket = interpret_body_as_xglCreateColorAttachmentView(packet);
            XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO));
            createInfo.image = remap(pPacket->pCreateInfo->image);
            XGL_COLOR_ATTACHMENT_VIEW ColorAttachmentView;
            replayResult = m_xglFuncs.real_xglCreateColorAttachmentView(remap(pPacket->device), &createInfo, &ColorAttachmentView);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pView, &ColorAttachmentView);
            }
            CHECK_RETURN_VALUE(xglCreateColorAttachmentView);
            break;
        }
        case GLV_TPI_XGL_xglCreateDepthStencilView:
        {
            struct_xglCreateDepthStencilView* pPacket = interpret_body_as_xglCreateDepthStencilView(packet);
            XGL_DEPTH_STENCIL_VIEW_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_DEPTH_STENCIL_VIEW_CREATE_INFO));
            createInfo.image = remap(pPacket->pCreateInfo->image);
            XGL_DEPTH_STENCIL_VIEW view;
            replayResult = m_xglFuncs.real_xglCreateDepthStencilView(remap(pPacket->device), &createInfo, &view);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pView, &view);
            }
            CHECK_RETURN_VALUE(xglCreateDepthStencilView);
            break;
        }
        case GLV_TPI_XGL_xglCreateShader:
        {
            struct_xglCreateShader* pPacket = interpret_body_as_xglCreateShader(packet);
            XGL_SHADER shader;
            replayResult = m_xglFuncs.real_xglCreateShader(remap(pPacket->device), pPacket->pCreateInfo, &shader);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pShader, &shader);
            }
            CHECK_RETURN_VALUE(xglCreateShader);
            break;
        }
        case GLV_TPI_XGL_xglCreateGraphicsPipeline:
        {
            struct_xglCreateGraphicsPipeline* pPacket = interpret_body_as_xglCreateGraphicsPipeline(packet);
            XGL_GRAPHICS_PIPELINE_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));
            // Cast to shader type, as those are of primariy interest and all structs in LL have same header w/ sType & pNext
            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;
            XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)createInfo.pNext;
            while ((NULL != pPacketNext) && (XGL_NULL_HANDLE != pPacketNext)) // TODO : Shouldn't need both of these checks
            {
                if (XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO == pNext->sType)
                    pNext->shader.shader = remap(pPacketNext->shader.shader);
                // TODO : Do we need to do anything here for non-shader blocks?
                pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacketNext->pNext;
                pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;
            }
            XGL_PIPELINE pipeline;
            replayResult = m_xglFuncs.real_xglCreateGraphicsPipeline(remap(pPacket->device), &createInfo, &pipeline);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pPipeline, &pipeline);
            }
            CHECK_RETURN_VALUE(xglCreateGraphicsPipeline);
            break;
        }
        case GLV_TPI_XGL_xglCreateComputePipeline:
        {
            struct_xglCreateComputePipeline* pPacket = interpret_body_as_xglCreateComputePipeline(packet);
            XGL_COMPUTE_PIPELINE_CREATE_INFO createInfo;
            memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_COMPUTE_PIPELINE_CREATE_INFO));
            createInfo.cs.shader = remap(pPacket->pCreateInfo->cs.shader);
            XGL_PIPELINE pipeline;
            replayResult = m_xglFuncs.real_xglCreateComputePipeline(remap(pPacket->device), &createInfo, &pipeline);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pPipeline, &pipeline);
            }
            CHECK_RETURN_VALUE(xglCreateComputePipeline);
            break;
        }
        case GLV_TPI_XGL_xglStorePipeline:
        {
            struct_xglStorePipeline* pPacket = interpret_body_as_xglStorePipeline(packet);
            XGL_SIZE size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglStorePipeline(remap(pPacket->pipeline), &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData == NULL)
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
            struct_xglLoadPipeline* pPacket = interpret_body_as_xglLoadPipeline(packet);
            XGL_PIPELINE pipeline;
            replayResult = m_xglFuncs.real_xglLoadPipeline(remap(pPacket->device), pPacket->dataSize, pPacket->pData, &pipeline);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pPipeline, &pipeline);
            }
            CHECK_RETURN_VALUE(xglLoadPipeline);
            break;
        }
        case GLV_TPI_XGL_xglCreatePipelineDelta:
        {
            struct_xglCreatePipelineDelta* pPacket = interpret_body_as_xglCreatePipelineDelta(packet);
            XGL_PIPELINE_DELTA pipeline;
            replayResult = m_xglFuncs.real_xglCreatePipelineDelta(remap(pPacket->device), pPacket->p1, pPacket->p2, &pipeline);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->delta, &pipeline);
            }
            CHECK_RETURN_VALUE(xglCreatePipelineDelta);
            break;
        }
        case GLV_TPI_XGL_xglCreateSampler:
        {
            struct_xglCreateSampler* pPacket = interpret_body_as_xglCreateSampler(packet);
            XGL_SAMPLER sampler;
            replayResult = m_xglFuncs.real_xglCreateSampler(remap(pPacket->device), pPacket->pCreateInfo, &sampler);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pSampler, &sampler);
            }
            CHECK_RETURN_VALUE(xglCreateSampler);
            break;
        }
        case GLV_TPI_XGL_xglCreateDescriptorSet:
        {
            struct_xglCreateDescriptorSet* pPacket = interpret_body_as_xglCreateDescriptorSet(packet);
            XGL_DESCRIPTOR_SET descriptorSet;
            replayResult = m_xglFuncs.real_xglCreateDescriptorSet(remap(pPacket->device), pPacket->pCreateInfo, &descriptorSet);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pDescriptorSet, &descriptorSet);
            }
            CHECK_RETURN_VALUE(xglCreateDescriptorSet);
            break;
        }
        case GLV_TPI_XGL_xglBeginDescriptorSetUpdate:
        {
            struct_xglBeginDescriptorSetUpdate* pPacket = interpret_body_as_xglBeginDescriptorSetUpdate(packet);
             m_xglFuncs.real_xglBeginDescriptorSetUpdate(remap(pPacket->descriptorSet));
            break;
        }
        case GLV_TPI_XGL_xglEndDescriptorSetUpdate:
        {
            struct_xglEndDescriptorSetUpdate* pPacket = interpret_body_as_xglEndDescriptorSetUpdate(packet);
             m_xglFuncs.real_xglEndDescriptorSetUpdate(remap(pPacket->descriptorSet));
            break;
        }
        case GLV_TPI_XGL_xglAttachSamplerDescriptors:
        {
            struct_xglAttachSamplerDescriptors* pPacket = interpret_body_as_xglAttachSamplerDescriptors(packet);
            XGL_SAMPLER* pSamplers = GLV_NEW_ARRAY(XGL_SAMPLER, pPacket->slotCount);
            memcpy(pSamplers, pPacket->pSamplers, pPacket->slotCount * sizeof(XGL_SAMPLER));
            for (XGL_UINT i = 0; i < pPacket->slotCount; i++)
            {
                pSamplers[i] = remap(pPacket->pSamplers[i]);
            }
             m_xglFuncs.real_xglAttachSamplerDescriptors(remap(pPacket->descriptorSet), pPacket->startSlot, pPacket->slotCount, pSamplers);
            GLV_DELETE(pSamplers);
            break;
        }
        case GLV_TPI_XGL_xglAttachImageViewDescriptors:
        {
            struct_xglAttachImageViewDescriptors* pPacket = interpret_body_as_xglAttachImageViewDescriptors(packet);
            XGL_IMAGE_VIEW_ATTACH_INFO* pImageViews = GLV_NEW_ARRAY(XGL_IMAGE_VIEW_ATTACH_INFO, pPacket->slotCount);
            memcpy(pImageViews, pPacket->pImageViews, pPacket->slotCount * sizeof(XGL_IMAGE_VIEW_ATTACH_INFO));
            for (XGL_UINT i = 0; i < pPacket->slotCount; i++)
            {
                pImageViews[i].view = remap(pPacket->pImageViews[i].view);
            }
             m_xglFuncs.real_xglAttachImageViewDescriptors(remap(pPacket->descriptorSet), pPacket->startSlot, pPacket->slotCount, pImageViews);
            GLV_DELETE(pImageViews);
            break;
        }
        case GLV_TPI_XGL_xglAttachMemoryViewDescriptors:
        {
            struct_xglAttachMemoryViewDescriptors* pPacket = interpret_body_as_xglAttachMemoryViewDescriptors(packet);
            XGL_MEMORY_VIEW_ATTACH_INFO* pMemViews = GLV_NEW_ARRAY(XGL_MEMORY_VIEW_ATTACH_INFO, pPacket->slotCount);
            memcpy(pMemViews, pPacket->pMemViews, pPacket->slotCount * sizeof(XGL_MEMORY_VIEW_ATTACH_INFO));
            for (XGL_UINT i = 0; i < pPacket->slotCount; i++)
            {
                pMemViews[i].mem = remap(pPacket->pMemViews[i].mem);
            }
             m_xglFuncs.real_xglAttachMemoryViewDescriptors(remap(pPacket->descriptorSet), pPacket->startSlot, pPacket->slotCount, pMemViews);
            GLV_DELETE(pMemViews);
            break;
        }
        case GLV_TPI_XGL_xglAttachNestedDescriptors:
        {
            struct_xglAttachNestedDescriptors* pPacket = interpret_body_as_xglAttachNestedDescriptors(packet);
            XGL_DESCRIPTOR_SET_ATTACH_INFO* pNestedDescriptorSets = GLV_NEW_ARRAY(XGL_DESCRIPTOR_SET_ATTACH_INFO, pPacket->slotCount);
            memcpy(pNestedDescriptorSets, pPacket->pNestedDescriptorSets, pPacket->slotCount * sizeof(XGL_DESCRIPTOR_SET_ATTACH_INFO));
            for (XGL_UINT i = 0; i < pPacket->slotCount; i++)
            {
                pNestedDescriptorSets[i].descriptorSet = remap(pPacket->pNestedDescriptorSets[i].descriptorSet);
            }
             m_xglFuncs.real_xglAttachNestedDescriptors(remap(pPacket->descriptorSet), pPacket->startSlot, pPacket->slotCount, pNestedDescriptorSets);
            GLV_DELETE(pNestedDescriptorSets);
            break;
        }
        case GLV_TPI_XGL_xglClearDescriptorSetSlots:
        {
            struct_xglClearDescriptorSetSlots* pPacket = interpret_body_as_xglClearDescriptorSetSlots(packet);
             m_xglFuncs.real_xglClearDescriptorSetSlots(remap(pPacket->descriptorSet), pPacket->startSlot, pPacket->slotCount);
            break;
        }
        case GLV_TPI_XGL_xglCreateViewportState:
        {
            struct_xglCreateViewportState* pPacket = interpret_body_as_xglCreateViewportState(packet);
            XGL_VIEWPORT_STATE_OBJECT state;
            replayResult = m_xglFuncs.real_xglCreateViewportState(remap(pPacket->device), pPacket->pCreateInfo, &state);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &state);
            }
            CHECK_RETURN_VALUE(xglCreateViewportState);
            break;
        }
        case GLV_TPI_XGL_xglCreateRasterState:
        {
            struct_xglCreateRasterState* pPacket = interpret_body_as_xglCreateRasterState(packet);
            XGL_RASTER_STATE_OBJECT state;
            replayResult = m_xglFuncs.real_xglCreateRasterState(remap(pPacket->device), pPacket->pCreateInfo, &state);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &state);
            }
            CHECK_RETURN_VALUE(xglCreateRasterState);
            break;
        }
        case GLV_TPI_XGL_xglCreateMsaaState:
        {
            struct_xglCreateMsaaState* pPacket = interpret_body_as_xglCreateMsaaState(packet);
            XGL_MSAA_STATE_OBJECT state;
            replayResult = m_xglFuncs.real_xglCreateMsaaState(remap(pPacket->device), pPacket->pCreateInfo, &state);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &state);
            }
            CHECK_RETURN_VALUE(xglCreateMsaaState);
            break;
        }
        case GLV_TPI_XGL_xglCreateColorBlendState:
        {
            struct_xglCreateColorBlendState* pPacket = interpret_body_as_xglCreateColorBlendState(packet);
            XGL_COLOR_BLEND_STATE_OBJECT state;
            replayResult = m_xglFuncs.real_xglCreateColorBlendState(remap(pPacket->device), pPacket->pCreateInfo, &state);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &state);
            }
            CHECK_RETURN_VALUE(xglCreateColorBlendState);
            break;
        }
        case GLV_TPI_XGL_xglCreateDepthStencilState:
        {
            struct_xglCreateDepthStencilState* pPacket = interpret_body_as_xglCreateDepthStencilState(packet);
            XGL_DEPTH_STENCIL_STATE_OBJECT state;
            replayResult = m_xglFuncs.real_xglCreateDepthStencilState(remap(pPacket->device), pPacket->pCreateInfo, &state);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pState, &state);
            }
            CHECK_RETURN_VALUE(xglCreateDepthStencilState);
            break;
        }
        case GLV_TPI_XGL_xglCreateCommandBuffer:
        {
            struct_xglCreateCommandBuffer* pPacket = interpret_body_as_xglCreateCommandBuffer(packet);
            XGL_CMD_BUFFER cmdBuffer;
            replayResult = m_xglFuncs.real_xglCreateCommandBuffer(remap(pPacket->device), pPacket->pCreateInfo, &cmdBuffer);
            if (replayResult == XGL_SUCCESS)
            {
                add_to_map(pPacket->pCmdBuffer, &cmdBuffer);
            }
            CHECK_RETURN_VALUE(xglCreateCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglBeginCommandBuffer:
        {
            struct_xglBeginCommandBuffer* pPacket = interpret_body_as_xglBeginCommandBuffer(packet);
            replayResult = m_xglFuncs.real_xglBeginCommandBuffer(remap(pPacket->cmdBuffer), pPacket->flags);
            CHECK_RETURN_VALUE(xglBeginCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglEndCommandBuffer:
        {
            struct_xglEndCommandBuffer* pPacket = interpret_body_as_xglEndCommandBuffer(packet);
            replayResult = m_xglFuncs.real_xglEndCommandBuffer(remap(pPacket->cmdBuffer));
            CHECK_RETURN_VALUE(xglEndCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglResetCommandBuffer:
        {
            struct_xglResetCommandBuffer* pPacket = interpret_body_as_xglResetCommandBuffer(packet);
            replayResult = m_xglFuncs.real_xglResetCommandBuffer(remap(pPacket->cmdBuffer));
            CHECK_RETURN_VALUE(xglResetCommandBuffer);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindPipeline:
        {
            struct_xglCmdBindPipeline* pPacket = interpret_body_as_xglCmdBindPipeline(packet);
            m_xglFuncs.real_xglCmdBindPipeline(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, remap(pPacket->pipeline));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindPipelineDelta:
        {
            struct_xglCmdBindPipelineDelta* pPacket = interpret_body_as_xglCmdBindPipelineDelta(packet);
            m_xglFuncs.real_xglCmdBindPipelineDelta(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, remap(pPacket->delta));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindStateObject:
        {
            struct_xglCmdBindStateObject* pPacket = interpret_body_as_xglCmdBindStateObject(packet);
             m_xglFuncs.real_xglCmdBindStateObject(remap(pPacket->cmdBuffer), pPacket->stateBindPoint, remap(pPacket->state));
            break;
        }
        case GLV_TPI_XGL_xglCmdBindDescriptorSet:
        {
            struct_xglCmdBindDescriptorSet* pPacket = interpret_body_as_xglCmdBindDescriptorSet(packet);
             m_xglFuncs.real_xglCmdBindDescriptorSet(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->index, remap(pPacket->descriptorSet), pPacket->slotOffset);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindDynamicMemoryView:
        {
            struct_xglCmdBindDynamicMemoryView* pPacket = interpret_body_as_xglCmdBindDynamicMemoryView(packet);
            XGL_MEMORY_VIEW_ATTACH_INFO memView;
            memcpy(&memView, pPacket->pMemView, sizeof(XGL_MEMORY_VIEW_ATTACH_INFO));
            memView.mem = remap(pPacket->pMemView->mem);
             m_xglFuncs.real_xglCmdBindDynamicMemoryView(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, &memView);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindIndexData:
        {
            struct_xglCmdBindIndexData* pPacket = interpret_body_as_xglCmdBindIndexData(packet);
             m_xglFuncs.real_xglCmdBindIndexData(remap(pPacket->cmdBuffer), remap(pPacket->mem), pPacket->offset, pPacket->indexType);
            break;
        }
        case GLV_TPI_XGL_xglCmdBindAttachments:
        {
            struct_xglCmdBindAttachments* pPacket = interpret_body_as_xglCmdBindAttachments(packet);
            // adjust color targets
            XGL_COLOR_ATTACHMENT_BIND_INFO* pColorAttachments = (XGL_COLOR_ATTACHMENT_BIND_INFO*)pPacket->pColorAttachments;
            bool allocatedColorAttachments = false;
            if (pColorAttachments != NULL)
            {
                allocatedColorAttachments = true;
                pColorAttachments = GLV_NEW_ARRAY(XGL_COLOR_ATTACHMENT_BIND_INFO, pPacket->colorAttachmentCount);
                memcpy(pColorAttachments, pPacket->pColorAttachments, sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO) * pPacket->colorAttachmentCount);
                for (XGL_UINT i = 0; i < pPacket->colorAttachmentCount; i++)
                {
                    pColorAttachments[i].view = remap(pPacket->pColorAttachments[i].view);
                }
            }

            // adjust depth stencil target
            const XGL_DEPTH_STENCIL_BIND_INFO* pDepthAttachment = pPacket->pDepthAttachment;
            XGL_DEPTH_STENCIL_BIND_INFO depthTarget;
            if (pDepthAttachment != NULL)
            {
                memcpy(&depthTarget, pPacket->pDepthAttachment, sizeof(XGL_DEPTH_STENCIL_BIND_INFO));
                depthTarget.view = remap(pPacket->pDepthAttachment->view);
                pDepthAttachment = &depthTarget;
            }

            // make call
             m_xglFuncs.real_xglCmdBindAttachments(remap(pPacket->cmdBuffer), pPacket->colorAttachmentCount, pColorAttachments, pDepthAttachment);

            // cleanup
            if (allocatedColorAttachments)
            {
                GLV_DELETE((void*)pColorAttachments);
            }
            break;
        }
        case GLV_TPI_XGL_xglCmdPrepareMemoryRegions:
        {
            struct_xglCmdPrepareMemoryRegions* pPacket = interpret_body_as_xglCmdPrepareMemoryRegions(packet);
            // adjust transitions
            XGL_MEMORY_STATE_TRANSITION* pStateTransitions = (XGL_MEMORY_STATE_TRANSITION*)pPacket->pStateTransitions;
            bool allocatedMem = false;
            if (pStateTransitions != NULL)
            {
                allocatedMem = true;
                pStateTransitions = GLV_NEW_ARRAY(XGL_MEMORY_STATE_TRANSITION, pPacket->transitionCount);
                memcpy(pStateTransitions, pPacket->pStateTransitions, sizeof(XGL_MEMORY_STATE_TRANSITION) * pPacket->transitionCount);
                for (XGL_UINT i = 0; i < pPacket->transitionCount; i++)
                {
                    pStateTransitions[i].mem = remap(pPacket->pStateTransitions[i].mem);
                }
            }

             m_xglFuncs.real_xglCmdPrepareMemoryRegions(remap(pPacket->cmdBuffer), pPacket->transitionCount, pStateTransitions);

            // cleanup
            if (allocatedMem)
            {
                GLV_DELETE((void*)pStateTransitions);
            }

            break;
        }
        case GLV_TPI_XGL_xglCmdPrepareImages:
        {
            struct_xglCmdPrepareImages* pPacket = interpret_body_as_xglCmdPrepareImages(packet);

            // adjust transitions
            XGL_IMAGE_STATE_TRANSITION* pStateTransitions = (XGL_IMAGE_STATE_TRANSITION*)pPacket->pStateTransitions;
            bool allocatedMem = false;
            if (pStateTransitions != NULL)
            {
                allocatedMem = true;
                pStateTransitions = GLV_NEW_ARRAY(XGL_IMAGE_STATE_TRANSITION, pPacket->transitionCount);
                memcpy(pStateTransitions, pPacket->pStateTransitions, sizeof(XGL_IMAGE_STATE_TRANSITION) * pPacket->transitionCount);
                for (XGL_UINT i = 0; i < pPacket->transitionCount; i++)
                {
                    pStateTransitions[i].image = remap(pPacket->pStateTransitions[i].image);
                }
            }

             m_xglFuncs.real_xglCmdPrepareImages(remap(pPacket->cmdBuffer), pPacket->transitionCount, pStateTransitions);

            // cleanup
            if (allocatedMem)
            {
                GLV_DELETE((void*)pStateTransitions);
            }
            break;
        }
        case GLV_TPI_XGL_xglCmdDraw:
        {
            struct_xglCmdDraw* pPacket = interpret_body_as_xglCmdDraw(packet);
             m_xglFuncs.real_xglCmdDraw(remap(pPacket->cmdBuffer), pPacket->firstVertex, pPacket->vertexCount, pPacket->firstInstance, pPacket->instanceCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexed:
        {
            struct_xglCmdDrawIndexed* pPacket = interpret_body_as_xglCmdDrawIndexed(packet);
             m_xglFuncs.real_xglCmdDrawIndexed(remap(pPacket->cmdBuffer), pPacket->firstIndex, pPacket->indexCount, pPacket->vertexOffset, pPacket->firstInstance, pPacket->instanceCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndirect:
        {
            struct_xglCmdDrawIndirect* pPacket = interpret_body_as_xglCmdDrawIndirect(packet);
             m_xglFuncs.real_xglCmdDrawIndirect(remap(pPacket->cmdBuffer), remap(pPacket->mem), pPacket->offset, pPacket->count, pPacket->stride);
            break;
        }
        case GLV_TPI_XGL_xglCmdDrawIndexedIndirect:
        {
            struct_xglCmdDrawIndexedIndirect* pPacket = interpret_body_as_xglCmdDrawIndexedIndirect(packet);
             m_xglFuncs.real_xglCmdDrawIndexedIndirect(remap(pPacket->cmdBuffer), remap(pPacket->mem), pPacket->offset, pPacket->count, pPacket->stride);
            break;
        }
        case GLV_TPI_XGL_xglCmdDispatch:
        {
            struct_xglCmdDispatch* pPacket = interpret_body_as_xglCmdDispatch(packet);
             m_xglFuncs.real_xglCmdDispatch(remap(pPacket->cmdBuffer), pPacket->x, pPacket->y, pPacket->z);
            break;
        }
        case GLV_TPI_XGL_xglCmdDispatchIndirect:
        {
            struct_xglCmdDispatchIndirect* pPacket = interpret_body_as_xglCmdDispatchIndirect(packet);
             m_xglFuncs.real_xglCmdDispatchIndirect(remap(pPacket->cmdBuffer), remap(pPacket->mem), pPacket->offset);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyMemory:
        {
            struct_xglCmdCopyMemory* pPacket = interpret_body_as_xglCmdCopyMemory(packet);
             m_xglFuncs.real_xglCmdCopyMemory(remap(pPacket->cmdBuffer), remap(pPacket->srcMem), remap(pPacket->destMem), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyImage:
        {
            struct_xglCmdCopyImage* pPacket = interpret_body_as_xglCmdCopyImage(packet);
             m_xglFuncs.real_xglCmdCopyImage(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), remap(pPacket->destImage), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyMemoryToImage:
        {
            struct_xglCmdCopyMemoryToImage* pPacket = interpret_body_as_xglCmdCopyMemoryToImage(packet);
             m_xglFuncs.real_xglCmdCopyMemoryToImage(remap(pPacket->cmdBuffer), remap(pPacket->srcMem), remap(pPacket->destImage), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCopyImageToMemory:
        {
            struct_xglCmdCopyImageToMemory* pPacket = interpret_body_as_xglCmdCopyImageToMemory(packet);
             m_xglFuncs.real_xglCmdCopyImageToMemory(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), remap(pPacket->destMem), pPacket->regionCount, pPacket->pRegions);
            break;
        }
        case GLV_TPI_XGL_xglCmdCloneImageData:
        {
            struct_xglCmdCloneImageData* pPacket = interpret_body_as_xglCmdCloneImageData(packet);
             m_xglFuncs.real_xglCmdCloneImageData(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), pPacket->srcImageState, remap(pPacket->destImage), pPacket->destImageState);
            break;
        }
        case GLV_TPI_XGL_xglCmdUpdateMemory:
        {
            struct_xglCmdUpdateMemory* pPacket = interpret_body_as_xglCmdUpdateMemory(packet);
             m_xglFuncs.real_xglCmdUpdateMemory(remap(pPacket->cmdBuffer), remap(pPacket->destMem), pPacket->destOffset, pPacket->dataSize, pPacket->pData);
            break;
        }
        case GLV_TPI_XGL_xglCmdFillMemory:
        {
            struct_xglCmdFillMemory* pPacket = interpret_body_as_xglCmdFillMemory(packet);
             m_xglFuncs.real_xglCmdFillMemory(remap(pPacket->cmdBuffer), remap(pPacket->destMem), pPacket->destOffset, pPacket->fillSize, pPacket->data);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearColorImage:
        {
            struct_xglCmdClearColorImage* pPacket = interpret_body_as_xglCmdClearColorImage(packet);
             m_xglFuncs.real_xglCmdClearColorImage(remap(pPacket->cmdBuffer), remap(pPacket->image), pPacket->color, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearColorImageRaw:
        {
            struct_xglCmdClearColorImageRaw* pPacket = interpret_body_as_xglCmdClearColorImageRaw(packet);
             m_xglFuncs.real_xglCmdClearColorImageRaw(remap(pPacket->cmdBuffer), remap(pPacket->image), pPacket->color, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdClearDepthStencil:
        {
            struct_xglCmdClearDepthStencil* pPacket = interpret_body_as_xglCmdClearDepthStencil(packet);
             m_xglFuncs.real_xglCmdClearDepthStencil(remap(pPacket->cmdBuffer), remap(pPacket->image), pPacket->depth, pPacket->stencil, pPacket->rangeCount, pPacket->pRanges);
            break;
        }
        case GLV_TPI_XGL_xglCmdResolveImage:
        {
            struct_xglCmdResolveImage* pPacket = interpret_body_as_xglCmdResolveImage(packet);
             m_xglFuncs.real_xglCmdResolveImage(remap(pPacket->cmdBuffer), remap(pPacket->srcImage), remap(pPacket->destImage), pPacket->rectCount, pPacket->pRects);
            break;
        }
        case GLV_TPI_XGL_xglCmdSetEvent:
        {
            struct_xglCmdSetEvent* pPacket = interpret_body_as_xglCmdSetEvent(packet);
             m_xglFuncs.real_xglCmdSetEvent(remap(pPacket->cmdBuffer), remap(pPacket->event));
            break;
        }
        case GLV_TPI_XGL_xglCmdResetEvent:
        {
            struct_xglCmdResetEvent* pPacket = interpret_body_as_xglCmdResetEvent(packet);
             m_xglFuncs.real_xglCmdResetEvent(remap(pPacket->cmdBuffer), remap(pPacket->event));
            break;
        }
        case GLV_TPI_XGL_xglCmdMemoryAtomic:
        {
            struct_xglCmdMemoryAtomic* pPacket = interpret_body_as_xglCmdMemoryAtomic(packet);
             m_xglFuncs.real_xglCmdMemoryAtomic(remap(pPacket->cmdBuffer), remap(pPacket->destMem), pPacket->destOffset, pPacket->srcData, pPacket->atomicOp);
            break;
        }
        case GLV_TPI_XGL_xglCmdBeginQuery:
        {
            struct_xglCmdBeginQuery* pPacket = interpret_body_as_xglCmdBeginQuery(packet);
             m_xglFuncs.real_xglCmdBeginQuery(remap(pPacket->cmdBuffer), remap(pPacket->queryPool), pPacket->slot, pPacket->flags);
            break;
        }
        case GLV_TPI_XGL_xglCmdEndQuery:
        {
            struct_xglCmdEndQuery* pPacket = interpret_body_as_xglCmdEndQuery(packet);
             m_xglFuncs.real_xglCmdEndQuery(remap(pPacket->cmdBuffer), remap(pPacket->queryPool), pPacket->slot);
            break;
        }
        case GLV_TPI_XGL_xglCmdResetQueryPool:
        {
            struct_xglCmdResetQueryPool* pPacket = interpret_body_as_xglCmdResetQueryPool(packet);
             m_xglFuncs.real_xglCmdResetQueryPool(remap(pPacket->cmdBuffer), remap(pPacket->queryPool), pPacket->startQuery, pPacket->queryCount);
            break;
        }
        case GLV_TPI_XGL_xglCmdWriteTimestamp:
        {
            struct_xglCmdWriteTimestamp* pPacket = interpret_body_as_xglCmdWriteTimestamp(packet);
             m_xglFuncs.real_xglCmdWriteTimestamp(remap(pPacket->cmdBuffer), pPacket->timestampType, remap(pPacket->destMem), pPacket->destOffset);
            break;
        }
        case GLV_TPI_XGL_xglCmdInitAtomicCounters:
        {
            struct_xglCmdInitAtomicCounters* pPacket = interpret_body_as_xglCmdInitAtomicCounters(packet);
             m_xglFuncs.real_xglCmdInitAtomicCounters(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, pPacket->pData);
            break;
        }
        case GLV_TPI_XGL_xglCmdLoadAtomicCounters:
        {
            struct_xglCmdLoadAtomicCounters* pPacket = interpret_body_as_xglCmdLoadAtomicCounters(packet);
             m_xglFuncs.real_xglCmdLoadAtomicCounters(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, pPacket->srcMem, pPacket->srcOffset);
            break;
        }
        case GLV_TPI_XGL_xglCmdSaveAtomicCounters:
        {
            struct_xglCmdSaveAtomicCounters* pPacket = interpret_body_as_xglCmdSaveAtomicCounters(packet);
             m_xglFuncs.real_xglCmdSaveAtomicCounters(remap(pPacket->cmdBuffer), pPacket->pipelineBindPoint, pPacket->startCounter, pPacket->counterCount, remap(pPacket->destMem), pPacket->destOffset);
            break;
        }

        // xglDbg.h functions
        case GLV_TPI_XGL_xglDbgSetValidationLevel:
        {
		struct_xglDbgSetValidationLevel *pPacket = interpret_body_as_xglDbgSetValidationLevel(packet);
		replayResult = m_xglFuncs.real_xglDbgSetValidationLevel(remap(pPacket->device), pPacket->validationLevel);
		CHECK_RETURN_VALUE(xglDbgSetValidationLevel);
		break;
        }
        case GLV_TPI_XGL_xglDbgRegisterMsgCallback:
        {
		struct_xglDbgRegisterMsgCallback *pPacket = interpret_body_as_xglDbgRegisterMsgCallback(packet);
		// just pass NULL since don't have replay pointer to app data
		replayResult = m_xglFuncs.real_xglDbgRegisterMsgCallback(pPacket->pfnMsgCallback, NULL);
		CHECK_RETURN_VALUE(xglDbgRegisterMsgCallback);
		break;
        }
        case GLV_TPI_XGL_xglDbgUnregisterMsgCallback:
        {
		struct_xglDbgUnregisterMsgCallback *pPacket = interpret_body_as_xglDbgUnregisterMsgCallback(packet);
		replayResult = m_xglFuncs.real_xglDbgUnregisterMsgCallback(pPacket->pfnMsgCallback);
		CHECK_RETURN_VALUE(xglDbgUnregisterMsgCallback);
		break;
        }
        case GLV_TPI_XGL_xglDbgSetMessageFilter:
        {
		struct_xglDbgSetMessageFilter *pPacket = interpret_body_as_xglDbgSetMessageFilter(packet);
		replayResult = m_xglFuncs.real_xglDbgSetMessageFilter(remap(pPacket->device), pPacket->msgCode, pPacket->filter);
		CHECK_RETURN_VALUE(xglDbgSetMessageFilter);
		break;
        }
        case GLV_TPI_XGL_xglDbgSetObjectTag:
        {
		struct_xglDbgSetObjectTag *pPacket = interpret_body_as_xglDbgSetObjectTag(packet);
		replayResult = m_xglFuncs.real_xglDbgSetObjectTag(remap(pPacket->object), pPacket->tagSize, pPacket->pTag);
		CHECK_RETURN_VALUE(xglDbgSetObjectTag);
		break;
        }
        case GLV_TPI_XGL_xglDbgSetGlobalOption:
        {
		struct_xglDbgSetGlobalOption *pPacket = interpret_body_as_xglDbgSetGlobalOption(packet);
		replayResult = m_xglFuncs.real_xglDbgSetGlobalOption(pPacket->dbgOption, pPacket->dataSize, pPacket->pData);
		CHECK_RETURN_VALUE(xglDbgSetGlobalOption);
		break;
        }
        case GLV_TPI_XGL_xglDbgSetDeviceOption:
        {
		struct_xglDbgSetDeviceOption *pPacket = interpret_body_as_xglDbgSetDeviceOption(packet);
		replayResult = m_xglFuncs.real_xglDbgSetDeviceOption(remap(pPacket->device), pPacket->dbgOption, pPacket->dataSize, pPacket->pData);
		CHECK_RETURN_VALUE(xglDbgSetDeviceOption);
		break;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerBegin:
        {
		struct_xglCmdDbgMarkerBegin *pPacket = interpret_body_as_xglCmdDbgMarkerBegin(packet);
                m_xglFuncs.real_xglCmdDbgMarkerBegin(remap(pPacket->cmdBuffer), pPacket->pMarker);
		break;
        }
        case GLV_TPI_XGL_xglCmdDbgMarkerEnd:
        {
		struct_xglCmdDbgMarkerEnd *pPacket = interpret_body_as_xglCmdDbgMarkerEnd(packet);
                m_xglFuncs.real_xglCmdDbgMarkerEnd(remap(pPacket->cmdBuffer));
		break;
        }

        // xglWsiX11Ext.h
        case GLV_TPI_XGL_xglWsiX11AssociateConnection:
        {
            struct_xglWsiX11AssociateConnection *pPacket = interpret_body_as_xglWsiX11AssociateConnection(packet);
            //associate with the replayers Wsi connection rather than tracers
            replayResult = m_xglFuncs.real_xglWsiX11AssociateConnection(remap(pPacket->gpu), &(m_display->m_WsiConnection));
            CHECK_RETURN_VALUE(xglWsiX11AssociateConnection);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11GetMSC:
        {
            struct_xglWsiX11GetMSC *pPacket = interpret_body_as_xglWsiX11GetMSC(packet);
            replayResult = m_xglFuncs.real_xglWsiX11GetMSC(remap(pPacket->device), pPacket->window, pPacket->crtc, pPacket->pMsc);
            CHECK_RETURN_VALUE(xglWsiX11GetMSC);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11CreatePresentableImage:
        {
            struct_xglWsiX11CreatePresentableImage *pPacket = interpret_body_as_xglWsiX11CreatePresentableImage(packet);
            XGL_IMAGE img;
            XGL_GPU_MEMORY mem;
            replayResult = m_xglFuncs.real_xglWsiX11CreatePresentableImage(remap(pPacket->device), pPacket->pCreateInfo, &img, &mem);
            if (replayResult == XGL_SUCCESS)
            {
                if (pPacket->pImage != NULL)
                    add_to_map(pPacket->pImage, &img);
                if(pPacket->pMem != NULL)
                    add_to_map(pPacket->pMem, &mem);
            }
            CHECK_RETURN_VALUE(xglWsiX11CreatePresentableImage);
            break;
        }
        case GLV_TPI_XGL_xglWsiX11QueuePresent:
        {
            struct_xglWsiX11QueuePresent *pPacket = interpret_body_as_xglWsiX11QueuePresent(packet);
            XGL_WSI_X11_PRESENT_INFO pInfo;
            memcpy(&pInfo, pPacket->pPresentInfo, sizeof(XGL_WSI_X11_PRESENT_INFO));
            pInfo.srcImage = remap(pPacket->pPresentInfo->srcImage);
            // use replayers Xcb window
            pInfo.destWindow = m_display->m_XcbWindow;
            replayResult = m_xglFuncs.real_xglWsiX11QueuePresent(remap(pPacket->queue), &pInfo, remap(pPacket->fence));
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

