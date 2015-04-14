#include <xcb/xcb.h>

#include <string>

#include <vulkan.h>

// COPIED FROM "loader.c" (not pointed to, because we're about to delete this
// code).  Ian Elliott <ian@lunarg.com>.
#include <windows.h>
char *loader_get_registry_string(const HKEY hive,
                                 const LPCTSTR sub_key,
                                 const char *value)
{
    DWORD access_flags = KEY_QUERY_VALUE;
    DWORD value_type;
    HKEY key;
    LONG  rtn_value;
    char *rtn_str = NULL;
    size_t rtn_len = 0;
    size_t allocated_len = 0;

    rtn_value = RegOpenKeyEx(hive, sub_key, 0, access_flags, &key);
    if (rtn_value != ERROR_SUCCESS) {
        // We didn't find the key.  Try the 32-bit hive (where we've seen the
        // key end up on some people's systems):
        access_flags |= KEY_WOW64_32KEY;
        rtn_value = RegOpenKeyEx(hive, sub_key, 0, access_flags, &key);
        if (rtn_value != ERROR_SUCCESS) {
            // We still couldn't find the key, so give up:
            return NULL;
        }
    }

    rtn_value = RegQueryValueEx(key, value, NULL, &value_type,
                                (LPBYTE) rtn_str, (LPDWORD) &rtn_len);
    if (rtn_value == ERROR_SUCCESS) {
        // If we get to here, we found the key, and need to allocate memory
        // large enough for rtn_str, and query again:
        allocated_len = rtn_len + 4;
        rtn_str = (char*) malloc(allocated_len);
        rtn_value = RegQueryValueEx(key, value, NULL, &value_type,
                                    (LPBYTE) rtn_str, (LPDWORD) &rtn_len);
        if (rtn_value == ERROR_SUCCESS) {
            // We added 4 extra bytes to rtn_str, so that we can ensure that
            // the string is NULL-terminated (albeit, in a brute-force manner):
            rtn_str[allocated_len-1] = '\0';
        } else {
            // This should never occur, but in case it does, clean up:
            free(rtn_str);
            rtn_str = NULL;
        }
    } // else - shouldn't happen, but if it does, return rtn_str, which is NULL

    // Close the registry key that was opened:
    RegCloseKey(key);

    return rtn_str;
}


typedef HWND (*xcbConnectType)();
typedef void (*xcbCreateWindowType)(uint16_t width, uint16_t height);
typedef void (*xcbDestroyWindowType)();
typedef int (*xcbGetMessageType)(MSG * msg);
typedef BOOL (*xcbPeekMessageType)(MSG * msg);

struct xcb_connection_t {
    xcb_screen_t screens[1];
    xcb_setup_t setup;
    HMODULE xcbNvidia;

    xcbConnectType xcbConnect;
    xcbCreateWindowType xcbCreateWindow;
    xcbDestroyWindowType xcbDestroyWindow;
    xcbGetMessageType xcbGetMessage;
    xcbPeekMessageType xcbPeekMessage;

    HWND hwnd;
};

xcb_connection_t * xcb_connect(const char *displayname, int *screenp)
{
    std::string vkNvidia = (getenv("VK_DRIVERS_PATH") == NULL) ? "" : getenv("VK_DRIVERS_PATH");
    vkNvidia += "\\VK_nvidia.dll";
    HMODULE module = LoadLibrary(vkNvidia.c_str());
    if (!module) {
        std::string vkNulldrv = (getenv("VK_DRIVERS_PATH") == NULL) ? "" : getenv("VK_DRIVERS_PATH");
        vkNulldrv += "\\vk_nulldrv.dll";
        module = LoadLibrary(vkNulldrv.c_str());
    }
    if (!module) {
        // TODO: Adapted up the following code (copied from "loader.c"):
        char *registry_str = NULL;
        DWORD registry_len = 0;
        DWORD registry_value_type;
        LONG  registry_return_value;
        char *rtn_str = NULL;
        size_t rtn_len;

        registry_str = loader_get_registry_string(HKEY_LOCAL_MACHINE,
                                                  "Software\\VK",
                                                  "VK_DRIVERS_PATH");
        registry_len = strlen(registry_str);
        rtn_len = registry_len + 16;
        rtn_str = (char *) malloc(rtn_len);
        _snprintf(rtn_str, rtn_len, "%s\\%s", registry_str, "vk_nvidia.dll");
        module = LoadLibrary(rtn_str);

        free(rtn_str);
    }
    if (!module) {
        return 0;
    }

    xcb_connection_t * connection = (xcb_connection_t *)calloc(1, sizeof(xcb_connection_t));
    connection->xcbNvidia = module;

    connection->xcbConnect = (xcbConnectType)GetProcAddress(module, "xcbConnect");
    connection->xcbCreateWindow = (xcbCreateWindowType)GetProcAddress(module, "xcbCreateWindow");
    connection->xcbDestroyWindow = (xcbDestroyWindowType)GetProcAddress(module, "xcbDestroyWindow");
    connection->xcbGetMessage = (xcbGetMessageType)GetProcAddress(module, "xcbGetMessage");
    connection->xcbPeekMessage = (xcbPeekMessageType)GetProcAddress(module, "xcbPeekMessage");

    if (!connection->xcbConnect) {
        return 0;
    }
    connection->hwnd = connection->xcbConnect();

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
    // This is a MONSTER hack to make VK_nvidia compatible with both
    // the LunarG apps and Dota2.
    return (uint32_t)c->hwnd;
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

xcb_void_cookie_t
xcb_configure_window (xcb_connection_t *c  ,
                      xcb_window_t      window  ,
                      uint16_t          value_mask  ,
                      const uint32_t   *value_list  )
{
    uint32_t width = 0;
    uint32_t height = 0;

    size_t index = 0;
    for (size_t i = 0; i < sizeof (uint16_t); ++i) {
        switch (value_mask & (1 << i)) {
        case XCB_CONFIG_WINDOW_WIDTH:
            width = value_list[index++];
            break;
        case XCB_CONFIG_WINDOW_HEIGHT:
            height = value_list[index++];
            break;
        default:
            break;
        }
    }

    if (width && height) {
        // Resize the window...
    }

    return xcb_void_cookie_t();
}
