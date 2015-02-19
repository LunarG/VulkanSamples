#include <xcb/xcb.h>
#include <xglWsiX11Ext.h>

#include <Windows.h>
#include <string>
#include <deque>

#include <xgl.h>

#include "../loader/loader_platform.h"

typedef void (*xcbCreateWindowType)(uint16_t width, uint16_t height);
typedef void (*xcbDestroyWindowType)();
typedef int (*xcbGetMessageType)(MSG * msg);
typedef BOOL (*xcbPeekMessageType)(MSG * msg);
typedef XGL_RESULT (*xcbQueuePresentType)(XGL_QUEUE queue, XGL_IMAGE image, XGL_FENCE fence);

struct xcb_connection_t {
    xcb_screen_t screens[1];
    xcb_setup_t setup;
    HMODULE xcbNvidia;

    xcbCreateWindowType xcbCreateWindow;
    xcbDestroyWindowType xcbDestroyWindow;
    xcbGetMessageType xcbGetMessage;
    xcbPeekMessageType xcbPeekMessage;
    xcbQueuePresentType xcbQueuePresent;
};

// XCB id database.
// FIXME: This is not thread safe.
uint32_t g_xcbId = 0;
struct XcbId {
    xcb_connection_t * connection;
};
std::deque<XcbId> g_xcbIds;

xcb_connection_t * xcb_connect(const char *displayname, int *screenp)
{
    std::string xglNvidia = getenv(DRIVER_PATH_ENV);
    xglNvidia += "\\xgl_nvidia.dll";
    HMODULE module = LoadLibrary(xglNvidia.c_str());
    if (!module) {
        std::string xglNulldrv = getenv("LIBXGL_DRIVERS_PATH");
        xglNulldrv += "\\xgl_nulldrv.dll";
        module = LoadLibrary(xglNulldrv.c_str());
    }
    if (!module) {
        return 0;
    }

    xcb_connection_t * connection = (xcb_connection_t *)calloc(1, sizeof(xcb_connection_t));
    connection->xcbNvidia = module;

    connection->xcbCreateWindow = (xcbCreateWindowType)GetProcAddress(module, "xcbCreateWindow");
    connection->xcbDestroyWindow = (xcbDestroyWindowType)GetProcAddress(module, "xcbDestroyWindow");
    connection->xcbGetMessage = (xcbGetMessageType)GetProcAddress(module, "xcbGetMessage");
    connection->xcbPeekMessage = (xcbPeekMessageType)GetProcAddress(module, "xcbPeekMessage");
    connection->xcbQueuePresent = (xcbQueuePresentType)GetProcAddress(module, "xcbQueuePresent");

    *screenp = 0;
    return static_cast<xcb_connection_t *>(connection);
}

void xcb_disconnect(xcb_connection_t *c)
{
    xcb_connection_t * connection = static_cast<xcb_connection_t *>(c);
    FreeLibrary(connection->xcbNvidia);
    free(connection);
}

int xcb_flush(xcb_connection_t * c)
{
    return 0;
}

static xcb_generic_event_t* TranslateWindowsMsg(MSG * msg)
{
    switch (msg->message) {
    case WM_PAINT:
        {
            xcb_generic_event_t * event = (xcb_generic_event_t *)calloc(1, sizeof (xcb_generic_event_t));
            event->response_type = XCB_EXPOSE;
            return event;
        }
        break;
    case WM_KEYUP:
        {
            xcb_key_release_event_t * event = (xcb_key_release_event_t *)calloc(1, sizeof (xcb_key_release_event_t));
            event->response_type = XCB_KEY_RELEASE;

            // TODO: What's the correct mapping?
            switch (msg->wParam) {
            case VK_ESCAPE:
                event->detail = 0x09;
                break;
            case VK_LEFT:
                event->detail = 0x71;
                break;
            case VK_RIGHT:
                event->detail = 0x72;
                break;
            case VK_SPACE:
                event->detail = 0x41;
                break;
            default:
                event->detail = (xcb_keycode_t)msg->wParam;
                break;
            }
            return (xcb_generic_event_t *)event;
        }
        break;
    case WM_DESTROY:
        {
            xcb_generic_event_t * event = (xcb_generic_event_t *)calloc(1, sizeof (xcb_generic_event_t));
            event->response_type = XCB_DESTROY_NOTIFY;
            return event;
        }
        break;
    default:
        return 0;
    }

    return 0;
}

xcb_generic_event_t* xcb_wait_for_event(xcb_connection_t * c)
{
    xcb_connection_t * connection = static_cast<xcb_connection_t *>(c);

    MSG msg;
    int result = connection->xcbGetMessage(&msg);
    if (result > 0) {
        return TranslateWindowsMsg(&msg);
    } else if (result == 0) {
        xcb_generic_event_t * event = (xcb_generic_event_t *)calloc(1, sizeof (xcb_generic_event_t));
        event->response_type = XCB_DESTROY_NOTIFY;
        return event;
    }

    return 0;
}

xcb_generic_event_t *xcb_poll_for_event(xcb_connection_t *c)
{
    xcb_connection_t * connection = static_cast<xcb_connection_t *>(c);

    MSG msg;
    if (connection->xcbPeekMessage(&msg)) {
        return TranslateWindowsMsg(&msg);
    }

    return 0;
}

uint32_t xcb_generate_id(xcb_connection_t *c)
{
    // FIXME: THIS IS NOT THREAD SAFE.
    uint32_t id = (uint32_t)g_xcbIds.size();
    XcbId xcbId = { static_cast<xcb_connection_t *>(c) };
    g_xcbIds.push_back(xcbId);
    return id;
}

xcb_void_cookie_t
xcb_create_window(xcb_connection_t *c,
                  uint8_t           depth,
                  xcb_window_t      wid,
                  xcb_window_t      parent,
                  int16_t           x,
                  int16_t           y,
                  uint16_t          width,
                  uint16_t          height,
                  uint16_t          border_width,
                  uint16_t          _class,
                  xcb_visualid_t    visual,
                  uint32_t          value_mask,
                  const uint32_t   *value_list)
{
    xcb_connection_t * connection = static_cast<xcb_connection_t *>(c);
    connection->xcbCreateWindow(width, height);

    xcb_void_cookie_t cookie = { };
    return cookie;
}

xcb_void_cookie_t
xcb_destroy_window(xcb_connection_t *c,
                   xcb_window_t      window)
{
    xcb_connection_t * connection = static_cast<xcb_connection_t *>(c);
    connection->xcbDestroyWindow();

    xcb_void_cookie_t cookie = { };
    return cookie;
}

xcb_intern_atom_cookie_t
xcb_intern_atom(xcb_connection_t *c,
                uint8_t           only_if_exists,
                uint16_t          name_len,
                const char       *name)
{
    xcb_intern_atom_cookie_t cookie = { };
    return cookie;
}

xcb_intern_atom_reply_t *
xcb_intern_atom_reply(xcb_connection_t          *c,
                      xcb_intern_atom_cookie_t   cookie,
                      xcb_generic_error_t      **e)
{
    xcb_intern_atom_reply_t * reply = (xcb_intern_atom_reply_t *)calloc(1, sizeof (xcb_intern_atom_reply_t));
  
    return reply;
}

xcb_void_cookie_t
xcb_change_property(xcb_connection_t *c,
                    uint8_t           mode,
                    xcb_window_t      window,
                    xcb_atom_t        property,
                    xcb_atom_t        type,
                    uint8_t           format,
                    uint32_t          data_len,
                    const void       *data)
{
    xcb_void_cookie_t cookie = { };
    return cookie;
}

xcb_void_cookie_t
xcb_map_window(xcb_connection_t *c,
               xcb_window_t      window)
{
    xcb_void_cookie_t cookie = { };
    return cookie;
}

const struct xcb_setup_t* xcb_get_setup(xcb_connection_t * c)
{
    xcb_connection_t * connection = static_cast<xcb_connection_t *>(c);
    return &connection->setup;
}

#define OFFSET_OF(_struct_, _member_) \
    ( \
        reinterpret_cast<size_t>(&(reinterpret_cast<_struct_ *>(1)->_member_)) - \
        reinterpret_cast<size_t>(reinterpret_cast<_struct_ *>(1)) \
    )

/// Returns a pointer to a struct or class based on the pointer to one of
/// its members.
#define STRUCT_PTR_FROM_MEMBER_PTR(_struct_, _member_, _member_ptr_) \
    reinterpret_cast<_struct_ *>( \
        reinterpret_cast<size_t>(_member_ptr_) - \
        OFFSET_OF(_struct_, _member_) \
    )

xcb_screen_iterator_t
xcb_setup_roots_iterator(const xcb_setup_t *R)
{
    xcb_connection_t * connection = STRUCT_PTR_FROM_MEMBER_PTR(xcb_connection_t, setup, R);

    xcb_screen_iterator_t iterator = { };
    iterator.data = &connection->screens[0];
    return iterator;
}

void
xcb_screen_next(xcb_screen_iterator_t *i)
{
}

XGL_RESULT XGLAPI xglWsiX11AssociateConnection(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_WSI_X11_CONNECTION_INFO*          pConnectionInfo)
{
    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(
    XGL_DEVICE                                  device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    XGL_RESULT err;

    XGL_IMAGE_CREATE_INFO presentable_image = { XGL_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    presentable_image.imageType = XGL_IMAGE_2D;
    presentable_image.format = pCreateInfo->format;
    presentable_image.extent.width = pCreateInfo->extent.width;
    presentable_image.extent.height = pCreateInfo->extent.height;
    presentable_image.extent.depth = 1;
    presentable_image.mipLevels = 1;
    presentable_image.arraySize = 1;
    presentable_image.samples = 1;
    presentable_image.tiling = XGL_OPTIMAL_TILING;
    presentable_image.usage = XGL_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    err = xglCreateImage(device, &presentable_image, pImage);
    if (err != XGL_SUCCESS) {
        return err;
    }

    uint32_t num_allocations = 0;
    size_t num_alloc_size = sizeof(num_allocations);
    err = xglGetObjectInfo(*pImage, XGL_INFO_TYPE_MEMORY_ALLOCATION_COUNT, &num_alloc_size, &num_allocations);
    if (err != XGL_SUCCESS) {
        xglDestroyObject(*pImage);
        return err;
    }
    if (num_allocations > 1) {
        xglDestroyObject(*pImage);
        return XGL_UNSUPPORTED;
    }

    size_t mem_reqs_size = sizeof(XGL_MEMORY_REQUIREMENTS);
    XGL_MEMORY_REQUIREMENTS mem_reqs;
    err = xglGetObjectInfo(*pImage, XGL_INFO_TYPE_MEMORY_REQUIREMENTS, &mem_reqs_size, &mem_reqs);
    if (err != XGL_SUCCESS) {
        xglDestroyObject(*pImage);
        return err;
    }

    size_t img_reqs_size = sizeof(XGL_IMAGE_MEMORY_REQUIREMENTS);
    XGL_IMAGE_MEMORY_REQUIREMENTS img_reqs;
    err = xglGetObjectInfo(*pImage, XGL_INFO_TYPE_IMAGE_MEMORY_REQUIREMENTS, &img_reqs_size, &img_reqs);
    if (err != XGL_SUCCESS) {
        xglDestroyObject(*pImage);
        return err;
    }

    XGL_MEMORY_ALLOC_IMAGE_INFO img_alloc = { XGL_STRUCTURE_TYPE_MEMORY_ALLOC_IMAGE_INFO };
    img_alloc.usage = img_reqs.usage;
    img_alloc.formatClass = img_reqs.formatClass;
    img_alloc.samples = img_reqs.samples;
    XGL_MEMORY_ALLOC_INFO mem_alloc = { XGL_STRUCTURE_TYPE_MEMORY_ALLOC_INFO };
    mem_alloc.pNext = &img_alloc;
    mem_alloc.allocationSize = 0,
    mem_alloc.memProps = XGL_MEMORY_PROPERTY_GPU_ONLY,
    mem_alloc.memType = XGL_MEMORY_TYPE_IMAGE,
    mem_alloc.memPriority = XGL_MEMORY_PRIORITY_NORMAL,
    mem_alloc.allocationSize = mem_reqs.size;
    err = xglAllocMemory(device, &mem_alloc, pMem);
    if (err != XGL_SUCCESS) {
        xglDestroyObject(*pImage);
        return err;
    }

    err = xglBindObjectMemory(*pImage, 0, *pMem, 0);
    if (err != XGL_SUCCESS) {
        xglFreeMemory(*pMem);
        xglDestroyObject(*pImage);
        return err;
    }

    return XGL_SUCCESS;
}

XGL_RESULT XGLAPI xglWsiX11QueuePresent(
    XGL_QUEUE                                   queue,
    const XGL_WSI_X11_PRESENT_INFO*             pPresentInfo,
    XGL_FENCE                                   fence)
{
    xcb_connection_t * connection = g_xcbIds[pPresentInfo->destWindow].connection;
    return connection->xcbQueuePresent(queue, pPresentInfo->srcImage, fence);
}
