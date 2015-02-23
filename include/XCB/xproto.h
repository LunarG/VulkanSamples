/*
 * This file generated automatically from xproto.xml by c_client.py.
 * Edit at your peril.
 */

#ifndef __XPROTO_H
#define __XPROTO_H

#include "xcb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xcb_char2b_t {
    uint8_t byte1; 
    uint8_t byte2; 
} xcb_char2b_t;

typedef struct xcb_char2b_iterator_t {
    xcb_char2b_t *data; 
    int           rem; 
    int           index; 
} xcb_char2b_iterator_t;

typedef uint32_t xcb_window_t;

typedef struct xcb_window_iterator_t {
    xcb_window_t *data; 
    int           rem; 
    int           index; 
} xcb_window_iterator_t;

typedef uint32_t xcb_pixmap_t;

typedef struct xcb_pixmap_iterator_t {
    xcb_pixmap_t *data; 
    int           rem; 
    int           index; 
} xcb_pixmap_iterator_t;

typedef uint32_t xcb_cursor_t;

typedef struct xcb_cursor_iterator_t {
    xcb_cursor_t *data; 
    int           rem; 
    int           index; 
} xcb_cursor_iterator_t;

typedef uint32_t xcb_font_t;

typedef struct xcb_font_iterator_t {
    xcb_font_t *data; 
    int         rem; 
    int         index; 
} xcb_font_iterator_t;

typedef uint32_t xcb_gcontext_t;

typedef struct xcb_gcontext_iterator_t {
    xcb_gcontext_t *data; 
    int             rem; 
    int             index; 
} xcb_gcontext_iterator_t;

typedef uint32_t xcb_colormap_t;

typedef struct xcb_colormap_iterator_t {
    xcb_colormap_t *data; 
    int             rem; 
    int             index; 
} xcb_colormap_iterator_t;

typedef uint32_t xcb_atom_t;

typedef struct xcb_atom_iterator_t {
    xcb_atom_t *data; 
    int         rem; 
    int         index; 
} xcb_atom_iterator_t;

typedef uint32_t xcb_drawable_t;

typedef struct xcb_drawable_iterator_t {
    xcb_drawable_t *data; 
    int             rem; 
    int             index; 
} xcb_drawable_iterator_t;

typedef uint32_t xcb_fontable_t;

typedef struct xcb_fontable_iterator_t {
    xcb_fontable_t *data; 
    int             rem; 
    int             index; 
} xcb_fontable_iterator_t;

typedef uint32_t xcb_visualid_t;

typedef struct xcb_visualid_iterator_t {
    xcb_visualid_t *data; 
    int             rem; 
    int             index; 
} xcb_visualid_iterator_t;

typedef uint32_t xcb_timestamp_t;

typedef struct xcb_timestamp_iterator_t {
    xcb_timestamp_t *data; 
    int              rem; 
    int              index; 
} xcb_timestamp_iterator_t;

typedef uint32_t xcb_keysym_t;

typedef struct xcb_keysym_iterator_t {
    xcb_keysym_t *data; 
    int           rem; 
    int           index; 
} xcb_keysym_iterator_t;

typedef uint8_t xcb_keycode_t;

typedef struct xcb_keycode_iterator_t {
    xcb_keycode_t *data; 
    int            rem; 
    int            index; 
} xcb_keycode_iterator_t;

typedef uint8_t xcb_button_t;

typedef struct xcb_button_iterator_t {
    xcb_button_t *data; 
    int           rem; 
    int           index; 
} xcb_button_iterator_t;

typedef struct xcb_point_t {
    int16_t x; 
    int16_t y; 
} xcb_point_t;

typedef struct xcb_point_iterator_t {
    xcb_point_t *data; 
    int          rem; 
    int          index; 
} xcb_point_iterator_t;

typedef struct xcb_rectangle_t {
    int16_t  x; 
    int16_t  y; 
    uint16_t width; 
    uint16_t height; 
} xcb_rectangle_t;

typedef struct xcb_rectangle_iterator_t {
    xcb_rectangle_t *data; 
    int              rem; 
    int              index; 
} xcb_rectangle_iterator_t;

typedef struct xcb_arc_t {
    int16_t  x; 
    int16_t  y; 
    uint16_t width; 
    uint16_t height; 
    int16_t  angle1; 
    int16_t  angle2; 
} xcb_arc_t;

typedef struct xcb_arc_iterator_t {
    xcb_arc_t *data; 
    int        rem; 
    int        index; 
} xcb_arc_iterator_t;

typedef struct xcb_format_t {
    uint8_t depth; 
    uint8_t bits_per_pixel; 
    uint8_t scanline_pad; 
    uint8_t pad0[5]; 
} xcb_format_t;

typedef struct xcb_format_iterator_t {
    xcb_format_t *data; 
    int           rem; 
    int           index; 
} xcb_format_iterator_t;

typedef enum xcb_visual_class_t {
    XCB_VISUAL_CLASS_STATIC_GRAY = 0,
    XCB_VISUAL_CLASS_GRAY_SCALE = 1,
    XCB_VISUAL_CLASS_STATIC_COLOR = 2,
    XCB_VISUAL_CLASS_PSEUDO_COLOR = 3,
    XCB_VISUAL_CLASS_TRUE_COLOR = 4,
    XCB_VISUAL_CLASS_DIRECT_COLOR = 5
} xcb_visual_class_t;

typedef struct xcb_visualtype_t {
    xcb_visualid_t visual_id; 
    uint8_t        _class; 
    uint8_t        bits_per_rgb_value; 
    uint16_t       colormap_entries; 
    uint32_t       red_mask; 
    uint32_t       green_mask; 
    uint32_t       blue_mask; 
    uint8_t        pad0[4]; 
} xcb_visualtype_t;

typedef struct xcb_visualtype_iterator_t {
    xcb_visualtype_t *data; 
    int               rem; 
    int               index; 
} xcb_visualtype_iterator_t;

typedef struct xcb_depth_t {
    uint8_t  depth; 
    uint8_t  pad0; 
    uint16_t visuals_len; 
    uint8_t  pad1[4]; 
} xcb_depth_t;

typedef struct xcb_depth_iterator_t {
    xcb_depth_t *data; 
    int          rem; 
    int          index; 
} xcb_depth_iterator_t;

typedef enum xcb_event_mask_t {
    XCB_EVENT_MASK_NO_EVENT = 0,
    XCB_EVENT_MASK_KEY_PRESS = 1,
    XCB_EVENT_MASK_KEY_RELEASE = 2,
    XCB_EVENT_MASK_BUTTON_PRESS = 4,
    XCB_EVENT_MASK_BUTTON_RELEASE = 8,
    XCB_EVENT_MASK_ENTER_WINDOW = 16,
    XCB_EVENT_MASK_LEAVE_WINDOW = 32,
    XCB_EVENT_MASK_POINTER_MOTION = 64,
    XCB_EVENT_MASK_POINTER_MOTION_HINT = 128,
    XCB_EVENT_MASK_BUTTON_1_MOTION = 256,
    XCB_EVENT_MASK_BUTTON_2_MOTION = 512,
    XCB_EVENT_MASK_BUTTON_3_MOTION = 1024,
    XCB_EVENT_MASK_BUTTON_4_MOTION = 2048,
    XCB_EVENT_MASK_BUTTON_5_MOTION = 4096,
    XCB_EVENT_MASK_BUTTON_MOTION = 8192,
    XCB_EVENT_MASK_KEYMAP_STATE = 16384,
    XCB_EVENT_MASK_EXPOSURE = 32768,
    XCB_EVENT_MASK_VISIBILITY_CHANGE = 65536,
    XCB_EVENT_MASK_STRUCTURE_NOTIFY = 131072,
    XCB_EVENT_MASK_RESIZE_REDIRECT = 262144,
    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY = 524288,
    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT = 1048576,
    XCB_EVENT_MASK_FOCUS_CHANGE = 2097152,
    XCB_EVENT_MASK_PROPERTY_CHANGE = 4194304,
    XCB_EVENT_MASK_COLOR_MAP_CHANGE = 8388608,
    XCB_EVENT_MASK_OWNER_GRAB_BUTTON = 16777216
} xcb_event_mask_t;

typedef enum xcb_backing_store_t {
    XCB_BACKING_STORE_NOT_USEFUL = 0,
    XCB_BACKING_STORE_WHEN_MAPPED = 1,
    XCB_BACKING_STORE_ALWAYS = 2
} xcb_backing_store_t;

typedef struct xcb_screen_t {
    xcb_window_t   root; 
    xcb_colormap_t default_colormap; 
    uint32_t       white_pixel; 
    uint32_t       black_pixel; 
    uint32_t       current_input_masks; 
    uint16_t       width_in_pixels; 
    uint16_t       height_in_pixels; 
    uint16_t       width_in_millimeters; 
    uint16_t       height_in_millimeters; 
    uint16_t       min_installed_maps; 
    uint16_t       max_installed_maps; 
    xcb_visualid_t root_visual; 
    uint8_t        backing_stores; 
    uint8_t        save_unders; 
    uint8_t        root_depth; 
    uint8_t        allowed_depths_len; 
} xcb_screen_t;

typedef struct xcb_screen_iterator_t {
    xcb_screen_t *data; 
    int           rem; 
    int           index; 
} xcb_screen_iterator_t;

typedef struct xcb_setup_request_t {
    uint8_t  byte_order; 
    uint8_t  pad0; 
    uint16_t protocol_major_version; 
    uint16_t protocol_minor_version; 
    uint16_t authorization_protocol_name_len; 
    uint16_t authorization_protocol_data_len; 
    uint8_t  pad1[2]; 
} xcb_setup_request_t;

typedef struct xcb_setup_request_iterator_t {
    xcb_setup_request_t *data; 
    int                  rem; 
    int                  index; 
} xcb_setup_request_iterator_t;

typedef struct xcb_setup_failed_t {
    uint8_t  status; 
    uint8_t  reason_len; 
    uint16_t protocol_major_version; 
    uint16_t protocol_minor_version; 
    uint16_t length; 
} xcb_setup_failed_t;

typedef struct xcb_setup_failed_iterator_t {
    xcb_setup_failed_t *data; 
    int                 rem; 
    int                 index; 
} xcb_setup_failed_iterator_t;

typedef struct xcb_setup_authenticate_t {
    uint8_t  status; 
    uint8_t  pad0[5]; 
    uint16_t length; 
} xcb_setup_authenticate_t;

typedef struct xcb_setup_authenticate_iterator_t {
    xcb_setup_authenticate_t *data; 
    int                       rem; 
    int                       index; 
} xcb_setup_authenticate_iterator_t;

typedef enum xcb_image_order_t {
    XCB_IMAGE_ORDER_LSB_FIRST = 0,
    XCB_IMAGE_ORDER_MSB_FIRST = 1
} xcb_image_order_t;

typedef struct xcb_setup_t {
    uint8_t       status; 
    uint8_t       pad0; 
    uint16_t      protocol_major_version; 
    uint16_t      protocol_minor_version; 
    uint16_t      length; 
    uint32_t      release_number; 
    uint32_t      resource_id_base; 
    uint32_t      resource_id_mask; 
    uint32_t      motion_buffer_size; 
    uint16_t      vendor_len; 
    uint16_t      maximum_request_length; 
    uint8_t       roots_len; 
    uint8_t       pixmap_formats_len; 
    uint8_t       image_byte_order; 
    uint8_t       bitmap_format_bit_order; 
    uint8_t       bitmap_format_scanline_unit; 
    uint8_t       bitmap_format_scanline_pad; 
    xcb_keycode_t min_keycode; 
    xcb_keycode_t max_keycode; 
    uint8_t       pad1[4]; 
} xcb_setup_t;

typedef struct xcb_setup_iterator_t {
    xcb_setup_t *data; 
    int          rem; 
    int          index; 
} xcb_setup_iterator_t;

typedef enum xcb_mod_mask_t {
    XCB_MOD_MASK_SHIFT = 1,
    XCB_MOD_MASK_LOCK = 2,
    XCB_MOD_MASK_CONTROL = 4,
    XCB_MOD_MASK_1 = 8,
    XCB_MOD_MASK_2 = 16,
    XCB_MOD_MASK_3 = 32,
    XCB_MOD_MASK_4 = 64,
    XCB_MOD_MASK_5 = 128,
    XCB_MOD_MASK_ANY = 32768
} xcb_mod_mask_t;

typedef enum xcb_key_but_mask_t {
    XCB_KEY_BUT_MASK_SHIFT = 1,
    XCB_KEY_BUT_MASK_LOCK = 2,
    XCB_KEY_BUT_MASK_CONTROL = 4,
    XCB_KEY_BUT_MASK_MOD_1 = 8,
    XCB_KEY_BUT_MASK_MOD_2 = 16,
    XCB_KEY_BUT_MASK_MOD_3 = 32,
    XCB_KEY_BUT_MASK_MOD_4 = 64,
    XCB_KEY_BUT_MASK_MOD_5 = 128,
    XCB_KEY_BUT_MASK_BUTTON_1 = 256,
    XCB_KEY_BUT_MASK_BUTTON_2 = 512,
    XCB_KEY_BUT_MASK_BUTTON_3 = 1024,
    XCB_KEY_BUT_MASK_BUTTON_4 = 2048,
    XCB_KEY_BUT_MASK_BUTTON_5 = 4096
} xcb_key_but_mask_t;

typedef enum xcb_window_enum_t {
    XCB_WINDOW_NONE = 0
} xcb_window_enum_t;

#define XCB_KEY_PRESS 2

typedef struct xcb_key_press_event_t {
    uint8_t         response_type; 
    xcb_keycode_t   detail; 
    uint16_t        sequence; 
    xcb_timestamp_t time; 
    xcb_window_t    root; 
    xcb_window_t    event; 
    xcb_window_t    child; 
    int16_t         root_x; 
    int16_t         root_y; 
    int16_t         event_x; 
    int16_t         event_y; 
    uint16_t        state; 
    uint8_t         same_screen; 
    uint8_t         pad0; 
} xcb_key_press_event_t;

#define XCB_KEY_RELEASE 3

typedef xcb_key_press_event_t xcb_key_release_event_t;

typedef enum xcb_button_mask_t {
    XCB_BUTTON_MASK_1 = 256,
    XCB_BUTTON_MASK_2 = 512,
    XCB_BUTTON_MASK_3 = 1024,
    XCB_BUTTON_MASK_4 = 2048,
    XCB_BUTTON_MASK_5 = 4096,
    XCB_BUTTON_MASK_ANY = 32768
} xcb_button_mask_t;

#define XCB_BUTTON_PRESS 4

typedef struct xcb_button_press_event_t {
    uint8_t         response_type; 
    xcb_button_t    detail; 
    uint16_t        sequence; 
    xcb_timestamp_t time; 
    xcb_window_t    root; 
    xcb_window_t    event; 
    xcb_window_t    child; 
    int16_t         root_x; 
    int16_t         root_y; 
    int16_t         event_x; 
    int16_t         event_y; 
    uint16_t        state; 
    uint8_t         same_screen; 
    uint8_t         pad0; 
} xcb_button_press_event_t;

#define XCB_BUTTON_RELEASE 5

typedef xcb_button_press_event_t xcb_button_release_event_t;

typedef enum xcb_motion_t {
    XCB_MOTION_NORMAL = 0,
    XCB_MOTION_HINT = 1
} xcb_motion_t;

#define XCB_MOTION_NOTIFY 6

typedef struct xcb_motion_notify_event_t {
    uint8_t         response_type; 
    uint8_t         detail; 
    uint16_t        sequence; 
    xcb_timestamp_t time; 
    xcb_window_t    root; 
    xcb_window_t    event; 
    xcb_window_t    child; 
    int16_t         root_x; 
    int16_t         root_y; 
    int16_t         event_x; 
    int16_t         event_y; 
    uint16_t        state; 
    uint8_t         same_screen; 
    uint8_t         pad0; 
} xcb_motion_notify_event_t;

typedef enum xcb_notify_detail_t {
    XCB_NOTIFY_DETAIL_ANCESTOR = 0,
    XCB_NOTIFY_DETAIL_VIRTUAL = 1,
    XCB_NOTIFY_DETAIL_INFERIOR = 2,
    XCB_NOTIFY_DETAIL_NONLINEAR = 3,
    XCB_NOTIFY_DETAIL_NONLINEAR_VIRTUAL = 4,
    XCB_NOTIFY_DETAIL_POINTER = 5,
    XCB_NOTIFY_DETAIL_POINTER_ROOT = 6,
    XCB_NOTIFY_DETAIL_NONE = 7
} xcb_notify_detail_t;

typedef enum xcb_notify_mode_t {
    XCB_NOTIFY_MODE_NORMAL = 0,
    XCB_NOTIFY_MODE_GRAB = 1,
    XCB_NOTIFY_MODE_UNGRAB = 2,
    XCB_NOTIFY_MODE_WHILE_GRABBED = 3
} xcb_notify_mode_t;

#define XCB_ENTER_NOTIFY 7

typedef struct xcb_enter_notify_event_t {
    uint8_t         response_type; 
    uint8_t         detail; 
    uint16_t        sequence; 
    xcb_timestamp_t time; 
    xcb_window_t    root; 
    xcb_window_t    event; 
    xcb_window_t    child; 
    int16_t         root_x; 
    int16_t         root_y; 
    int16_t         event_x; 
    int16_t         event_y; 
    uint16_t        state; 
    uint8_t         mode; 
    uint8_t         same_screen_focus; 
} xcb_enter_notify_event_t;

#define XCB_LEAVE_NOTIFY 8

typedef xcb_enter_notify_event_t xcb_leave_notify_event_t;

#define XCB_FOCUS_IN 9

typedef struct xcb_focus_in_event_t {
    uint8_t      response_type; 
    uint8_t      detail; 
    uint16_t     sequence; 
    xcb_window_t event; 
    uint8_t      mode; 
    uint8_t      pad0[3]; 
} xcb_focus_in_event_t;

#define XCB_FOCUS_OUT 10

typedef xcb_focus_in_event_t xcb_focus_out_event_t;

#define XCB_KEYMAP_NOTIFY 11

typedef struct xcb_keymap_notify_event_t {
    uint8_t response_type; 
    uint8_t keys[31]; 
} xcb_keymap_notify_event_t;

#define XCB_EXPOSE 12

typedef struct xcb_expose_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t window; 
    uint16_t     x; 
    uint16_t     y; 
    uint16_t     width; 
    uint16_t     height; 
    uint16_t     count; 
    uint8_t      pad1[2]; 
} xcb_expose_event_t;

#define XCB_GRAPHICS_EXPOSURE 13

typedef struct xcb_graphics_exposure_event_t {
    uint8_t        response_type; 
    uint8_t        pad0; 
    uint16_t       sequence; 
    xcb_drawable_t drawable; 
    uint16_t       x; 
    uint16_t       y; 
    uint16_t       width; 
    uint16_t       height; 
    uint16_t       minor_opcode; 
    uint16_t       count; 
    uint8_t        major_opcode; 
    uint8_t        pad1[3]; 
} xcb_graphics_exposure_event_t;

#define XCB_NO_EXPOSURE 14

typedef struct xcb_no_exposure_event_t {
    uint8_t        response_type; 
    uint8_t        pad0; 
    uint16_t       sequence; 
    xcb_drawable_t drawable; 
    uint16_t       minor_opcode; 
    uint8_t        major_opcode; 
    uint8_t        pad1; 
} xcb_no_exposure_event_t;

typedef enum xcb_visibility_t {
    XCB_VISIBILITY_UNOBSCURED = 0,
    XCB_VISIBILITY_PARTIALLY_OBSCURED = 1,
    XCB_VISIBILITY_FULLY_OBSCURED = 2
} xcb_visibility_t;

#define XCB_VISIBILITY_NOTIFY 15

typedef struct xcb_visibility_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t window; 
    uint8_t      state; 
    uint8_t      pad1[3]; 
} xcb_visibility_notify_event_t;

#define XCB_CREATE_NOTIFY 16

typedef struct xcb_create_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t parent; 
    xcb_window_t window; 
    int16_t      x; 
    int16_t      y; 
    uint16_t     width; 
    uint16_t     height; 
    uint16_t     border_width; 
    uint8_t      override_redirect; 
    uint8_t      pad1; 
} xcb_create_notify_event_t;

#define XCB_DESTROY_NOTIFY 17

typedef struct xcb_destroy_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t event; 
    xcb_window_t window; 
} xcb_destroy_notify_event_t;

#define XCB_UNMAP_NOTIFY 18

typedef struct xcb_unmap_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t event; 
    xcb_window_t window; 
    uint8_t      from_configure; 
    uint8_t      pad1[3]; 
} xcb_unmap_notify_event_t;

#define XCB_MAP_NOTIFY 19

typedef struct xcb_map_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t event; 
    xcb_window_t window; 
    uint8_t      override_redirect; 
    uint8_t      pad1[3]; 
} xcb_map_notify_event_t;

#define XCB_MAP_REQUEST 20

typedef struct xcb_map_request_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t parent; 
    xcb_window_t window; 
} xcb_map_request_event_t;

#define XCB_REPARENT_NOTIFY 21

typedef struct xcb_reparent_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t event; 
    xcb_window_t window; 
    xcb_window_t parent; 
    int16_t      x; 
    int16_t      y; 
    uint8_t      override_redirect; 
    uint8_t      pad1[3]; 
} xcb_reparent_notify_event_t;

#define XCB_CONFIGURE_NOTIFY 22

typedef struct xcb_configure_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t event; 
    xcb_window_t window; 
    xcb_window_t above_sibling; 
    int16_t      x; 
    int16_t      y; 
    uint16_t     width; 
    uint16_t     height; 
    uint16_t     border_width; 
    uint8_t      override_redirect; 
    uint8_t      pad1; 
} xcb_configure_notify_event_t;

#define XCB_CONFIGURE_REQUEST 23

typedef struct xcb_configure_request_event_t {
    uint8_t      response_type; 
    uint8_t      stack_mode; 
    uint16_t     sequence; 
    xcb_window_t parent; 
    xcb_window_t window; 
    xcb_window_t sibling; 
    int16_t      x; 
    int16_t      y; 
    uint16_t     width; 
    uint16_t     height; 
    uint16_t     border_width; 
    uint16_t     value_mask; 
} xcb_configure_request_event_t;

#define XCB_GRAVITY_NOTIFY 24

typedef struct xcb_gravity_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t event; 
    xcb_window_t window; 
    int16_t      x; 
    int16_t      y; 
} xcb_gravity_notify_event_t;

#define XCB_RESIZE_REQUEST 25

typedef struct xcb_resize_request_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t window; 
    uint16_t     width; 
    uint16_t     height; 
} xcb_resize_request_event_t;

typedef enum xcb_place_t {
    XCB_PLACE_ON_TOP = 0,
    XCB_PLACE_ON_BOTTOM = 1
} xcb_place_t;

#define XCB_CIRCULATE_NOTIFY 26

typedef struct xcb_circulate_notify_event_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    xcb_window_t event; 
    xcb_window_t window; 
    uint8_t      pad1[4]; 
    uint8_t      place; 
    uint8_t      pad2[3]; 
} xcb_circulate_notify_event_t;

#define XCB_CIRCULATE_REQUEST 27

typedef xcb_circulate_notify_event_t xcb_circulate_request_event_t;

typedef enum xcb_property_t {
    XCB_PROPERTY_NEW_VALUE = 0,
    XCB_PROPERTY_DELETE = 1
} xcb_property_t;

#define XCB_PROPERTY_NOTIFY 28

typedef struct xcb_property_notify_event_t {
    uint8_t         response_type; 
    uint8_t         pad0; 
    uint16_t        sequence; 
    xcb_window_t    window; 
    xcb_atom_t      atom; 
    xcb_timestamp_t time; 
    uint8_t         state; 
    uint8_t         pad1[3]; 
} xcb_property_notify_event_t;

#define XCB_SELECTION_CLEAR 29

typedef struct xcb_selection_clear_event_t {
    uint8_t         response_type; 
    uint8_t         pad0; 
    uint16_t        sequence; 
    xcb_timestamp_t time; 
    xcb_window_t    owner; 
    xcb_atom_t      selection; 
} xcb_selection_clear_event_t;

typedef enum xcb_time_t {
    XCB_TIME_CURRENT_TIME = 0
} xcb_time_t;

typedef enum xcb_atom_enum_t {
    XCB_ATOM_NONE = 0,
    XCB_ATOM_ANY = 0,
    XCB_ATOM_PRIMARY = 1,
    XCB_ATOM_SECONDARY = 2,
    XCB_ATOM_ARC = 3,
    XCB_ATOM_ATOM = 4,
    XCB_ATOM_BITMAP = 5,
    XCB_ATOM_CARDINAL = 6,
    XCB_ATOM_COLORMAP = 7,
    XCB_ATOM_CURSOR = 8,
    XCB_ATOM_CUT_BUFFER0 = 9,
    XCB_ATOM_CUT_BUFFER1 = 10,
    XCB_ATOM_CUT_BUFFER2 = 11,
    XCB_ATOM_CUT_BUFFER3 = 12,
    XCB_ATOM_CUT_BUFFER4 = 13,
    XCB_ATOM_CUT_BUFFER5 = 14,
    XCB_ATOM_CUT_BUFFER6 = 15,
    XCB_ATOM_CUT_BUFFER7 = 16,
    XCB_ATOM_DRAWABLE = 17,
    XCB_ATOM_FONT = 18,
    XCB_ATOM_INTEGER = 19,
    XCB_ATOM_PIXMAP = 20,
    XCB_ATOM_POINT = 21,
    XCB_ATOM_RECTANGLE = 22,
    XCB_ATOM_RESOURCE_MANAGER = 23,
    XCB_ATOM_RGB_COLOR_MAP = 24,
    XCB_ATOM_RGB_BEST_MAP = 25,
    XCB_ATOM_RGB_BLUE_MAP = 26,
    XCB_ATOM_RGB_DEFAULT_MAP = 27,
    XCB_ATOM_RGB_GRAY_MAP = 28,
    XCB_ATOM_RGB_GREEN_MAP = 29,
    XCB_ATOM_RGB_RED_MAP = 30,
    XCB_ATOM_STRING = 31,
    XCB_ATOM_VISUALID = 32,
    XCB_ATOM_WINDOW = 33,
    XCB_ATOM_WM_COMMAND = 34,
    XCB_ATOM_WM_HINTS = 35,
    XCB_ATOM_WM_CLIENT_MACHINE = 36,
    XCB_ATOM_WM_ICON_NAME = 37,
    XCB_ATOM_WM_ICON_SIZE = 38,
    XCB_ATOM_WM_NAME = 39,
    XCB_ATOM_WM_NORMAL_HINTS = 40,
    XCB_ATOM_WM_SIZE_HINTS = 41,
    XCB_ATOM_WM_ZOOM_HINTS = 42,
    XCB_ATOM_MIN_SPACE = 43,
    XCB_ATOM_NORM_SPACE = 44,
    XCB_ATOM_MAX_SPACE = 45,
    XCB_ATOM_END_SPACE = 46,
    XCB_ATOM_SUPERSCRIPT_X = 47,
    XCB_ATOM_SUPERSCRIPT_Y = 48,
    XCB_ATOM_SUBSCRIPT_X = 49,
    XCB_ATOM_SUBSCRIPT_Y = 50,
    XCB_ATOM_UNDERLINE_POSITION = 51,
    XCB_ATOM_UNDERLINE_THICKNESS = 52,
    XCB_ATOM_STRIKEOUT_ASCENT = 53,
    XCB_ATOM_STRIKEOUT_DESCENT = 54,
    XCB_ATOM_ITALIC_ANGLE = 55,
    XCB_ATOM_X_HEIGHT = 56,
    XCB_ATOM_QUAD_WIDTH = 57,
    XCB_ATOM_WEIGHT = 58,
    XCB_ATOM_POINT_SIZE = 59,
    XCB_ATOM_RESOLUTION = 60,
    XCB_ATOM_COPYRIGHT = 61,
    XCB_ATOM_NOTICE = 62,
    XCB_ATOM_FONT_NAME = 63,
    XCB_ATOM_FAMILY_NAME = 64,
    XCB_ATOM_FULL_NAME = 65,
    XCB_ATOM_CAP_HEIGHT = 66,
    XCB_ATOM_WM_CLASS = 67,
    XCB_ATOM_WM_TRANSIENT_FOR = 68
} xcb_atom_enum_t;

#define XCB_SELECTION_REQUEST 30

typedef struct xcb_selection_request_event_t {
    uint8_t         response_type; 
    uint8_t         pad0; 
    uint16_t        sequence; 
    xcb_timestamp_t time; 
    xcb_window_t    owner; 
    xcb_window_t    requestor; 
    xcb_atom_t      selection; 
    xcb_atom_t      target; 
    xcb_atom_t      property; 
} xcb_selection_request_event_t;

#define XCB_SELECTION_NOTIFY 31

typedef struct xcb_selection_notify_event_t {
    uint8_t         response_type; 
    uint8_t         pad0; 
    uint16_t        sequence; 
    xcb_timestamp_t time; 
    xcb_window_t    requestor; 
    xcb_atom_t      selection; 
    xcb_atom_t      target; 
    xcb_atom_t      property; 
} xcb_selection_notify_event_t;

typedef enum xcb_colormap_state_t {
    XCB_COLORMAP_STATE_UNINSTALLED = 0,
    XCB_COLORMAP_STATE_INSTALLED = 1
} xcb_colormap_state_t;

typedef enum xcb_colormap_enum_t {
    XCB_COLORMAP_NONE = 0
} xcb_colormap_enum_t;

#define XCB_COLORMAP_NOTIFY 32

typedef struct xcb_colormap_notify_event_t {
    uint8_t        response_type; 
    uint8_t        pad0; 
    uint16_t       sequence; 
    xcb_window_t   window; 
    xcb_colormap_t colormap; 
    uint8_t        _new; 
    uint8_t        state; 
    uint8_t        pad1[2]; 
} xcb_colormap_notify_event_t;

typedef union xcb_client_message_data_t {
    uint8_t  data8[20]; 
    uint16_t data16[10]; 
    uint32_t data32[5]; 
} xcb_client_message_data_t;

typedef struct xcb_client_message_data_iterator_t {
    xcb_client_message_data_t *data; 
    int                        rem; 
    int                        index; 
} xcb_client_message_data_iterator_t;

#define XCB_CLIENT_MESSAGE 33

typedef struct xcb_client_message_event_t {
    uint8_t                   response_type; 
    uint8_t                   format; 
    uint16_t                  sequence; 
    xcb_window_t              window; 
    xcb_atom_t                type; 
    xcb_client_message_data_t data; 
} xcb_client_message_event_t;

typedef enum xcb_mapping_t {
    XCB_MAPPING_MODIFIER = 0,
    XCB_MAPPING_KEYBOARD = 1,
    XCB_MAPPING_POINTER = 2
} xcb_mapping_t;

#define XCB_MAPPING_NOTIFY 34

typedef struct xcb_mapping_notify_event_t {
    uint8_t       response_type; 
    uint8_t       pad0; 
    uint16_t      sequence; 
    uint8_t       request; 
    xcb_keycode_t first_keycode; 
    uint8_t       count; 
    uint8_t       pad1; 
} xcb_mapping_notify_event_t;

#define XCB_GE_GENERIC 35

typedef struct xcb_ge_generic_event_t {
    uint8_t  response_type; 
    uint8_t  extension; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t event_type; 
    uint8_t  pad0[22]; 
    uint32_t full_sequence; 
} xcb_ge_generic_event_t;

#define XCB_REQUEST 1

typedef struct xcb_request_error_t {
    uint8_t  response_type; 
    uint8_t  error_code; 
    uint16_t sequence; 
    uint32_t bad_value; 
    uint16_t minor_opcode; 
    uint8_t  major_opcode; 
    uint8_t  pad0; 
} xcb_request_error_t;

#define XCB_VALUE 2

typedef struct xcb_value_error_t {
    uint8_t  response_type; 
    uint8_t  error_code; 
    uint16_t sequence; 
    uint32_t bad_value; 
    uint16_t minor_opcode; 
    uint8_t  major_opcode; 
    uint8_t  pad0; 
} xcb_value_error_t;

#define XCB_WINDOW 3

typedef xcb_value_error_t xcb_window_error_t;

#define XCB_PIXMAP 4

typedef xcb_value_error_t xcb_pixmap_error_t;

#define XCB_ATOM 5

typedef xcb_value_error_t xcb_atom_error_t;

#define XCB_CURSOR 6

typedef xcb_value_error_t xcb_cursor_error_t;

#define XCB_FONT 7

typedef xcb_value_error_t xcb_font_error_t;

#define XCB_MATCH 8

typedef xcb_request_error_t xcb_match_error_t;

#define XCB_DRAWABLE 9

typedef xcb_value_error_t xcb_drawable_error_t;

#define XCB_ACCESS 10

typedef xcb_request_error_t xcb_access_error_t;

#define XCB_ALLOC 11

typedef xcb_request_error_t xcb_alloc_error_t;

#define XCB_COLORMAP 12

typedef xcb_value_error_t xcb_colormap_error_t;

#define XCB_G_CONTEXT 13

typedef xcb_value_error_t xcb_g_context_error_t;

#define XCB_ID_CHOICE 14

typedef xcb_value_error_t xcb_id_choice_error_t;

#define XCB_NAME 15

typedef xcb_request_error_t xcb_name_error_t;

#define XCB_LENGTH 16

typedef xcb_request_error_t xcb_length_error_t;

#define XCB_IMPLEMENTATION 17

typedef xcb_request_error_t xcb_implementation_error_t;

typedef enum xcb_window_class_t {
    XCB_WINDOW_CLASS_COPY_FROM_PARENT = 0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT = 1,
    XCB_WINDOW_CLASS_INPUT_ONLY = 2
} xcb_window_class_t;

typedef enum xcb_cw_t {
    XCB_CW_BACK_PIXMAP = 1,
    XCB_CW_BACK_PIXEL = 2,
    XCB_CW_BORDER_PIXMAP = 4,
    XCB_CW_BORDER_PIXEL = 8,
    XCB_CW_BIT_GRAVITY = 16,
    XCB_CW_WIN_GRAVITY = 32,
    XCB_CW_BACKING_STORE = 64,
    XCB_CW_BACKING_PLANES = 128,
    XCB_CW_BACKING_PIXEL = 256,
    XCB_CW_OVERRIDE_REDIRECT = 512,
    XCB_CW_SAVE_UNDER = 1024,
    XCB_CW_EVENT_MASK = 2048,
    XCB_CW_DONT_PROPAGATE = 4096,
    XCB_CW_COLORMAP = 8192,
    XCB_CW_CURSOR = 16384
} xcb_cw_t;

typedef enum xcb_back_pixmap_t {
    XCB_BACK_PIXMAP_NONE = 0,
    XCB_BACK_PIXMAP_PARENT_RELATIVE = 1
} xcb_back_pixmap_t;

typedef enum xcb_gravity_t {
    XCB_GRAVITY_BIT_FORGET = 0,
    XCB_GRAVITY_WIN_UNMAP = 0,
    XCB_GRAVITY_NORTH_WEST = 1,
    XCB_GRAVITY_NORTH = 2,
    XCB_GRAVITY_NORTH_EAST = 3,
    XCB_GRAVITY_WEST = 4,
    XCB_GRAVITY_CENTER = 5,
    XCB_GRAVITY_EAST = 6,
    XCB_GRAVITY_SOUTH_WEST = 7,
    XCB_GRAVITY_SOUTH = 8,
    XCB_GRAVITY_SOUTH_EAST = 9,
    XCB_GRAVITY_STATIC = 10
} xcb_gravity_t;

#define XCB_CREATE_WINDOW 1

typedef struct xcb_create_window_request_t {
    uint8_t        major_opcode; 
    uint8_t        depth; 
    uint16_t       length; 
    xcb_window_t   wid; 
    xcb_window_t   parent; 
    int16_t        x; 
    int16_t        y; 
    uint16_t       width; 
    uint16_t       height; 
    uint16_t       border_width; 
    uint16_t       _class; 
    xcb_visualid_t visual; 
    uint32_t       value_mask; 
} xcb_create_window_request_t;

#define XCB_CHANGE_WINDOW_ATTRIBUTES 2

typedef struct xcb_change_window_attributes_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
    uint32_t     value_mask; 
} xcb_change_window_attributes_request_t;

typedef enum xcb_map_state_t {
    XCB_MAP_STATE_UNMAPPED = 0,
    XCB_MAP_STATE_UNVIEWABLE = 1,
    XCB_MAP_STATE_VIEWABLE = 2
} xcb_map_state_t;

typedef struct xcb_get_window_attributes_cookie_t {
    unsigned int sequence; 
} xcb_get_window_attributes_cookie_t;

#define XCB_GET_WINDOW_ATTRIBUTES 3

typedef struct xcb_get_window_attributes_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_get_window_attributes_request_t;

typedef struct xcb_get_window_attributes_reply_t {
    uint8_t        response_type; 
    uint8_t        backing_store; 
    uint16_t       sequence; 
    uint32_t       length; 
    xcb_visualid_t visual; 
    uint16_t       _class; 
    uint8_t        bit_gravity; 
    uint8_t        win_gravity; 
    uint32_t       backing_planes; 
    uint32_t       backing_pixel; 
    uint8_t        save_under; 
    uint8_t        map_is_installed; 
    uint8_t        map_state; 
    uint8_t        override_redirect; 
    xcb_colormap_t colormap; 
    uint32_t       all_event_masks; 
    uint32_t       your_event_mask; 
    uint16_t       do_not_propagate_mask; 
    uint8_t        pad0[2]; 
} xcb_get_window_attributes_reply_t;

#define XCB_DESTROY_WINDOW 4

typedef struct xcb_destroy_window_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_destroy_window_request_t;

#define XCB_DESTROY_SUBWINDOWS 5

typedef struct xcb_destroy_subwindows_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_destroy_subwindows_request_t;

typedef enum xcb_set_mode_t {
    XCB_SET_MODE_INSERT = 0,
    XCB_SET_MODE_DELETE = 1
} xcb_set_mode_t;

#define XCB_CHANGE_SAVE_SET 6

typedef struct xcb_change_save_set_request_t {
    uint8_t      major_opcode; 
    uint8_t      mode; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_change_save_set_request_t;

#define XCB_REPARENT_WINDOW 7

typedef struct xcb_reparent_window_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
    xcb_window_t parent; 
    int16_t      x; 
    int16_t      y; 
} xcb_reparent_window_request_t;

#define XCB_MAP_WINDOW 8

typedef struct xcb_map_window_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_map_window_request_t;

#define XCB_MAP_SUBWINDOWS 9

typedef struct xcb_map_subwindows_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_map_subwindows_request_t;

#define XCB_UNMAP_WINDOW 10

typedef struct xcb_unmap_window_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_unmap_window_request_t;

#define XCB_UNMAP_SUBWINDOWS 11

typedef struct xcb_unmap_subwindows_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_unmap_subwindows_request_t;

typedef enum xcb_config_window_t {
    XCB_CONFIG_WINDOW_X = 1,
    XCB_CONFIG_WINDOW_Y = 2,
    XCB_CONFIG_WINDOW_WIDTH = 4,
    XCB_CONFIG_WINDOW_HEIGHT = 8,
    XCB_CONFIG_WINDOW_BORDER_WIDTH = 16,
    XCB_CONFIG_WINDOW_SIBLING = 32,
    XCB_CONFIG_WINDOW_STACK_MODE = 64
} xcb_config_window_t;

typedef enum xcb_stack_mode_t {
    XCB_STACK_MODE_ABOVE = 0,
    XCB_STACK_MODE_BELOW = 1,
    XCB_STACK_MODE_TOP_IF = 2,
    XCB_STACK_MODE_BOTTOM_IF = 3,
    XCB_STACK_MODE_OPPOSITE = 4
} xcb_stack_mode_t;

#define XCB_CONFIGURE_WINDOW 12

typedef struct xcb_configure_window_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
    uint16_t     value_mask; 
    uint8_t      pad1[2]; 
} xcb_configure_window_request_t;

typedef enum xcb_circulate_t {
    XCB_CIRCULATE_RAISE_LOWEST = 0,
    XCB_CIRCULATE_LOWER_HIGHEST = 1
} xcb_circulate_t;

#define XCB_CIRCULATE_WINDOW 13

typedef struct xcb_circulate_window_request_t {
    uint8_t      major_opcode; 
    uint8_t      direction; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_circulate_window_request_t;

typedef struct xcb_get_geometry_cookie_t {
    unsigned int sequence; 
} xcb_get_geometry_cookie_t;

#define XCB_GET_GEOMETRY 14

typedef struct xcb_get_geometry_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
} xcb_get_geometry_request_t;

typedef struct xcb_get_geometry_reply_t {
    uint8_t      response_type; 
    uint8_t      depth; 
    uint16_t     sequence; 
    uint32_t     length; 
    xcb_window_t root; 
    int16_t      x; 
    int16_t      y; 
    uint16_t     width; 
    uint16_t     height; 
    uint16_t     border_width; 
    uint8_t      pad0[2]; 
} xcb_get_geometry_reply_t;

typedef struct xcb_query_tree_cookie_t {
    unsigned int sequence; 
} xcb_query_tree_cookie_t;

#define XCB_QUERY_TREE 15

typedef struct xcb_query_tree_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_query_tree_request_t;

typedef struct xcb_query_tree_reply_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    uint32_t     length; 
    xcb_window_t root; 
    xcb_window_t parent; 
    uint16_t     children_len; 
    uint8_t      pad1[14]; 
} xcb_query_tree_reply_t;

typedef struct xcb_intern_atom_cookie_t {
    unsigned int sequence; 
} xcb_intern_atom_cookie_t;

#define XCB_INTERN_ATOM 16

typedef struct xcb_intern_atom_request_t {
    uint8_t  major_opcode; 
    uint8_t  only_if_exists; 
    uint16_t length; 
    uint16_t name_len; 
    uint8_t  pad0[2]; 
} xcb_intern_atom_request_t;

typedef struct xcb_intern_atom_reply_t {
    uint8_t    response_type; 
    uint8_t    pad0; 
    uint16_t   sequence; 
    uint32_t   length; 
    xcb_atom_t atom; 
} xcb_intern_atom_reply_t;

typedef struct xcb_get_atom_name_cookie_t {
    unsigned int sequence; 
} xcb_get_atom_name_cookie_t;

#define XCB_GET_ATOM_NAME 17

typedef struct xcb_get_atom_name_request_t {
    uint8_t    major_opcode; 
    uint8_t    pad0; 
    uint16_t   length; 
    xcb_atom_t atom; 
} xcb_get_atom_name_request_t;

typedef struct xcb_get_atom_name_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t name_len; 
    uint8_t  pad1[22]; 
} xcb_get_atom_name_reply_t;

typedef enum xcb_prop_mode_t {
    XCB_PROP_MODE_REPLACE = 0,
    XCB_PROP_MODE_PREPEND = 1,
    XCB_PROP_MODE_APPEND = 2
} xcb_prop_mode_t;

#define XCB_CHANGE_PROPERTY 18

typedef struct xcb_change_property_request_t {
    uint8_t      major_opcode; 
    uint8_t      mode; 
    uint16_t     length; 
    xcb_window_t window; 
    xcb_atom_t   property; 
    xcb_atom_t   type; 
    uint8_t      format; 
    uint8_t      pad0[3]; 
    uint32_t     data_len; 
} xcb_change_property_request_t;

#define XCB_DELETE_PROPERTY 19

typedef struct xcb_delete_property_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
    xcb_atom_t   property; 
} xcb_delete_property_request_t;

typedef enum xcb_get_property_type_t {
    XCB_GET_PROPERTY_TYPE_ANY = 0
} xcb_get_property_type_t;

typedef struct xcb_get_property_cookie_t {
    unsigned int sequence; 
} xcb_get_property_cookie_t;

#define XCB_GET_PROPERTY 20

typedef struct xcb_get_property_request_t {
    uint8_t      major_opcode; 
    uint8_t      _delete; 
    uint16_t     length; 
    xcb_window_t window; 
    xcb_atom_t   property; 
    xcb_atom_t   type; 
    uint32_t     long_offset; 
    uint32_t     long_length; 
} xcb_get_property_request_t;

typedef struct xcb_get_property_reply_t {
    uint8_t    response_type; 
    uint8_t    format; 
    uint16_t   sequence; 
    uint32_t   length; 
    xcb_atom_t type; 
    uint32_t   bytes_after; 
    uint32_t   value_len; 
    uint8_t    pad0[12]; 
} xcb_get_property_reply_t;

typedef struct xcb_list_properties_cookie_t {
    unsigned int sequence; 
} xcb_list_properties_cookie_t;

#define XCB_LIST_PROPERTIES 21

typedef struct xcb_list_properties_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_list_properties_request_t;

typedef struct xcb_list_properties_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t atoms_len; 
    uint8_t  pad1[22]; 
} xcb_list_properties_reply_t;

#define XCB_SET_SELECTION_OWNER 22

typedef struct xcb_set_selection_owner_request_t {
    uint8_t         major_opcode; 
    uint8_t         pad0; 
    uint16_t        length; 
    xcb_window_t    owner; 
    xcb_atom_t      selection; 
    xcb_timestamp_t time; 
} xcb_set_selection_owner_request_t;

typedef struct xcb_get_selection_owner_cookie_t {
    unsigned int sequence; 
} xcb_get_selection_owner_cookie_t;

#define XCB_GET_SELECTION_OWNER 23

typedef struct xcb_get_selection_owner_request_t {
    uint8_t    major_opcode; 
    uint8_t    pad0; 
    uint16_t   length; 
    xcb_atom_t selection; 
} xcb_get_selection_owner_request_t;

typedef struct xcb_get_selection_owner_reply_t {
    uint8_t      response_type; 
    uint8_t      pad0; 
    uint16_t     sequence; 
    uint32_t     length; 
    xcb_window_t owner; 
} xcb_get_selection_owner_reply_t;

#define XCB_CONVERT_SELECTION 24

typedef struct xcb_convert_selection_request_t {
    uint8_t         major_opcode; 
    uint8_t         pad0; 
    uint16_t        length; 
    xcb_window_t    requestor; 
    xcb_atom_t      selection; 
    xcb_atom_t      target; 
    xcb_atom_t      property; 
    xcb_timestamp_t time; 
} xcb_convert_selection_request_t;

typedef enum xcb_send_event_dest_t {
    XCB_SEND_EVENT_DEST_POINTER_WINDOW = 0,
    XCB_SEND_EVENT_DEST_ITEM_FOCUS = 1
} xcb_send_event_dest_t;

#define XCB_SEND_EVENT 25

typedef struct xcb_send_event_request_t {
    uint8_t      major_opcode; 
    uint8_t      propagate; 
    uint16_t     length; 
    xcb_window_t destination; 
    uint32_t     event_mask; 
    char         event[32]; 
} xcb_send_event_request_t;

typedef enum xcb_grab_mode_t {
    XCB_GRAB_MODE_SYNC = 0,
    XCB_GRAB_MODE_ASYNC = 1
} xcb_grab_mode_t;

typedef enum xcb_grab_status_t {
    XCB_GRAB_STATUS_SUCCESS = 0,
    XCB_GRAB_STATUS_ALREADY_GRABBED = 1,
    XCB_GRAB_STATUS_INVALID_TIME = 2,
    XCB_GRAB_STATUS_NOT_VIEWABLE = 3,
    XCB_GRAB_STATUS_FROZEN = 4
} xcb_grab_status_t;

typedef enum xcb_cursor_enum_t {
    XCB_CURSOR_NONE = 0
} xcb_cursor_enum_t;

typedef struct xcb_grab_pointer_cookie_t {
    unsigned int sequence; 
} xcb_grab_pointer_cookie_t;

#define XCB_GRAB_POINTER 26

typedef struct xcb_grab_pointer_request_t {
    uint8_t         major_opcode; 
    uint8_t         owner_events; 
    uint16_t        length; 
    xcb_window_t    grab_window; 
    uint16_t        event_mask; 
    uint8_t         pointer_mode; 
    uint8_t         keyboard_mode; 
    xcb_window_t    confine_to; 
    xcb_cursor_t    cursor; 
    xcb_timestamp_t time; 
} xcb_grab_pointer_request_t;

typedef struct xcb_grab_pointer_reply_t {
    uint8_t  response_type; 
    uint8_t  status; 
    uint16_t sequence; 
    uint32_t length; 
} xcb_grab_pointer_reply_t;

#define XCB_UNGRAB_POINTER 27

typedef struct xcb_ungrab_pointer_request_t {
    uint8_t         major_opcode; 
    uint8_t         pad0; 
    uint16_t        length; 
    xcb_timestamp_t time; 
} xcb_ungrab_pointer_request_t;

typedef enum xcb_button_index_t {
    XCB_BUTTON_INDEX_ANY = 0,
    XCB_BUTTON_INDEX_1 = 1,
    XCB_BUTTON_INDEX_2 = 2,
    XCB_BUTTON_INDEX_3 = 3,
    XCB_BUTTON_INDEX_4 = 4,
    XCB_BUTTON_INDEX_5 = 5
} xcb_button_index_t;

#define XCB_GRAB_BUTTON 28

typedef struct xcb_grab_button_request_t {
    uint8_t      major_opcode; 
    uint8_t      owner_events; 
    uint16_t     length; 
    xcb_window_t grab_window; 
    uint16_t     event_mask; 
    uint8_t      pointer_mode; 
    uint8_t      keyboard_mode; 
    xcb_window_t confine_to; 
    xcb_cursor_t cursor; 
    uint8_t      button; 
    uint8_t      pad0; 
    uint16_t     modifiers; 
} xcb_grab_button_request_t;

#define XCB_UNGRAB_BUTTON 29

typedef struct xcb_ungrab_button_request_t {
    uint8_t      major_opcode; 
    uint8_t      button; 
    uint16_t     length; 
    xcb_window_t grab_window; 
    uint16_t     modifiers; 
    uint8_t      pad0[2]; 
} xcb_ungrab_button_request_t;

#define XCB_CHANGE_ACTIVE_POINTER_GRAB 30

typedef struct xcb_change_active_pointer_grab_request_t {
    uint8_t         major_opcode; 
    uint8_t         pad0; 
    uint16_t        length; 
    xcb_cursor_t    cursor; 
    xcb_timestamp_t time; 
    uint16_t        event_mask; 
    uint8_t         pad1[2]; 
} xcb_change_active_pointer_grab_request_t;

typedef struct xcb_grab_keyboard_cookie_t {
    unsigned int sequence; 
} xcb_grab_keyboard_cookie_t;

#define XCB_GRAB_KEYBOARD 31

typedef struct xcb_grab_keyboard_request_t {
    uint8_t         major_opcode; 
    uint8_t         owner_events; 
    uint16_t        length; 
    xcb_window_t    grab_window; 
    xcb_timestamp_t time; 
    uint8_t         pointer_mode; 
    uint8_t         keyboard_mode; 
    uint8_t         pad0[2]; 
} xcb_grab_keyboard_request_t;

typedef struct xcb_grab_keyboard_reply_t {
    uint8_t  response_type; 
    uint8_t  status; 
    uint16_t sequence; 
    uint32_t length; 
} xcb_grab_keyboard_reply_t;

#define XCB_UNGRAB_KEYBOARD 32

typedef struct xcb_ungrab_keyboard_request_t {
    uint8_t         major_opcode; 
    uint8_t         pad0; 
    uint16_t        length; 
    xcb_timestamp_t time; 
} xcb_ungrab_keyboard_request_t;

typedef enum xcb_grab_t {
    XCB_GRAB_ANY = 0
} xcb_grab_t;

#define XCB_GRAB_KEY 33

typedef struct xcb_grab_key_request_t {
    uint8_t       major_opcode; 
    uint8_t       owner_events; 
    uint16_t      length; 
    xcb_window_t  grab_window; 
    uint16_t      modifiers; 
    xcb_keycode_t key; 
    uint8_t       pointer_mode; 
    uint8_t       keyboard_mode; 
    uint8_t       pad0[3]; 
} xcb_grab_key_request_t;

#define XCB_UNGRAB_KEY 34

typedef struct xcb_ungrab_key_request_t {
    uint8_t       major_opcode; 
    xcb_keycode_t key; 
    uint16_t      length; 
    xcb_window_t  grab_window; 
    uint16_t      modifiers; 
    uint8_t       pad0[2]; 
} xcb_ungrab_key_request_t;

typedef enum xcb_allow_t {
    XCB_ALLOW_ASYNC_POINTER = 0,
    XCB_ALLOW_SYNC_POINTER = 1,
    XCB_ALLOW_REPLAY_POINTER = 2,
    XCB_ALLOW_ASYNC_KEYBOARD = 3,
    XCB_ALLOW_SYNC_KEYBOARD = 4,
    XCB_ALLOW_REPLAY_KEYBOARD = 5,
    XCB_ALLOW_ASYNC_BOTH = 6,
    XCB_ALLOW_SYNC_BOTH = 7
} xcb_allow_t;

#define XCB_ALLOW_EVENTS 35

typedef struct xcb_allow_events_request_t {
    uint8_t         major_opcode; 
    uint8_t         mode; 
    uint16_t        length; 
    xcb_timestamp_t time; 
} xcb_allow_events_request_t;

#define XCB_GRAB_SERVER 36

typedef struct xcb_grab_server_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_grab_server_request_t;

#define XCB_UNGRAB_SERVER 37

typedef struct xcb_ungrab_server_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_ungrab_server_request_t;

typedef struct xcb_query_pointer_cookie_t {
    unsigned int sequence; 
} xcb_query_pointer_cookie_t;

#define XCB_QUERY_POINTER 38

typedef struct xcb_query_pointer_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_query_pointer_request_t;

typedef struct xcb_query_pointer_reply_t {
    uint8_t      response_type; 
    uint8_t      same_screen; 
    uint16_t     sequence; 
    uint32_t     length; 
    xcb_window_t root; 
    xcb_window_t child; 
    int16_t      root_x; 
    int16_t      root_y; 
    int16_t      win_x; 
    int16_t      win_y; 
    uint16_t     mask; 
    uint8_t      pad0[2]; 
} xcb_query_pointer_reply_t;

typedef struct xcb_timecoord_t {
    xcb_timestamp_t time; 
    int16_t         x; 
    int16_t         y; 
} xcb_timecoord_t;

typedef struct xcb_timecoord_iterator_t {
    xcb_timecoord_t *data; 
    int              rem; 
    int              index; 
} xcb_timecoord_iterator_t;

typedef struct xcb_get_motion_events_cookie_t {
    unsigned int sequence; 
} xcb_get_motion_events_cookie_t;

#define XCB_GET_MOTION_EVENTS 39

typedef struct xcb_get_motion_events_request_t {
    uint8_t         major_opcode; 
    uint8_t         pad0; 
    uint16_t        length; 
    xcb_window_t    window; 
    xcb_timestamp_t start; 
    xcb_timestamp_t stop; 
} xcb_get_motion_events_request_t;

typedef struct xcb_get_motion_events_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint32_t events_len; 
    uint8_t  pad1[20]; 
} xcb_get_motion_events_reply_t;

typedef struct xcb_translate_coordinates_cookie_t {
    unsigned int sequence; 
} xcb_translate_coordinates_cookie_t;

#define XCB_TRANSLATE_COORDINATES 40

typedef struct xcb_translate_coordinates_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t src_window; 
    xcb_window_t dst_window; 
    int16_t      src_x; 
    int16_t      src_y; 
} xcb_translate_coordinates_request_t;

typedef struct xcb_translate_coordinates_reply_t {
    uint8_t      response_type; 
    uint8_t      same_screen; 
    uint16_t     sequence; 
    uint32_t     length; 
    xcb_window_t child; 
    int16_t      dst_x; 
    int16_t      dst_y; 
} xcb_translate_coordinates_reply_t;

#define XCB_WARP_POINTER 41

typedef struct xcb_warp_pointer_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t src_window; 
    xcb_window_t dst_window; 
    int16_t      src_x; 
    int16_t      src_y; 
    uint16_t     src_width; 
    uint16_t     src_height; 
    int16_t      dst_x; 
    int16_t      dst_y; 
} xcb_warp_pointer_request_t;

typedef enum xcb_input_focus_t {
    XCB_INPUT_FOCUS_NONE = 0,
    XCB_INPUT_FOCUS_POINTER_ROOT = 1,
    XCB_INPUT_FOCUS_PARENT = 2,
    XCB_INPUT_FOCUS_FOLLOW_KEYBOARD = 3
} xcb_input_focus_t;

#define XCB_SET_INPUT_FOCUS 42

typedef struct xcb_set_input_focus_request_t {
    uint8_t         major_opcode; 
    uint8_t         revert_to; 
    uint16_t        length; 
    xcb_window_t    focus; 
    xcb_timestamp_t time; 
} xcb_set_input_focus_request_t;

typedef struct xcb_get_input_focus_cookie_t {
    unsigned int sequence; 
} xcb_get_input_focus_cookie_t;

#define XCB_GET_INPUT_FOCUS 43

typedef struct xcb_get_input_focus_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_get_input_focus_request_t;

typedef struct xcb_get_input_focus_reply_t {
    uint8_t      response_type; 
    uint8_t      revert_to; 
    uint16_t     sequence; 
    uint32_t     length; 
    xcb_window_t focus; 
} xcb_get_input_focus_reply_t;

typedef struct xcb_query_keymap_cookie_t {
    unsigned int sequence; 
} xcb_query_keymap_cookie_t;

#define XCB_QUERY_KEYMAP 44

typedef struct xcb_query_keymap_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_query_keymap_request_t;

typedef struct xcb_query_keymap_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint8_t  keys[32]; 
} xcb_query_keymap_reply_t;

#define XCB_OPEN_FONT 45

typedef struct xcb_open_font_request_t {
    uint8_t    major_opcode; 
    uint8_t    pad0; 
    uint16_t   length; 
    xcb_font_t fid; 
    uint16_t   name_len; 
    uint8_t    pad1[2]; 
} xcb_open_font_request_t;

#define XCB_CLOSE_FONT 46

typedef struct xcb_close_font_request_t {
    uint8_t    major_opcode; 
    uint8_t    pad0; 
    uint16_t   length; 
    xcb_font_t font; 
} xcb_close_font_request_t;

typedef enum xcb_font_draw_t {
    XCB_FONT_DRAW_LEFT_TO_RIGHT = 0,
    XCB_FONT_DRAW_RIGHT_TO_LEFT = 1
} xcb_font_draw_t;

typedef struct xcb_fontprop_t {
    xcb_atom_t name; 
    uint32_t   value; 
} xcb_fontprop_t;

typedef struct xcb_fontprop_iterator_t {
    xcb_fontprop_t *data; 
    int             rem; 
    int             index; 
} xcb_fontprop_iterator_t;

typedef struct xcb_charinfo_t {
    int16_t  left_side_bearing; 
    int16_t  right_side_bearing; 
    int16_t  character_width; 
    int16_t  ascent; 
    int16_t  descent; 
    uint16_t attributes; 
} xcb_charinfo_t;

typedef struct xcb_charinfo_iterator_t {
    xcb_charinfo_t *data; 
    int             rem; 
    int             index; 
} xcb_charinfo_iterator_t;

typedef struct xcb_query_font_cookie_t {
    unsigned int sequence; 
} xcb_query_font_cookie_t;

#define XCB_QUERY_FONT 47

typedef struct xcb_query_font_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_fontable_t font; 
} xcb_query_font_request_t;

typedef struct xcb_query_font_reply_t {
    uint8_t        response_type; 
    uint8_t        pad0; 
    uint16_t       sequence; 
    uint32_t       length; 
    xcb_charinfo_t min_bounds; 
    uint8_t        pad1[4]; 
    xcb_charinfo_t max_bounds; 
    uint8_t        pad2[4]; 
    uint16_t       min_char_or_byte2; 
    uint16_t       max_char_or_byte2; 
    uint16_t       default_char; 
    uint16_t       properties_len; 
    uint8_t        draw_direction; 
    uint8_t        min_byte1; 
    uint8_t        max_byte1; 
    uint8_t        all_chars_exist; 
    int16_t        font_ascent; 
    int16_t        font_descent; 
    uint32_t       char_infos_len; 
} xcb_query_font_reply_t;

typedef struct xcb_query_text_extents_cookie_t {
    unsigned int sequence; 
} xcb_query_text_extents_cookie_t;

#define XCB_QUERY_TEXT_EXTENTS 48

typedef struct xcb_query_text_extents_request_t {
    uint8_t        major_opcode; 
    uint8_t        odd_length; 
    uint16_t       length; 
    xcb_fontable_t font; 
} xcb_query_text_extents_request_t;

typedef struct xcb_query_text_extents_reply_t {
    uint8_t  response_type; 
    uint8_t  draw_direction; 
    uint16_t sequence; 
    uint32_t length; 
    int16_t  font_ascent; 
    int16_t  font_descent; 
    int16_t  overall_ascent; 
    int16_t  overall_descent; 
    int32_t  overall_width; 
    int32_t  overall_left; 
    int32_t  overall_right; 
} xcb_query_text_extents_reply_t;

typedef struct xcb_str_t {
    uint8_t name_len; 
} xcb_str_t;

typedef struct xcb_str_iterator_t {
    xcb_str_t *data; 
    int        rem; 
    int        index; 
} xcb_str_iterator_t;

typedef struct xcb_list_fonts_cookie_t {
    unsigned int sequence; 
} xcb_list_fonts_cookie_t;

#define XCB_LIST_FONTS 49

typedef struct xcb_list_fonts_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    uint16_t max_names; 
    uint16_t pattern_len; 
} xcb_list_fonts_request_t;

typedef struct xcb_list_fonts_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t names_len; 
    uint8_t  pad1[22]; 
} xcb_list_fonts_reply_t;

typedef struct xcb_list_fonts_with_info_cookie_t {
    unsigned int sequence; 
} xcb_list_fonts_with_info_cookie_t;

#define XCB_LIST_FONTS_WITH_INFO 50

typedef struct xcb_list_fonts_with_info_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    uint16_t max_names; 
    uint16_t pattern_len; 
} xcb_list_fonts_with_info_request_t;

typedef struct xcb_list_fonts_with_info_reply_t {
    uint8_t        response_type; 
    uint8_t        name_len; 
    uint16_t       sequence; 
    uint32_t       length; 
    xcb_charinfo_t min_bounds; 
    uint8_t        pad0[4]; 
    xcb_charinfo_t max_bounds; 
    uint8_t        pad1[4]; 
    uint16_t       min_char_or_byte2; 
    uint16_t       max_char_or_byte2; 
    uint16_t       default_char; 
    uint16_t       properties_len; 
    uint8_t        draw_direction; 
    uint8_t        min_byte1; 
    uint8_t        max_byte1; 
    uint8_t        all_chars_exist; 
    int16_t        font_ascent; 
    int16_t        font_descent; 
    uint32_t       replies_hint; 
} xcb_list_fonts_with_info_reply_t;

#define XCB_SET_FONT_PATH 51

typedef struct xcb_set_font_path_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    uint16_t font_qty; 
    uint8_t  pad1[2]; 
} xcb_set_font_path_request_t;

typedef struct xcb_get_font_path_cookie_t {
    unsigned int sequence; 
} xcb_get_font_path_cookie_t;

#define XCB_GET_FONT_PATH 52

typedef struct xcb_get_font_path_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_get_font_path_request_t;

typedef struct xcb_get_font_path_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t path_len; 
    uint8_t  pad1[22]; 
} xcb_get_font_path_reply_t;

#define XCB_CREATE_PIXMAP 53

typedef struct xcb_create_pixmap_request_t {
    uint8_t        major_opcode; 
    uint8_t        depth; 
    uint16_t       length; 
    xcb_pixmap_t   pid; 
    xcb_drawable_t drawable; 
    uint16_t       width; 
    uint16_t       height; 
} xcb_create_pixmap_request_t;

#define XCB_FREE_PIXMAP 54

typedef struct xcb_free_pixmap_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_pixmap_t pixmap; 
} xcb_free_pixmap_request_t;

typedef enum xcb_gc_t {
    XCB_GC_FUNCTION = 1,
    XCB_GC_PLANE_MASK = 2,
    XCB_GC_FOREGROUND = 4,
    XCB_GC_BACKGROUND = 8,
    XCB_GC_LINE_WIDTH = 16,
    XCB_GC_LINE_STYLE = 32,
    XCB_GC_CAP_STYLE = 64,
    XCB_GC_JOIN_STYLE = 128,
    XCB_GC_FILL_STYLE = 256,
    XCB_GC_FILL_RULE = 512,
    XCB_GC_TILE = 1024,
    XCB_GC_STIPPLE = 2048,
    XCB_GC_TILE_STIPPLE_ORIGIN_X = 4096,
    XCB_GC_TILE_STIPPLE_ORIGIN_Y = 8192,
    XCB_GC_FONT = 16384,
    XCB_GC_SUBWINDOW_MODE = 32768,
    XCB_GC_GRAPHICS_EXPOSURES = 65536,
    XCB_GC_CLIP_ORIGIN_X = 131072,
    XCB_GC_CLIP_ORIGIN_Y = 262144,
    XCB_GC_CLIP_MASK = 524288,
    XCB_GC_DASH_OFFSET = 1048576,
    XCB_GC_DASH_LIST = 2097152,
    XCB_GC_ARC_MODE = 4194304
} xcb_gc_t;

typedef enum xcb_gx_t {
    XCB_GX_CLEAR = 0,
    XCB_GX_AND = 1,
    XCB_GX_AND_REVERSE = 2,
    XCB_GX_COPY = 3,
    XCB_GX_AND_INVERTED = 4,
    XCB_GX_NOOP = 5,
    XCB_GX_XOR = 6,
    XCB_GX_OR = 7,
    XCB_GX_NOR = 8,
    XCB_GX_EQUIV = 9,
    XCB_GX_INVERT = 10,
    XCB_GX_OR_REVERSE = 11,
    XCB_GX_COPY_INVERTED = 12,
    XCB_GX_OR_INVERTED = 13,
    XCB_GX_NAND = 14,
    XCB_GX_SET = 15
} xcb_gx_t;

typedef enum xcb_line_style_t {
    XCB_LINE_STYLE_SOLID = 0,
    XCB_LINE_STYLE_ON_OFF_DASH = 1,
    XCB_LINE_STYLE_DOUBLE_DASH = 2
} xcb_line_style_t;

typedef enum xcb_cap_style_t {
    XCB_CAP_STYLE_NOT_LAST = 0,
    XCB_CAP_STYLE_BUTT = 1,
    XCB_CAP_STYLE_ROUND = 2,
    XCB_CAP_STYLE_PROJECTING = 3
} xcb_cap_style_t;

typedef enum xcb_join_style_t {
    XCB_JOIN_STYLE_MITER = 0,
    XCB_JOIN_STYLE_ROUND = 1,
    XCB_JOIN_STYLE_BEVEL = 2
} xcb_join_style_t;

typedef enum xcb_fill_style_t {
    XCB_FILL_STYLE_SOLID = 0,
    XCB_FILL_STYLE_TILED = 1,
    XCB_FILL_STYLE_STIPPLED = 2,
    XCB_FILL_STYLE_OPAQUE_STIPPLED = 3
} xcb_fill_style_t;

typedef enum xcb_fill_rule_t {
    XCB_FILL_RULE_EVEN_ODD = 0,
    XCB_FILL_RULE_WINDING = 1
} xcb_fill_rule_t;

typedef enum xcb_subwindow_mode_t {
    XCB_SUBWINDOW_MODE_CLIP_BY_CHILDREN = 0,
    XCB_SUBWINDOW_MODE_INCLUDE_INFERIORS = 1
} xcb_subwindow_mode_t;

typedef enum xcb_arc_mode_t {
    XCB_ARC_MODE_CHORD = 0,
    XCB_ARC_MODE_PIE_SLICE = 1
} xcb_arc_mode_t;

#define XCB_CREATE_GC 55

typedef struct xcb_create_gc_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_gcontext_t cid; 
    xcb_drawable_t drawable; 
    uint32_t       value_mask; 
} xcb_create_gc_request_t;

#define XCB_CHANGE_GC 56

typedef struct xcb_change_gc_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_gcontext_t gc; 
    uint32_t       value_mask; 
} xcb_change_gc_request_t;

#define XCB_COPY_GC 57

typedef struct xcb_copy_gc_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_gcontext_t src_gc; 
    xcb_gcontext_t dst_gc; 
    uint32_t       value_mask; 
} xcb_copy_gc_request_t;

#define XCB_SET_DASHES 58

typedef struct xcb_set_dashes_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_gcontext_t gc; 
    uint16_t       dash_offset; 
    uint16_t       dashes_len; 
} xcb_set_dashes_request_t;

typedef enum xcb_clip_ordering_t {
    XCB_CLIP_ORDERING_UNSORTED = 0,
    XCB_CLIP_ORDERING_Y_SORTED = 1,
    XCB_CLIP_ORDERING_YX_SORTED = 2,
    XCB_CLIP_ORDERING_YX_BANDED = 3
} xcb_clip_ordering_t;

#define XCB_SET_CLIP_RECTANGLES 59

typedef struct xcb_set_clip_rectangles_request_t {
    uint8_t        major_opcode; 
    uint8_t        ordering; 
    uint16_t       length; 
    xcb_gcontext_t gc; 
    int16_t        clip_x_origin; 
    int16_t        clip_y_origin; 
} xcb_set_clip_rectangles_request_t;

#define XCB_FREE_GC 60

typedef struct xcb_free_gc_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_gcontext_t gc; 
} xcb_free_gc_request_t;

#define XCB_CLEAR_AREA 61

typedef struct xcb_clear_area_request_t {
    uint8_t      major_opcode; 
    uint8_t      exposures; 
    uint16_t     length; 
    xcb_window_t window; 
    int16_t      x; 
    int16_t      y; 
    uint16_t     width; 
    uint16_t     height; 
} xcb_clear_area_request_t;

#define XCB_COPY_AREA 62

typedef struct xcb_copy_area_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t src_drawable; 
    xcb_drawable_t dst_drawable; 
    xcb_gcontext_t gc; 
    int16_t        src_x; 
    int16_t        src_y; 
    int16_t        dst_x; 
    int16_t        dst_y; 
    uint16_t       width; 
    uint16_t       height; 
} xcb_copy_area_request_t;

#define XCB_COPY_PLANE 63

typedef struct xcb_copy_plane_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t src_drawable; 
    xcb_drawable_t dst_drawable; 
    xcb_gcontext_t gc; 
    int16_t        src_x; 
    int16_t        src_y; 
    int16_t        dst_x; 
    int16_t        dst_y; 
    uint16_t       width; 
    uint16_t       height; 
    uint32_t       bit_plane; 
} xcb_copy_plane_request_t;

typedef enum xcb_coord_mode_t {
    XCB_COORD_MODE_ORIGIN = 0,
    XCB_COORD_MODE_PREVIOUS = 1
} xcb_coord_mode_t;

#define XCB_POLY_POINT 64

typedef struct xcb_poly_point_request_t {
    uint8_t        major_opcode; 
    uint8_t        coordinate_mode; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
} xcb_poly_point_request_t;

#define XCB_POLY_LINE 65

typedef struct xcb_poly_line_request_t {
    uint8_t        major_opcode; 
    uint8_t        coordinate_mode; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
} xcb_poly_line_request_t;

typedef struct xcb_segment_t {
    int16_t x1; 
    int16_t y1; 
    int16_t x2; 
    int16_t y2; 
} xcb_segment_t;

typedef struct xcb_segment_iterator_t {
    xcb_segment_t *data; 
    int            rem; 
    int            index; 
} xcb_segment_iterator_t;

#define XCB_POLY_SEGMENT 66

typedef struct xcb_poly_segment_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
} xcb_poly_segment_request_t;

#define XCB_POLY_RECTANGLE 67

typedef struct xcb_poly_rectangle_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
} xcb_poly_rectangle_request_t;

#define XCB_POLY_ARC 68

typedef struct xcb_poly_arc_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
} xcb_poly_arc_request_t;

typedef enum xcb_poly_shape_t {
    XCB_POLY_SHAPE_COMPLEX = 0,
    XCB_POLY_SHAPE_NONCONVEX = 1,
    XCB_POLY_SHAPE_CONVEX = 2
} xcb_poly_shape_t;

#define XCB_FILL_POLY 69

typedef struct xcb_fill_poly_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
    uint8_t        shape; 
    uint8_t        coordinate_mode; 
    uint8_t        pad1[2]; 
} xcb_fill_poly_request_t;

#define XCB_POLY_FILL_RECTANGLE 70

typedef struct xcb_poly_fill_rectangle_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
} xcb_poly_fill_rectangle_request_t;

#define XCB_POLY_FILL_ARC 71

typedef struct xcb_poly_fill_arc_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
} xcb_poly_fill_arc_request_t;

typedef enum xcb_image_format_t {
    XCB_IMAGE_FORMAT_XY_BITMAP = 0,
    XCB_IMAGE_FORMAT_XY_PIXMAP = 1,
    XCB_IMAGE_FORMAT_Z_PIXMAP = 2
} xcb_image_format_t;

#define XCB_PUT_IMAGE 72

typedef struct xcb_put_image_request_t {
    uint8_t        major_opcode; 
    uint8_t        format; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
    uint16_t       width; 
    uint16_t       height; 
    int16_t        dst_x; 
    int16_t        dst_y; 
    uint8_t        left_pad; 
    uint8_t        depth; 
    uint8_t        pad0[2]; 
} xcb_put_image_request_t;

typedef struct xcb_get_image_cookie_t {
    unsigned int sequence; 
} xcb_get_image_cookie_t;

#define XCB_GET_IMAGE 73

typedef struct xcb_get_image_request_t {
    uint8_t        major_opcode; 
    uint8_t        format; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    int16_t        x; 
    int16_t        y; 
    uint16_t       width; 
    uint16_t       height; 
    uint32_t       plane_mask; 
} xcb_get_image_request_t;

typedef struct xcb_get_image_reply_t {
    uint8_t        response_type; 
    uint8_t        depth; 
    uint16_t       sequence; 
    uint32_t       length; 
    xcb_visualid_t visual; 
    uint8_t        pad0[20]; 
} xcb_get_image_reply_t;

#define XCB_POLY_TEXT_8 74

typedef struct xcb_poly_text_8_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
    int16_t        x; 
    int16_t        y; 
} xcb_poly_text_8_request_t;

#define XCB_POLY_TEXT_16 75

typedef struct xcb_poly_text_16_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
    int16_t        x; 
    int16_t        y; 
} xcb_poly_text_16_request_t;

#define XCB_IMAGE_TEXT_8 76

typedef struct xcb_image_text_8_request_t {
    uint8_t        major_opcode; 
    uint8_t        string_len; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
    int16_t        x; 
    int16_t        y; 
} xcb_image_text_8_request_t;

#define XCB_IMAGE_TEXT_16 77

typedef struct xcb_image_text_16_request_t {
    uint8_t        major_opcode; 
    uint8_t        string_len; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    xcb_gcontext_t gc; 
    int16_t        x; 
    int16_t        y; 
} xcb_image_text_16_request_t;

typedef enum xcb_colormap_alloc_t {
    XCB_COLORMAP_ALLOC_NONE = 0,
    XCB_COLORMAP_ALLOC_ALL = 1
} xcb_colormap_alloc_t;

#define XCB_CREATE_COLORMAP 78

typedef struct xcb_create_colormap_request_t {
    uint8_t        major_opcode; 
    uint8_t        alloc; 
    uint16_t       length; 
    xcb_colormap_t mid; 
    xcb_window_t   window; 
    xcb_visualid_t visual; 
} xcb_create_colormap_request_t;

#define XCB_FREE_COLORMAP 79

typedef struct xcb_free_colormap_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
} xcb_free_colormap_request_t;

#define XCB_COPY_COLORMAP_AND_FREE 80

typedef struct xcb_copy_colormap_and_free_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t mid; 
    xcb_colormap_t src_cmap; 
} xcb_copy_colormap_and_free_request_t;

#define XCB_INSTALL_COLORMAP 81

typedef struct xcb_install_colormap_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
} xcb_install_colormap_request_t;

#define XCB_UNINSTALL_COLORMAP 82

typedef struct xcb_uninstall_colormap_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
} xcb_uninstall_colormap_request_t;

typedef struct xcb_list_installed_colormaps_cookie_t {
    unsigned int sequence; 
} xcb_list_installed_colormaps_cookie_t;

#define XCB_LIST_INSTALLED_COLORMAPS 83

typedef struct xcb_list_installed_colormaps_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
} xcb_list_installed_colormaps_request_t;

typedef struct xcb_list_installed_colormaps_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t cmaps_len; 
    uint8_t  pad1[22]; 
} xcb_list_installed_colormaps_reply_t;

typedef struct xcb_alloc_color_cookie_t {
    unsigned int sequence; 
} xcb_alloc_color_cookie_t;

#define XCB_ALLOC_COLOR 84

typedef struct xcb_alloc_color_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
    uint16_t       red; 
    uint16_t       green; 
    uint16_t       blue; 
    uint8_t        pad1[2]; 
} xcb_alloc_color_request_t;

typedef struct xcb_alloc_color_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t red; 
    uint16_t green; 
    uint16_t blue; 
    uint8_t  pad1[2]; 
    uint32_t pixel; 
} xcb_alloc_color_reply_t;

typedef struct xcb_alloc_named_color_cookie_t {
    unsigned int sequence; 
} xcb_alloc_named_color_cookie_t;

#define XCB_ALLOC_NAMED_COLOR 85

typedef struct xcb_alloc_named_color_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
    uint16_t       name_len; 
    uint8_t        pad1[2]; 
} xcb_alloc_named_color_request_t;

typedef struct xcb_alloc_named_color_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint32_t pixel; 
    uint16_t exact_red; 
    uint16_t exact_green; 
    uint16_t exact_blue; 
    uint16_t visual_red; 
    uint16_t visual_green; 
    uint16_t visual_blue; 
} xcb_alloc_named_color_reply_t;

typedef struct xcb_alloc_color_cells_cookie_t {
    unsigned int sequence; 
} xcb_alloc_color_cells_cookie_t;

#define XCB_ALLOC_COLOR_CELLS 86

typedef struct xcb_alloc_color_cells_request_t {
    uint8_t        major_opcode; 
    uint8_t        contiguous; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
    uint16_t       colors; 
    uint16_t       planes; 
} xcb_alloc_color_cells_request_t;

typedef struct xcb_alloc_color_cells_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t pixels_len; 
    uint16_t masks_len; 
    uint8_t  pad1[20]; 
} xcb_alloc_color_cells_reply_t;

typedef struct xcb_alloc_color_planes_cookie_t {
    unsigned int sequence; 
} xcb_alloc_color_planes_cookie_t;

#define XCB_ALLOC_COLOR_PLANES 87

typedef struct xcb_alloc_color_planes_request_t {
    uint8_t        major_opcode; 
    uint8_t        contiguous; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
    uint16_t       colors; 
    uint16_t       reds; 
    uint16_t       greens; 
    uint16_t       blues; 
} xcb_alloc_color_planes_request_t;

typedef struct xcb_alloc_color_planes_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t pixels_len; 
    uint8_t  pad1[2]; 
    uint32_t red_mask; 
    uint32_t green_mask; 
    uint32_t blue_mask; 
    uint8_t  pad2[8]; 
} xcb_alloc_color_planes_reply_t;

#define XCB_FREE_COLORS 88

typedef struct xcb_free_colors_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
    uint32_t       plane_mask; 
} xcb_free_colors_request_t;

typedef enum xcb_color_flag_t {
    XCB_COLOR_FLAG_RED = 1,
    XCB_COLOR_FLAG_GREEN = 2,
    XCB_COLOR_FLAG_BLUE = 4
} xcb_color_flag_t;

typedef struct xcb_coloritem_t {
    uint32_t pixel; 
    uint16_t red; 
    uint16_t green; 
    uint16_t blue; 
    uint8_t  flags; 
    uint8_t  pad0; 
} xcb_coloritem_t;

typedef struct xcb_coloritem_iterator_t {
    xcb_coloritem_t *data; 
    int              rem; 
    int              index; 
} xcb_coloritem_iterator_t;

#define XCB_STORE_COLORS 89

typedef struct xcb_store_colors_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
} xcb_store_colors_request_t;

#define XCB_STORE_NAMED_COLOR 90

typedef struct xcb_store_named_color_request_t {
    uint8_t        major_opcode; 
    uint8_t        flags; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
    uint32_t       pixel; 
    uint16_t       name_len; 
    uint8_t        pad0[2]; 
} xcb_store_named_color_request_t;

typedef struct xcb_rgb_t {
    uint16_t red; 
    uint16_t green; 
    uint16_t blue; 
    uint8_t  pad0[2]; 
} xcb_rgb_t;

typedef struct xcb_rgb_iterator_t {
    xcb_rgb_t *data; 
    int        rem; 
    int        index; 
} xcb_rgb_iterator_t;

typedef struct xcb_query_colors_cookie_t {
    unsigned int sequence; 
} xcb_query_colors_cookie_t;

#define XCB_QUERY_COLORS 91

typedef struct xcb_query_colors_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
} xcb_query_colors_request_t;

typedef struct xcb_query_colors_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t colors_len; 
    uint8_t  pad1[22]; 
} xcb_query_colors_reply_t;

typedef struct xcb_lookup_color_cookie_t {
    unsigned int sequence; 
} xcb_lookup_color_cookie_t;

#define XCB_LOOKUP_COLOR 92

typedef struct xcb_lookup_color_request_t {
    uint8_t        major_opcode; 
    uint8_t        pad0; 
    uint16_t       length; 
    xcb_colormap_t cmap; 
    uint16_t       name_len; 
    uint8_t        pad1[2]; 
} xcb_lookup_color_request_t;

typedef struct xcb_lookup_color_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t exact_red; 
    uint16_t exact_green; 
    uint16_t exact_blue; 
    uint16_t visual_red; 
    uint16_t visual_green; 
    uint16_t visual_blue; 
} xcb_lookup_color_reply_t;

typedef enum xcb_pixmap_enum_t {
    XCB_PIXMAP_NONE = 0
} xcb_pixmap_enum_t;

#define XCB_CREATE_CURSOR 93

typedef struct xcb_create_cursor_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_cursor_t cid; 
    xcb_pixmap_t source; 
    xcb_pixmap_t mask; 
    uint16_t     fore_red; 
    uint16_t     fore_green; 
    uint16_t     fore_blue; 
    uint16_t     back_red; 
    uint16_t     back_green; 
    uint16_t     back_blue; 
    uint16_t     x; 
    uint16_t     y; 
} xcb_create_cursor_request_t;

typedef enum xcb_font_enum_t {
    XCB_FONT_NONE = 0
} xcb_font_enum_t;

#define XCB_CREATE_GLYPH_CURSOR 94

typedef struct xcb_create_glyph_cursor_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_cursor_t cid; 
    xcb_font_t   source_font; 
    xcb_font_t   mask_font; 
    uint16_t     source_char; 
    uint16_t     mask_char; 
    uint16_t     fore_red; 
    uint16_t     fore_green; 
    uint16_t     fore_blue; 
    uint16_t     back_red; 
    uint16_t     back_green; 
    uint16_t     back_blue; 
} xcb_create_glyph_cursor_request_t;

#define XCB_FREE_CURSOR 95

typedef struct xcb_free_cursor_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_cursor_t cursor; 
} xcb_free_cursor_request_t;

#define XCB_RECOLOR_CURSOR 96

typedef struct xcb_recolor_cursor_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_cursor_t cursor; 
    uint16_t     fore_red; 
    uint16_t     fore_green; 
    uint16_t     fore_blue; 
    uint16_t     back_red; 
    uint16_t     back_green; 
    uint16_t     back_blue; 
} xcb_recolor_cursor_request_t;

typedef enum xcb_query_shape_of_t {
    XCB_QUERY_SHAPE_OF_LARGEST_CURSOR = 0,
    XCB_QUERY_SHAPE_OF_FASTEST_TILE = 1,
    XCB_QUERY_SHAPE_OF_FASTEST_STIPPLE = 2
} xcb_query_shape_of_t;

typedef struct xcb_query_best_size_cookie_t {
    unsigned int sequence; 
} xcb_query_best_size_cookie_t;

#define XCB_QUERY_BEST_SIZE 97

typedef struct xcb_query_best_size_request_t {
    uint8_t        major_opcode; 
    uint8_t        _class; 
    uint16_t       length; 
    xcb_drawable_t drawable; 
    uint16_t       width; 
    uint16_t       height; 
} xcb_query_best_size_request_t;

typedef struct xcb_query_best_size_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t width; 
    uint16_t height; 
} xcb_query_best_size_reply_t;

typedef struct xcb_query_extension_cookie_t {
    unsigned int sequence; 
} xcb_query_extension_cookie_t;

#define XCB_QUERY_EXTENSION 98

typedef struct xcb_query_extension_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    uint16_t name_len; 
    uint8_t  pad1[2]; 
} xcb_query_extension_request_t;

typedef struct xcb_query_extension_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint8_t  present; 
    uint8_t  major_opcode; 
    uint8_t  first_event; 
    uint8_t  first_error; 
} xcb_query_extension_reply_t;

typedef struct xcb_list_extensions_cookie_t {
    unsigned int sequence; 
} xcb_list_extensions_cookie_t;

#define XCB_LIST_EXTENSIONS 99

typedef struct xcb_list_extensions_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_list_extensions_request_t;

typedef struct xcb_list_extensions_reply_t {
    uint8_t  response_type; 
    uint8_t  names_len; 
    uint16_t sequence; 
    uint32_t length; 
    uint8_t  pad0[24]; 
} xcb_list_extensions_reply_t;

#define XCB_CHANGE_KEYBOARD_MAPPING 100

typedef struct xcb_change_keyboard_mapping_request_t {
    uint8_t       major_opcode; 
    uint8_t       keycode_count; 
    uint16_t      length; 
    xcb_keycode_t first_keycode; 
    uint8_t       keysyms_per_keycode; 
    uint8_t       pad0[2]; 
} xcb_change_keyboard_mapping_request_t;

typedef struct xcb_get_keyboard_mapping_cookie_t {
    unsigned int sequence; 
} xcb_get_keyboard_mapping_cookie_t;

#define XCB_GET_KEYBOARD_MAPPING 101

typedef struct xcb_get_keyboard_mapping_request_t {
    uint8_t       major_opcode; 
    uint8_t       pad0; 
    uint16_t      length; 
    xcb_keycode_t first_keycode; 
    uint8_t       count; 
} xcb_get_keyboard_mapping_request_t;

typedef struct xcb_get_keyboard_mapping_reply_t {
    uint8_t  response_type; 
    uint8_t  keysyms_per_keycode; 
    uint16_t sequence; 
    uint32_t length; 
    uint8_t  pad0[24]; 
} xcb_get_keyboard_mapping_reply_t;

typedef enum xcb_kb_t {
    XCB_KB_KEY_CLICK_PERCENT = 1,
    XCB_KB_BELL_PERCENT = 2,
    XCB_KB_BELL_PITCH = 4,
    XCB_KB_BELL_DURATION = 8,
    XCB_KB_LED = 16,
    XCB_KB_LED_MODE = 32,
    XCB_KB_KEY = 64,
    XCB_KB_AUTO_REPEAT_MODE = 128
} xcb_kb_t;

typedef enum xcb_led_mode_t {
    XCB_LED_MODE_OFF = 0,
    XCB_LED_MODE_ON = 1
} xcb_led_mode_t;

typedef enum xcb_auto_repeat_mode_t {
    XCB_AUTO_REPEAT_MODE_OFF = 0,
    XCB_AUTO_REPEAT_MODE_ON = 1,
    XCB_AUTO_REPEAT_MODE_DEFAULT = 2
} xcb_auto_repeat_mode_t;

#define XCB_CHANGE_KEYBOARD_CONTROL 102

typedef struct xcb_change_keyboard_control_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    uint32_t value_mask; 
} xcb_change_keyboard_control_request_t;

typedef struct xcb_get_keyboard_control_cookie_t {
    unsigned int sequence; 
} xcb_get_keyboard_control_cookie_t;

#define XCB_GET_KEYBOARD_CONTROL 103

typedef struct xcb_get_keyboard_control_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_get_keyboard_control_request_t;

typedef struct xcb_get_keyboard_control_reply_t {
    uint8_t  response_type; 
    uint8_t  global_auto_repeat; 
    uint16_t sequence; 
    uint32_t length; 
    uint32_t led_mask; 
    uint8_t  key_click_percent; 
    uint8_t  bell_percent; 
    uint16_t bell_pitch; 
    uint16_t bell_duration; 
    uint8_t  pad0[2]; 
    uint8_t  auto_repeats[32]; 
} xcb_get_keyboard_control_reply_t;

#define XCB_BELL 104

typedef struct xcb_bell_request_t {
    uint8_t  major_opcode; 
    int8_t   percent; 
    uint16_t length; 
} xcb_bell_request_t;

#define XCB_CHANGE_POINTER_CONTROL 105

typedef struct xcb_change_pointer_control_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    int16_t  acceleration_numerator; 
    int16_t  acceleration_denominator; 
    int16_t  threshold; 
    uint8_t  do_acceleration; 
    uint8_t  do_threshold; 
} xcb_change_pointer_control_request_t;

typedef struct xcb_get_pointer_control_cookie_t {
    unsigned int sequence; 
} xcb_get_pointer_control_cookie_t;

#define XCB_GET_POINTER_CONTROL 106

typedef struct xcb_get_pointer_control_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_get_pointer_control_request_t;

typedef struct xcb_get_pointer_control_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t acceleration_numerator; 
    uint16_t acceleration_denominator; 
    uint16_t threshold; 
    uint8_t  pad1[18]; 
} xcb_get_pointer_control_reply_t;

typedef enum xcb_blanking_t {
    XCB_BLANKING_NOT_PREFERRED = 0,
    XCB_BLANKING_PREFERRED = 1,
    XCB_BLANKING_DEFAULT = 2
} xcb_blanking_t;

typedef enum xcb_exposures_t {
    XCB_EXPOSURES_NOT_ALLOWED = 0,
    XCB_EXPOSURES_ALLOWED = 1,
    XCB_EXPOSURES_DEFAULT = 2
} xcb_exposures_t;

#define XCB_SET_SCREEN_SAVER 107

typedef struct xcb_set_screen_saver_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    int16_t  timeout; 
    int16_t  interval; 
    uint8_t  prefer_blanking; 
    uint8_t  allow_exposures; 
} xcb_set_screen_saver_request_t;

typedef struct xcb_get_screen_saver_cookie_t {
    unsigned int sequence; 
} xcb_get_screen_saver_cookie_t;

#define XCB_GET_SCREEN_SAVER 108

typedef struct xcb_get_screen_saver_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_get_screen_saver_request_t;

typedef struct xcb_get_screen_saver_reply_t {
    uint8_t  response_type; 
    uint8_t  pad0; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t timeout; 
    uint16_t interval; 
    uint8_t  prefer_blanking; 
    uint8_t  allow_exposures; 
    uint8_t  pad1[18]; 
} xcb_get_screen_saver_reply_t;

typedef enum xcb_host_mode_t {
    XCB_HOST_MODE_INSERT = 0,
    XCB_HOST_MODE_DELETE = 1
} xcb_host_mode_t;

typedef enum xcb_family_t {
    XCB_FAMILY_INTERNET = 0,
    XCB_FAMILY_DECNET = 1,
    XCB_FAMILY_CHAOS = 2,
    XCB_FAMILY_SERVER_INTERPRETED = 5,
    XCB_FAMILY_INTERNET_6 = 6
} xcb_family_t;

#define XCB_CHANGE_HOSTS 109

typedef struct xcb_change_hosts_request_t {
    uint8_t  major_opcode; 
    uint8_t  mode; 
    uint16_t length; 
    uint8_t  family; 
    uint8_t  pad0; 
    uint16_t address_len; 
} xcb_change_hosts_request_t;

typedef struct xcb_host_t {
    uint8_t  family; 
    uint8_t  pad0; 
    uint16_t address_len; 
} xcb_host_t;

typedef struct xcb_host_iterator_t {
    xcb_host_t *data; 
    int         rem; 
    int         index; 
} xcb_host_iterator_t;

typedef struct xcb_list_hosts_cookie_t {
    unsigned int sequence; 
} xcb_list_hosts_cookie_t;

#define XCB_LIST_HOSTS 110

typedef struct xcb_list_hosts_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_list_hosts_request_t;

typedef struct xcb_list_hosts_reply_t {
    uint8_t  response_type; 
    uint8_t  mode; 
    uint16_t sequence; 
    uint32_t length; 
    uint16_t hosts_len; 
    uint8_t  pad0[22]; 
} xcb_list_hosts_reply_t;

typedef enum xcb_access_control_t {
    XCB_ACCESS_CONTROL_DISABLE = 0,
    XCB_ACCESS_CONTROL_ENABLE = 1
} xcb_access_control_t;

#define XCB_SET_ACCESS_CONTROL 111

typedef struct xcb_set_access_control_request_t {
    uint8_t  major_opcode; 
    uint8_t  mode; 
    uint16_t length; 
} xcb_set_access_control_request_t;

typedef enum xcb_close_down_t {
    XCB_CLOSE_DOWN_DESTROY_ALL = 0,
    XCB_CLOSE_DOWN_RETAIN_PERMANENT = 1,
    XCB_CLOSE_DOWN_RETAIN_TEMPORARY = 2
} xcb_close_down_t;

#define XCB_SET_CLOSE_DOWN_MODE 112

typedef struct xcb_set_close_down_mode_request_t {
    uint8_t  major_opcode; 
    uint8_t  mode; 
    uint16_t length; 
} xcb_set_close_down_mode_request_t;

typedef enum xcb_kill_t {
    XCB_KILL_ALL_TEMPORARY = 0
} xcb_kill_t;

#define XCB_KILL_CLIENT 113

typedef struct xcb_kill_client_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
    uint32_t resource; 
} xcb_kill_client_request_t;

#define XCB_ROTATE_PROPERTIES 114

typedef struct xcb_rotate_properties_request_t {
    uint8_t      major_opcode; 
    uint8_t      pad0; 
    uint16_t     length; 
    xcb_window_t window; 
    uint16_t     atoms_len; 
    int16_t      delta; 
} xcb_rotate_properties_request_t;

typedef enum xcb_screen_saver_t {
    XCB_SCREEN_SAVER_RESET = 0,
    XCB_SCREEN_SAVER_ACTIVE = 1
} xcb_screen_saver_t;

#define XCB_FORCE_SCREEN_SAVER 115

typedef struct xcb_force_screen_saver_request_t {
    uint8_t  major_opcode; 
    uint8_t  mode; 
    uint16_t length; 
} xcb_force_screen_saver_request_t;

typedef enum xcb_mapping_status_t {
    XCB_MAPPING_STATUS_SUCCESS = 0,
    XCB_MAPPING_STATUS_BUSY = 1,
    XCB_MAPPING_STATUS_FAILURE = 2
} xcb_mapping_status_t;

typedef struct xcb_set_pointer_mapping_cookie_t {
    unsigned int sequence; 
} xcb_set_pointer_mapping_cookie_t;

#define XCB_SET_POINTER_MAPPING 116

typedef struct xcb_set_pointer_mapping_request_t {
    uint8_t  major_opcode; 
    uint8_t  map_len; 
    uint16_t length; 
} xcb_set_pointer_mapping_request_t;

typedef struct xcb_set_pointer_mapping_reply_t {
    uint8_t  response_type; 
    uint8_t  status; 
    uint16_t sequence; 
    uint32_t length; 
} xcb_set_pointer_mapping_reply_t;

typedef struct xcb_get_pointer_mapping_cookie_t {
    unsigned int sequence; 
} xcb_get_pointer_mapping_cookie_t;

#define XCB_GET_POINTER_MAPPING 117

typedef struct xcb_get_pointer_mapping_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_get_pointer_mapping_request_t;

typedef struct xcb_get_pointer_mapping_reply_t {
    uint8_t  response_type; 
    uint8_t  map_len; 
    uint16_t sequence; 
    uint32_t length; 
    uint8_t  pad0[24]; 
} xcb_get_pointer_mapping_reply_t;

typedef enum xcb_map_index_t {
    XCB_MAP_INDEX_SHIFT = 0,
    XCB_MAP_INDEX_LOCK = 1,
    XCB_MAP_INDEX_CONTROL = 2,
    XCB_MAP_INDEX_1 = 3,
    XCB_MAP_INDEX_2 = 4,
    XCB_MAP_INDEX_3 = 5,
    XCB_MAP_INDEX_4 = 6,
    XCB_MAP_INDEX_5 = 7
} xcb_map_index_t;

typedef struct xcb_set_modifier_mapping_cookie_t {
    unsigned int sequence; 
} xcb_set_modifier_mapping_cookie_t;

#define XCB_SET_MODIFIER_MAPPING 118

typedef struct xcb_set_modifier_mapping_request_t {
    uint8_t  major_opcode; 
    uint8_t  keycodes_per_modifier; 
    uint16_t length; 
} xcb_set_modifier_mapping_request_t;

typedef struct xcb_set_modifier_mapping_reply_t {
    uint8_t  response_type; 
    uint8_t  status; 
    uint16_t sequence; 
    uint32_t length; 
} xcb_set_modifier_mapping_reply_t;

typedef struct xcb_get_modifier_mapping_cookie_t {
    unsigned int sequence; 
} xcb_get_modifier_mapping_cookie_t;

#define XCB_GET_MODIFIER_MAPPING 119

typedef struct xcb_get_modifier_mapping_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_get_modifier_mapping_request_t;

typedef struct xcb_get_modifier_mapping_reply_t {
    uint8_t  response_type; 
    uint8_t  keycodes_per_modifier; 
    uint16_t sequence; 
    uint32_t length; 
    uint8_t  pad0[24]; 
} xcb_get_modifier_mapping_reply_t;

#define XCB_NO_OPERATION 127

typedef struct xcb_no_operation_request_t {
    uint8_t  major_opcode; 
    uint8_t  pad0; 
    uint16_t length; 
} xcb_no_operation_request_t;

void
xcb_char2b_next (xcb_char2b_iterator_t *i  );

xcb_generic_iterator_t
xcb_char2b_end (xcb_char2b_iterator_t i  );

void
xcb_window_next (xcb_window_iterator_t *i  );

xcb_generic_iterator_t
xcb_window_end (xcb_window_iterator_t i  );

void
xcb_pixmap_next (xcb_pixmap_iterator_t *i  );

xcb_generic_iterator_t
xcb_pixmap_end (xcb_pixmap_iterator_t i  );

void
xcb_cursor_next (xcb_cursor_iterator_t *i  );

xcb_generic_iterator_t
xcb_cursor_end (xcb_cursor_iterator_t i  );

void
xcb_font_next (xcb_font_iterator_t *i  );

xcb_generic_iterator_t
xcb_font_end (xcb_font_iterator_t i  );

void
xcb_gcontext_next (xcb_gcontext_iterator_t *i  );

xcb_generic_iterator_t
xcb_gcontext_end (xcb_gcontext_iterator_t i  );

void
xcb_colormap_next (xcb_colormap_iterator_t *i  );

xcb_generic_iterator_t
xcb_colormap_end (xcb_colormap_iterator_t i  );

void
xcb_atom_next (xcb_atom_iterator_t *i  );

xcb_generic_iterator_t
xcb_atom_end (xcb_atom_iterator_t i  );

void
xcb_drawable_next (xcb_drawable_iterator_t *i  );

xcb_generic_iterator_t
xcb_drawable_end (xcb_drawable_iterator_t i  );

void
xcb_fontable_next (xcb_fontable_iterator_t *i  );

xcb_generic_iterator_t
xcb_fontable_end (xcb_fontable_iterator_t i  );

void
xcb_visualid_next (xcb_visualid_iterator_t *i  );

xcb_generic_iterator_t
xcb_visualid_end (xcb_visualid_iterator_t i  );

void
xcb_timestamp_next (xcb_timestamp_iterator_t *i  );

xcb_generic_iterator_t
xcb_timestamp_end (xcb_timestamp_iterator_t i  );

void
xcb_keysym_next (xcb_keysym_iterator_t *i  );

xcb_generic_iterator_t
xcb_keysym_end (xcb_keysym_iterator_t i  );

void
xcb_keycode_next (xcb_keycode_iterator_t *i  );

xcb_generic_iterator_t
xcb_keycode_end (xcb_keycode_iterator_t i  );

void
xcb_button_next (xcb_button_iterator_t *i  );

xcb_generic_iterator_t
xcb_button_end (xcb_button_iterator_t i  );

void
xcb_point_next (xcb_point_iterator_t *i  );

xcb_generic_iterator_t
xcb_point_end (xcb_point_iterator_t i  );

void
xcb_rectangle_next (xcb_rectangle_iterator_t *i  );

xcb_generic_iterator_t
xcb_rectangle_end (xcb_rectangle_iterator_t i  );

void
xcb_arc_next (xcb_arc_iterator_t *i  );

xcb_generic_iterator_t
xcb_arc_end (xcb_arc_iterator_t i  );

void
xcb_format_next (xcb_format_iterator_t *i  );

xcb_generic_iterator_t
xcb_format_end (xcb_format_iterator_t i  );

void
xcb_visualtype_next (xcb_visualtype_iterator_t *i  );

xcb_generic_iterator_t
xcb_visualtype_end (xcb_visualtype_iterator_t i  );

int
xcb_depth_sizeof (const void  *_buffer  );

xcb_visualtype_t *
xcb_depth_visuals (const xcb_depth_t *R  );

int
xcb_depth_visuals_length (const xcb_depth_t *R  );

xcb_visualtype_iterator_t
xcb_depth_visuals_iterator (const xcb_depth_t *R  );

void
xcb_depth_next (xcb_depth_iterator_t *i  );

xcb_generic_iterator_t
xcb_depth_end (xcb_depth_iterator_t i  );

int
xcb_screen_sizeof (const void  *_buffer  );

int
xcb_screen_allowed_depths_length (const xcb_screen_t *R  );

xcb_depth_iterator_t
xcb_screen_allowed_depths_iterator (const xcb_screen_t *R  );

void
xcb_screen_next (xcb_screen_iterator_t *i  );

xcb_generic_iterator_t
xcb_screen_end (xcb_screen_iterator_t i  );

int
xcb_setup_request_sizeof (const void  *_buffer  );

char *
xcb_setup_request_authorization_protocol_name (const xcb_setup_request_t *R  );

int
xcb_setup_request_authorization_protocol_name_length (const xcb_setup_request_t *R  );

xcb_generic_iterator_t
xcb_setup_request_authorization_protocol_name_end (const xcb_setup_request_t *R  );

char *
xcb_setup_request_authorization_protocol_data (const xcb_setup_request_t *R  );

int
xcb_setup_request_authorization_protocol_data_length (const xcb_setup_request_t *R  );

xcb_generic_iterator_t
xcb_setup_request_authorization_protocol_data_end (const xcb_setup_request_t *R  );

void
xcb_setup_request_next (xcb_setup_request_iterator_t *i  );

xcb_generic_iterator_t
xcb_setup_request_end (xcb_setup_request_iterator_t i  );

int
xcb_setup_failed_sizeof (const void  *_buffer  );

char *
xcb_setup_failed_reason (const xcb_setup_failed_t *R  );

int
xcb_setup_failed_reason_length (const xcb_setup_failed_t *R  );

xcb_generic_iterator_t
xcb_setup_failed_reason_end (const xcb_setup_failed_t *R  );

void
xcb_setup_failed_next (xcb_setup_failed_iterator_t *i  );

xcb_generic_iterator_t
xcb_setup_failed_end (xcb_setup_failed_iterator_t i  );

int
xcb_setup_authenticate_sizeof (const void  *_buffer  );

char *
xcb_setup_authenticate_reason (const xcb_setup_authenticate_t *R  );

int
xcb_setup_authenticate_reason_length (const xcb_setup_authenticate_t *R  );

xcb_generic_iterator_t
xcb_setup_authenticate_reason_end (const xcb_setup_authenticate_t *R  );

void
xcb_setup_authenticate_next (xcb_setup_authenticate_iterator_t *i  );

xcb_generic_iterator_t
xcb_setup_authenticate_end (xcb_setup_authenticate_iterator_t i  );

int
xcb_setup_sizeof (const void  *_buffer  );

char *
xcb_setup_vendor (const xcb_setup_t *R  );

int
xcb_setup_vendor_length (const xcb_setup_t *R  );

xcb_generic_iterator_t
xcb_setup_vendor_end (const xcb_setup_t *R  );

xcb_format_t *
xcb_setup_pixmap_formats (const xcb_setup_t *R  );

int
xcb_setup_pixmap_formats_length (const xcb_setup_t *R  );

xcb_format_iterator_t
xcb_setup_pixmap_formats_iterator (const xcb_setup_t *R  );

int
xcb_setup_roots_length (const xcb_setup_t *R  );

xcb_screen_iterator_t
xcb_setup_roots_iterator (const xcb_setup_t *R  );

void
xcb_setup_next (xcb_setup_iterator_t *i  );

xcb_generic_iterator_t
xcb_setup_end (xcb_setup_iterator_t i  );

void
xcb_client_message_data_next (xcb_client_message_data_iterator_t *i  );

xcb_generic_iterator_t
xcb_client_message_data_end (xcb_client_message_data_iterator_t i  );

int
xcb_create_window_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_create_window_checked (xcb_connection_t *c  ,
                           uint8_t           depth  ,
                           xcb_window_t      wid  ,
                           xcb_window_t      parent  ,
                           int16_t           x  ,
                           int16_t           y  ,
                           uint16_t          width  ,
                           uint16_t          height  ,
                           uint16_t          border_width  ,
                           uint16_t          _class  ,
                           xcb_visualid_t    visual  ,
                           uint32_t          value_mask  ,
                           const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_create_window (xcb_connection_t *c  ,
                   uint8_t           depth  ,
                   xcb_window_t      wid  ,
                   xcb_window_t      parent  ,
                   int16_t           x  ,
                   int16_t           y  ,
                   uint16_t          width  ,
                   uint16_t          height  ,
                   uint16_t          border_width  ,
                   uint16_t          _class  ,
                   xcb_visualid_t    visual  ,
                   uint32_t          value_mask  ,
                   const uint32_t   *value_list  );

int
xcb_change_window_attributes_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_change_window_attributes_checked (xcb_connection_t *c  ,
                                      xcb_window_t      window  ,
                                      uint32_t          value_mask  ,
                                      const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_change_window_attributes (xcb_connection_t *c  ,
                              xcb_window_t      window  ,
                              uint32_t          value_mask  ,
                              const uint32_t   *value_list  );

xcb_get_window_attributes_cookie_t
xcb_get_window_attributes (xcb_connection_t *c  ,
                           xcb_window_t      window  );

xcb_get_window_attributes_cookie_t
xcb_get_window_attributes_unchecked (xcb_connection_t *c  ,
                                     xcb_window_t      window  );

xcb_get_window_attributes_reply_t *
xcb_get_window_attributes_reply (xcb_connection_t                    *c  ,
                                 xcb_get_window_attributes_cookie_t   cookie  ,
                                 xcb_generic_error_t                **e  );

xcb_void_cookie_t
xcb_destroy_window_checked (xcb_connection_t *c  ,
                            xcb_window_t      window  );

xcb_void_cookie_t
xcb_destroy_window (xcb_connection_t *c  ,
                    xcb_window_t      window  );

xcb_void_cookie_t
xcb_destroy_subwindows_checked (xcb_connection_t *c  ,
                                xcb_window_t      window  );

xcb_void_cookie_t
xcb_destroy_subwindows (xcb_connection_t *c  ,
                        xcb_window_t      window  );

xcb_void_cookie_t
xcb_change_save_set_checked (xcb_connection_t *c  ,
                             uint8_t           mode  ,
                             xcb_window_t      window  );

xcb_void_cookie_t
xcb_change_save_set (xcb_connection_t *c  ,
                     uint8_t           mode  ,
                     xcb_window_t      window  );

xcb_void_cookie_t
xcb_reparent_window_checked (xcb_connection_t *c  ,
                             xcb_window_t      window  ,
                             xcb_window_t      parent  ,
                             int16_t           x  ,
                             int16_t           y  );

xcb_void_cookie_t
xcb_reparent_window (xcb_connection_t *c  ,
                     xcb_window_t      window  ,
                     xcb_window_t      parent  ,
                     int16_t           x  ,
                     int16_t           y  );

xcb_void_cookie_t
xcb_map_window_checked (xcb_connection_t *c  ,
                        xcb_window_t      window  );

xcb_void_cookie_t
xcb_map_window (xcb_connection_t *c  ,
                xcb_window_t      window  );

xcb_void_cookie_t
xcb_map_subwindows_checked (xcb_connection_t *c  ,
                            xcb_window_t      window  );

xcb_void_cookie_t
xcb_map_subwindows (xcb_connection_t *c  ,
                    xcb_window_t      window  );

xcb_void_cookie_t
xcb_unmap_window_checked (xcb_connection_t *c  ,
                          xcb_window_t      window  );

xcb_void_cookie_t
xcb_unmap_window (xcb_connection_t *c  ,
                  xcb_window_t      window  );

xcb_void_cookie_t
xcb_unmap_subwindows_checked (xcb_connection_t *c  ,
                              xcb_window_t      window  );

xcb_void_cookie_t
xcb_unmap_subwindows (xcb_connection_t *c  ,
                      xcb_window_t      window  );

int
xcb_configure_window_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_configure_window_checked (xcb_connection_t *c  ,
                              xcb_window_t      window  ,
                              uint16_t          value_mask  ,
                              const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_configure_window (xcb_connection_t *c  ,
                      xcb_window_t      window  ,
                      uint16_t          value_mask  ,
                      const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_circulate_window_checked (xcb_connection_t *c  ,
                              uint8_t           direction  ,
                              xcb_window_t      window  );

xcb_void_cookie_t
xcb_circulate_window (xcb_connection_t *c  ,
                      uint8_t           direction  ,
                      xcb_window_t      window  );

xcb_get_geometry_cookie_t
xcb_get_geometry (xcb_connection_t *c  ,
                  xcb_drawable_t    drawable  );

xcb_get_geometry_cookie_t
xcb_get_geometry_unchecked (xcb_connection_t *c  ,
                            xcb_drawable_t    drawable  );

xcb_get_geometry_reply_t *
xcb_get_geometry_reply (xcb_connection_t           *c  ,
                        xcb_get_geometry_cookie_t   cookie  ,
                        xcb_generic_error_t       **e  );

int
xcb_query_tree_sizeof (const void  *_buffer  );

xcb_query_tree_cookie_t
xcb_query_tree (xcb_connection_t *c  ,
                xcb_window_t      window  );

xcb_query_tree_cookie_t
xcb_query_tree_unchecked (xcb_connection_t *c  ,
                          xcb_window_t      window  );

xcb_window_t *
xcb_query_tree_children (const xcb_query_tree_reply_t *R  );

int
xcb_query_tree_children_length (const xcb_query_tree_reply_t *R  );

xcb_generic_iterator_t
xcb_query_tree_children_end (const xcb_query_tree_reply_t *R  );

xcb_query_tree_reply_t *
xcb_query_tree_reply (xcb_connection_t         *c  ,
                      xcb_query_tree_cookie_t   cookie  ,
                      xcb_generic_error_t     **e  );

int
xcb_intern_atom_sizeof (const void  *_buffer  );

xcb_intern_atom_cookie_t
xcb_intern_atom (xcb_connection_t *c  ,
                 uint8_t           only_if_exists  ,
                 uint16_t          name_len  ,
                 const char       *name  );

xcb_intern_atom_cookie_t
xcb_intern_atom_unchecked (xcb_connection_t *c  ,
                           uint8_t           only_if_exists  ,
                           uint16_t          name_len  ,
                           const char       *name  );

xcb_intern_atom_reply_t *
xcb_intern_atom_reply (xcb_connection_t          *c  ,
                       xcb_intern_atom_cookie_t   cookie  ,
                       xcb_generic_error_t      **e  );

int
xcb_get_atom_name_sizeof (const void  *_buffer  );

xcb_get_atom_name_cookie_t
xcb_get_atom_name (xcb_connection_t *c  ,
                   xcb_atom_t        atom  );

xcb_get_atom_name_cookie_t
xcb_get_atom_name_unchecked (xcb_connection_t *c  ,
                             xcb_atom_t        atom  );

char *
xcb_get_atom_name_name (const xcb_get_atom_name_reply_t *R  );

int
xcb_get_atom_name_name_length (const xcb_get_atom_name_reply_t *R  );

xcb_generic_iterator_t
xcb_get_atom_name_name_end (const xcb_get_atom_name_reply_t *R  );

xcb_get_atom_name_reply_t *
xcb_get_atom_name_reply (xcb_connection_t            *c  ,
                         xcb_get_atom_name_cookie_t   cookie  ,
                         xcb_generic_error_t        **e  );

int
xcb_change_property_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_change_property_checked (xcb_connection_t *c  ,
                             uint8_t           mode  ,
                             xcb_window_t      window  ,
                             xcb_atom_t        property  ,
                             xcb_atom_t        type  ,
                             uint8_t           format  ,
                             uint32_t          data_len  ,
                             const void       *data  );

xcb_void_cookie_t
xcb_change_property (xcb_connection_t *c  ,
                     uint8_t           mode  ,
                     xcb_window_t      window  ,
                     xcb_atom_t        property  ,
                     xcb_atom_t        type  ,
                     uint8_t           format  ,
                     uint32_t          data_len  ,
                     const void       *data  );

xcb_void_cookie_t
xcb_delete_property_checked (xcb_connection_t *c  ,
                             xcb_window_t      window  ,
                             xcb_atom_t        property  );

xcb_void_cookie_t
xcb_delete_property (xcb_connection_t *c  ,
                     xcb_window_t      window  ,
                     xcb_atom_t        property  );

int
xcb_get_property_sizeof (const void  *_buffer  );

xcb_get_property_cookie_t
xcb_get_property (xcb_connection_t *c  ,
                  uint8_t           _delete  ,
                  xcb_window_t      window  ,
                  xcb_atom_t        property  ,
                  xcb_atom_t        type  ,
                  uint32_t          long_offset  ,
                  uint32_t          long_length  );

xcb_get_property_cookie_t
xcb_get_property_unchecked (xcb_connection_t *c  ,
                            uint8_t           _delete  ,
                            xcb_window_t      window  ,
                            xcb_atom_t        property  ,
                            xcb_atom_t        type  ,
                            uint32_t          long_offset  ,
                            uint32_t          long_length  );

void *
xcb_get_property_value (const xcb_get_property_reply_t *R  );

int
xcb_get_property_value_length (const xcb_get_property_reply_t *R  );

xcb_generic_iterator_t
xcb_get_property_value_end (const xcb_get_property_reply_t *R  );

xcb_get_property_reply_t *
xcb_get_property_reply (xcb_connection_t           *c  ,
                        xcb_get_property_cookie_t   cookie  ,
                        xcb_generic_error_t       **e  );

int
xcb_list_properties_sizeof (const void  *_buffer  );

xcb_list_properties_cookie_t
xcb_list_properties (xcb_connection_t *c  ,
                     xcb_window_t      window  );

xcb_list_properties_cookie_t
xcb_list_properties_unchecked (xcb_connection_t *c  ,
                               xcb_window_t      window  );

xcb_atom_t *
xcb_list_properties_atoms (const xcb_list_properties_reply_t *R  );

int
xcb_list_properties_atoms_length (const xcb_list_properties_reply_t *R  );

xcb_generic_iterator_t
xcb_list_properties_atoms_end (const xcb_list_properties_reply_t *R  );

xcb_list_properties_reply_t *
xcb_list_properties_reply (xcb_connection_t              *c  ,
                           xcb_list_properties_cookie_t   cookie  ,
                           xcb_generic_error_t          **e  );

xcb_void_cookie_t
xcb_set_selection_owner_checked (xcb_connection_t *c  ,
                                 xcb_window_t      owner  ,
                                 xcb_atom_t        selection  ,
                                 xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_set_selection_owner (xcb_connection_t *c  ,
                         xcb_window_t      owner  ,
                         xcb_atom_t        selection  ,
                         xcb_timestamp_t   time  );

xcb_get_selection_owner_cookie_t
xcb_get_selection_owner (xcb_connection_t *c  ,
                         xcb_atom_t        selection  );

xcb_get_selection_owner_cookie_t
xcb_get_selection_owner_unchecked (xcb_connection_t *c  ,
                                   xcb_atom_t        selection  );

xcb_get_selection_owner_reply_t *
xcb_get_selection_owner_reply (xcb_connection_t                  *c  ,
                               xcb_get_selection_owner_cookie_t   cookie  ,
                               xcb_generic_error_t              **e  );

xcb_void_cookie_t
xcb_convert_selection_checked (xcb_connection_t *c  ,
                               xcb_window_t      requestor  ,
                               xcb_atom_t        selection  ,
                               xcb_atom_t        target  ,
                               xcb_atom_t        property  ,
                               xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_convert_selection (xcb_connection_t *c  ,
                       xcb_window_t      requestor  ,
                       xcb_atom_t        selection  ,
                       xcb_atom_t        target  ,
                       xcb_atom_t        property  ,
                       xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_send_event_checked (xcb_connection_t *c  ,
                        uint8_t           propagate  ,
                        xcb_window_t      destination  ,
                        uint32_t          event_mask  ,
                        const char       *event  );

xcb_void_cookie_t
xcb_send_event (xcb_connection_t *c  ,
                uint8_t           propagate  ,
                xcb_window_t      destination  ,
                uint32_t          event_mask  ,
                const char       *event  );

xcb_grab_pointer_cookie_t
xcb_grab_pointer (xcb_connection_t *c  ,
                  uint8_t           owner_events  ,
                  xcb_window_t      grab_window  ,
                  uint16_t          event_mask  ,
                  uint8_t           pointer_mode  ,
                  uint8_t           keyboard_mode  ,
                  xcb_window_t      confine_to  ,
                  xcb_cursor_t      cursor  ,
                  xcb_timestamp_t   time  );

xcb_grab_pointer_cookie_t
xcb_grab_pointer_unchecked (xcb_connection_t *c  ,
                            uint8_t           owner_events  ,
                            xcb_window_t      grab_window  ,
                            uint16_t          event_mask  ,
                            uint8_t           pointer_mode  ,
                            uint8_t           keyboard_mode  ,
                            xcb_window_t      confine_to  ,
                            xcb_cursor_t      cursor  ,
                            xcb_timestamp_t   time  );

xcb_grab_pointer_reply_t *
xcb_grab_pointer_reply (xcb_connection_t           *c  ,
                        xcb_grab_pointer_cookie_t   cookie  ,
                        xcb_generic_error_t       **e  );

xcb_void_cookie_t
xcb_ungrab_pointer_checked (xcb_connection_t *c  ,
                            xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_ungrab_pointer (xcb_connection_t *c  ,
                    xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_grab_button_checked (xcb_connection_t *c  ,
                         uint8_t           owner_events  ,
                         xcb_window_t      grab_window  ,
                         uint16_t          event_mask  ,
                         uint8_t           pointer_mode  ,
                         uint8_t           keyboard_mode  ,
                         xcb_window_t      confine_to  ,
                         xcb_cursor_t      cursor  ,
                         uint8_t           button  ,
                         uint16_t          modifiers  );

xcb_void_cookie_t
xcb_grab_button (xcb_connection_t *c  ,
                 uint8_t           owner_events  ,
                 xcb_window_t      grab_window  ,
                 uint16_t          event_mask  ,
                 uint8_t           pointer_mode  ,
                 uint8_t           keyboard_mode  ,
                 xcb_window_t      confine_to  ,
                 xcb_cursor_t      cursor  ,
                 uint8_t           button  ,
                 uint16_t          modifiers  );

xcb_void_cookie_t
xcb_ungrab_button_checked (xcb_connection_t *c  ,
                           uint8_t           button  ,
                           xcb_window_t      grab_window  ,
                           uint16_t          modifiers  );

xcb_void_cookie_t
xcb_ungrab_button (xcb_connection_t *c  ,
                   uint8_t           button  ,
                   xcb_window_t      grab_window  ,
                   uint16_t          modifiers  );

xcb_void_cookie_t
xcb_change_active_pointer_grab_checked (xcb_connection_t *c  ,
                                        xcb_cursor_t      cursor  ,
                                        xcb_timestamp_t   time  ,
                                        uint16_t          event_mask  );

xcb_void_cookie_t
xcb_change_active_pointer_grab (xcb_connection_t *c  ,
                                xcb_cursor_t      cursor  ,
                                xcb_timestamp_t   time  ,
                                uint16_t          event_mask  );

xcb_grab_keyboard_cookie_t
xcb_grab_keyboard (xcb_connection_t *c  ,
                   uint8_t           owner_events  ,
                   xcb_window_t      grab_window  ,
                   xcb_timestamp_t   time  ,
                   uint8_t           pointer_mode  ,
                   uint8_t           keyboard_mode  );

xcb_grab_keyboard_cookie_t
xcb_grab_keyboard_unchecked (xcb_connection_t *c  ,
                             uint8_t           owner_events  ,
                             xcb_window_t      grab_window  ,
                             xcb_timestamp_t   time  ,
                             uint8_t           pointer_mode  ,
                             uint8_t           keyboard_mode  );

xcb_grab_keyboard_reply_t *
xcb_grab_keyboard_reply (xcb_connection_t            *c  ,
                         xcb_grab_keyboard_cookie_t   cookie  ,
                         xcb_generic_error_t        **e  );

xcb_void_cookie_t
xcb_ungrab_keyboard_checked (xcb_connection_t *c  ,
                             xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_ungrab_keyboard (xcb_connection_t *c  ,
                     xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_grab_key_checked (xcb_connection_t *c  ,
                      uint8_t           owner_events  ,
                      xcb_window_t      grab_window  ,
                      uint16_t          modifiers  ,
                      xcb_keycode_t     key  ,
                      uint8_t           pointer_mode  ,
                      uint8_t           keyboard_mode  );

xcb_void_cookie_t
xcb_grab_key (xcb_connection_t *c  ,
              uint8_t           owner_events  ,
              xcb_window_t      grab_window  ,
              uint16_t          modifiers  ,
              xcb_keycode_t     key  ,
              uint8_t           pointer_mode  ,
              uint8_t           keyboard_mode  );

xcb_void_cookie_t
xcb_ungrab_key_checked (xcb_connection_t *c  ,
                        xcb_keycode_t     key  ,
                        xcb_window_t      grab_window  ,
                        uint16_t          modifiers  );

xcb_void_cookie_t
xcb_ungrab_key (xcb_connection_t *c  ,
                xcb_keycode_t     key  ,
                xcb_window_t      grab_window  ,
                uint16_t          modifiers  );

xcb_void_cookie_t
xcb_allow_events_checked (xcb_connection_t *c  ,
                          uint8_t           mode  ,
                          xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_allow_events (xcb_connection_t *c  ,
                  uint8_t           mode  ,
                  xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_grab_server_checked (xcb_connection_t *c  );

xcb_void_cookie_t
xcb_grab_server (xcb_connection_t *c  );

xcb_void_cookie_t
xcb_ungrab_server_checked (xcb_connection_t *c  );

xcb_void_cookie_t
xcb_ungrab_server (xcb_connection_t *c  );

xcb_query_pointer_cookie_t
xcb_query_pointer (xcb_connection_t *c  ,
                   xcb_window_t      window  );

xcb_query_pointer_cookie_t
xcb_query_pointer_unchecked (xcb_connection_t *c  ,
                             xcb_window_t      window  );

xcb_query_pointer_reply_t *
xcb_query_pointer_reply (xcb_connection_t            *c  ,
                         xcb_query_pointer_cookie_t   cookie  ,
                         xcb_generic_error_t        **e  );

void
xcb_timecoord_next (xcb_timecoord_iterator_t *i  );

xcb_generic_iterator_t
xcb_timecoord_end (xcb_timecoord_iterator_t i  );

int
xcb_get_motion_events_sizeof (const void  *_buffer  );

xcb_get_motion_events_cookie_t
xcb_get_motion_events (xcb_connection_t *c  ,
                       xcb_window_t      window  ,
                       xcb_timestamp_t   start  ,
                       xcb_timestamp_t   stop  );

xcb_get_motion_events_cookie_t
xcb_get_motion_events_unchecked (xcb_connection_t *c  ,
                                 xcb_window_t      window  ,
                                 xcb_timestamp_t   start  ,
                                 xcb_timestamp_t   stop  );

xcb_timecoord_t *
xcb_get_motion_events_events (const xcb_get_motion_events_reply_t *R  );

int
xcb_get_motion_events_events_length (const xcb_get_motion_events_reply_t *R  );

xcb_timecoord_iterator_t
xcb_get_motion_events_events_iterator (const xcb_get_motion_events_reply_t *R  );

xcb_get_motion_events_reply_t *
xcb_get_motion_events_reply (xcb_connection_t                *c  ,
                             xcb_get_motion_events_cookie_t   cookie  ,
                             xcb_generic_error_t            **e  );

xcb_translate_coordinates_cookie_t
xcb_translate_coordinates (xcb_connection_t *c  ,
                           xcb_window_t      src_window  ,
                           xcb_window_t      dst_window  ,
                           int16_t           src_x  ,
                           int16_t           src_y  );

xcb_translate_coordinates_cookie_t
xcb_translate_coordinates_unchecked (xcb_connection_t *c  ,
                                     xcb_window_t      src_window  ,
                                     xcb_window_t      dst_window  ,
                                     int16_t           src_x  ,
                                     int16_t           src_y  );

xcb_translate_coordinates_reply_t *
xcb_translate_coordinates_reply (xcb_connection_t                    *c  ,
                                 xcb_translate_coordinates_cookie_t   cookie  ,
                                 xcb_generic_error_t                **e  );

xcb_void_cookie_t
xcb_warp_pointer_checked (xcb_connection_t *c  ,
                          xcb_window_t      src_window  ,
                          xcb_window_t      dst_window  ,
                          int16_t           src_x  ,
                          int16_t           src_y  ,
                          uint16_t          src_width  ,
                          uint16_t          src_height  ,
                          int16_t           dst_x  ,
                          int16_t           dst_y  );

xcb_void_cookie_t
xcb_warp_pointer (xcb_connection_t *c  ,
                  xcb_window_t      src_window  ,
                  xcb_window_t      dst_window  ,
                  int16_t           src_x  ,
                  int16_t           src_y  ,
                  uint16_t          src_width  ,
                  uint16_t          src_height  ,
                  int16_t           dst_x  ,
                  int16_t           dst_y  );

xcb_void_cookie_t
xcb_set_input_focus_checked (xcb_connection_t *c  ,
                             uint8_t           revert_to  ,
                             xcb_window_t      focus  ,
                             xcb_timestamp_t   time  );

xcb_void_cookie_t
xcb_set_input_focus (xcb_connection_t *c  ,
                     uint8_t           revert_to  ,
                     xcb_window_t      focus  ,
                     xcb_timestamp_t   time  );

xcb_get_input_focus_cookie_t
xcb_get_input_focus (xcb_connection_t *c  );

xcb_get_input_focus_cookie_t
xcb_get_input_focus_unchecked (xcb_connection_t *c  );

xcb_get_input_focus_reply_t *
xcb_get_input_focus_reply (xcb_connection_t              *c  ,
                           xcb_get_input_focus_cookie_t   cookie  ,
                           xcb_generic_error_t          **e  );

xcb_query_keymap_cookie_t
xcb_query_keymap (xcb_connection_t *c  );

xcb_query_keymap_cookie_t
xcb_query_keymap_unchecked (xcb_connection_t *c  );

xcb_query_keymap_reply_t *
xcb_query_keymap_reply (xcb_connection_t           *c  ,
                        xcb_query_keymap_cookie_t   cookie  ,
                        xcb_generic_error_t       **e  );

int
xcb_open_font_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_open_font_checked (xcb_connection_t *c  ,
                       xcb_font_t        fid  ,
                       uint16_t          name_len  ,
                       const char       *name  );

xcb_void_cookie_t
xcb_open_font (xcb_connection_t *c  ,
               xcb_font_t        fid  ,
               uint16_t          name_len  ,
               const char       *name  );

xcb_void_cookie_t
xcb_close_font_checked (xcb_connection_t *c  ,
                        xcb_font_t        font  );

xcb_void_cookie_t
xcb_close_font (xcb_connection_t *c  ,
                xcb_font_t        font  );

void
xcb_fontprop_next (xcb_fontprop_iterator_t *i  );

xcb_generic_iterator_t
xcb_fontprop_end (xcb_fontprop_iterator_t i  );

void
xcb_charinfo_next (xcb_charinfo_iterator_t *i  );

xcb_generic_iterator_t
xcb_charinfo_end (xcb_charinfo_iterator_t i  );

int
xcb_query_font_sizeof (const void  *_buffer  );

xcb_query_font_cookie_t
xcb_query_font (xcb_connection_t *c  ,
                xcb_fontable_t    font  );

xcb_query_font_cookie_t
xcb_query_font_unchecked (xcb_connection_t *c  ,
                          xcb_fontable_t    font  );

xcb_fontprop_t *
xcb_query_font_properties (const xcb_query_font_reply_t *R  );

int
xcb_query_font_properties_length (const xcb_query_font_reply_t *R  );

xcb_fontprop_iterator_t
xcb_query_font_properties_iterator (const xcb_query_font_reply_t *R  );

xcb_charinfo_t *
xcb_query_font_char_infos (const xcb_query_font_reply_t *R  );

int
xcb_query_font_char_infos_length (const xcb_query_font_reply_t *R  );

xcb_charinfo_iterator_t
xcb_query_font_char_infos_iterator (const xcb_query_font_reply_t *R  );

xcb_query_font_reply_t *
xcb_query_font_reply (xcb_connection_t         *c  ,
                      xcb_query_font_cookie_t   cookie  ,
                      xcb_generic_error_t     **e  );

int
xcb_query_text_extents_sizeof (const void  *_buffer  ,
                               uint32_t     string_len  );

xcb_query_text_extents_cookie_t
xcb_query_text_extents (xcb_connection_t   *c  ,
                        xcb_fontable_t      font  ,
                        uint32_t            string_len  ,
                        const xcb_char2b_t *string  );

xcb_query_text_extents_cookie_t
xcb_query_text_extents_unchecked (xcb_connection_t   *c  ,
                                  xcb_fontable_t      font  ,
                                  uint32_t            string_len  ,
                                  const xcb_char2b_t *string  );

xcb_query_text_extents_reply_t *
xcb_query_text_extents_reply (xcb_connection_t                 *c  ,
                              xcb_query_text_extents_cookie_t   cookie  ,
                              xcb_generic_error_t             **e  );

int
xcb_str_sizeof (const void  *_buffer  );

char *
xcb_str_name (const xcb_str_t *R  );

int
xcb_str_name_length (const xcb_str_t *R  );

xcb_generic_iterator_t
xcb_str_name_end (const xcb_str_t *R  );

void
xcb_str_next (xcb_str_iterator_t *i  );

xcb_generic_iterator_t
xcb_str_end (xcb_str_iterator_t i  );

int
xcb_list_fonts_sizeof (const void  *_buffer  );

xcb_list_fonts_cookie_t
xcb_list_fonts (xcb_connection_t *c  ,
                uint16_t          max_names  ,
                uint16_t          pattern_len  ,
                const char       *pattern  );

xcb_list_fonts_cookie_t
xcb_list_fonts_unchecked (xcb_connection_t *c  ,
                          uint16_t          max_names  ,
                          uint16_t          pattern_len  ,
                          const char       *pattern  );

int
xcb_list_fonts_names_length (const xcb_list_fonts_reply_t *R  );

xcb_str_iterator_t
xcb_list_fonts_names_iterator (const xcb_list_fonts_reply_t *R  );

xcb_list_fonts_reply_t *
xcb_list_fonts_reply (xcb_connection_t         *c  ,
                      xcb_list_fonts_cookie_t   cookie  ,
                      xcb_generic_error_t     **e  );

int
xcb_list_fonts_with_info_sizeof (const void  *_buffer  );

xcb_list_fonts_with_info_cookie_t
xcb_list_fonts_with_info (xcb_connection_t *c  ,
                          uint16_t          max_names  ,
                          uint16_t          pattern_len  ,
                          const char       *pattern  );

xcb_list_fonts_with_info_cookie_t
xcb_list_fonts_with_info_unchecked (xcb_connection_t *c  ,
                                    uint16_t          max_names  ,
                                    uint16_t          pattern_len  ,
                                    const char       *pattern  );

xcb_fontprop_t *
xcb_list_fonts_with_info_properties (const xcb_list_fonts_with_info_reply_t *R  );

int
xcb_list_fonts_with_info_properties_length (const xcb_list_fonts_with_info_reply_t *R  );

xcb_fontprop_iterator_t
xcb_list_fonts_with_info_properties_iterator (const xcb_list_fonts_with_info_reply_t *R  );

char *
xcb_list_fonts_with_info_name (const xcb_list_fonts_with_info_reply_t *R  );

int
xcb_list_fonts_with_info_name_length (const xcb_list_fonts_with_info_reply_t *R  );

xcb_generic_iterator_t
xcb_list_fonts_with_info_name_end (const xcb_list_fonts_with_info_reply_t *R  );

xcb_list_fonts_with_info_reply_t *
xcb_list_fonts_with_info_reply (xcb_connection_t                   *c  ,
                                xcb_list_fonts_with_info_cookie_t   cookie  ,
                                xcb_generic_error_t               **e  );

int
xcb_set_font_path_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_set_font_path_checked (xcb_connection_t *c  ,
                           uint16_t          font_qty  ,
                           const xcb_str_t  *font  );

xcb_void_cookie_t
xcb_set_font_path (xcb_connection_t *c  ,
                   uint16_t          font_qty  ,
                   const xcb_str_t  *font  );

int
xcb_get_font_path_sizeof (const void  *_buffer  );

xcb_get_font_path_cookie_t
xcb_get_font_path (xcb_connection_t *c  );

xcb_get_font_path_cookie_t
xcb_get_font_path_unchecked (xcb_connection_t *c  );

int
xcb_get_font_path_path_length (const xcb_get_font_path_reply_t *R  );

xcb_str_iterator_t
xcb_get_font_path_path_iterator (const xcb_get_font_path_reply_t *R  );

xcb_get_font_path_reply_t *
xcb_get_font_path_reply (xcb_connection_t            *c  ,
                         xcb_get_font_path_cookie_t   cookie  ,
                         xcb_generic_error_t        **e  );

xcb_void_cookie_t
xcb_create_pixmap_checked (xcb_connection_t *c  ,
                           uint8_t           depth  ,
                           xcb_pixmap_t      pid  ,
                           xcb_drawable_t    drawable  ,
                           uint16_t          width  ,
                           uint16_t          height  );

xcb_void_cookie_t
xcb_create_pixmap (xcb_connection_t *c  ,
                   uint8_t           depth  ,
                   xcb_pixmap_t      pid  ,
                   xcb_drawable_t    drawable  ,
                   uint16_t          width  ,
                   uint16_t          height  );

xcb_void_cookie_t
xcb_free_pixmap_checked (xcb_connection_t *c  ,
                         xcb_pixmap_t      pixmap  );

xcb_void_cookie_t
xcb_free_pixmap (xcb_connection_t *c  ,
                 xcb_pixmap_t      pixmap  );

int
xcb_create_gc_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_create_gc_checked (xcb_connection_t *c  ,
                       xcb_gcontext_t    cid  ,
                       xcb_drawable_t    drawable  ,
                       uint32_t          value_mask  ,
                       const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_create_gc (xcb_connection_t *c  ,
               xcb_gcontext_t    cid  ,
               xcb_drawable_t    drawable  ,
               uint32_t          value_mask  ,
               const uint32_t   *value_list  );

int
xcb_change_gc_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_change_gc_checked (xcb_connection_t *c  ,
                       xcb_gcontext_t    gc  ,
                       uint32_t          value_mask  ,
                       const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_change_gc (xcb_connection_t *c  ,
               xcb_gcontext_t    gc  ,
               uint32_t          value_mask  ,
               const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_copy_gc_checked (xcb_connection_t *c  ,
                     xcb_gcontext_t    src_gc  ,
                     xcb_gcontext_t    dst_gc  ,
                     uint32_t          value_mask  );

xcb_void_cookie_t
xcb_copy_gc (xcb_connection_t *c  ,
             xcb_gcontext_t    src_gc  ,
             xcb_gcontext_t    dst_gc  ,
             uint32_t          value_mask  );

int
xcb_set_dashes_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_set_dashes_checked (xcb_connection_t *c  ,
                        xcb_gcontext_t    gc  ,
                        uint16_t          dash_offset  ,
                        uint16_t          dashes_len  ,
                        const uint8_t    *dashes  );

xcb_void_cookie_t
xcb_set_dashes (xcb_connection_t *c  ,
                xcb_gcontext_t    gc  ,
                uint16_t          dash_offset  ,
                uint16_t          dashes_len  ,
                const uint8_t    *dashes  );

int
xcb_set_clip_rectangles_sizeof (const void  *_buffer  ,
                                uint32_t     rectangles_len  );

xcb_void_cookie_t
xcb_set_clip_rectangles_checked (xcb_connection_t      *c  ,
                                 uint8_t                ordering  ,
                                 xcb_gcontext_t         gc  ,
                                 int16_t                clip_x_origin  ,
                                 int16_t                clip_y_origin  ,
                                 uint32_t               rectangles_len  ,
                                 const xcb_rectangle_t *rectangles  );

xcb_void_cookie_t
xcb_set_clip_rectangles (xcb_connection_t      *c  ,
                         uint8_t                ordering  ,
                         xcb_gcontext_t         gc  ,
                         int16_t                clip_x_origin  ,
                         int16_t                clip_y_origin  ,
                         uint32_t               rectangles_len  ,
                         const xcb_rectangle_t *rectangles  );

xcb_void_cookie_t
xcb_free_gc_checked (xcb_connection_t *c  ,
                     xcb_gcontext_t    gc  );

xcb_void_cookie_t
xcb_free_gc (xcb_connection_t *c  ,
             xcb_gcontext_t    gc  );

xcb_void_cookie_t
xcb_clear_area_checked (xcb_connection_t *c  ,
                        uint8_t           exposures  ,
                        xcb_window_t      window  ,
                        int16_t           x  ,
                        int16_t           y  ,
                        uint16_t          width  ,
                        uint16_t          height  );

xcb_void_cookie_t
xcb_clear_area (xcb_connection_t *c  ,
                uint8_t           exposures  ,
                xcb_window_t      window  ,
                int16_t           x  ,
                int16_t           y  ,
                uint16_t          width  ,
                uint16_t          height  );

xcb_void_cookie_t
xcb_copy_area_checked (xcb_connection_t *c  ,
                       xcb_drawable_t    src_drawable  ,
                       xcb_drawable_t    dst_drawable  ,
                       xcb_gcontext_t    gc  ,
                       int16_t           src_x  ,
                       int16_t           src_y  ,
                       int16_t           dst_x  ,
                       int16_t           dst_y  ,
                       uint16_t          width  ,
                       uint16_t          height  );

xcb_void_cookie_t
xcb_copy_area (xcb_connection_t *c  ,
               xcb_drawable_t    src_drawable  ,
               xcb_drawable_t    dst_drawable  ,
               xcb_gcontext_t    gc  ,
               int16_t           src_x  ,
               int16_t           src_y  ,
               int16_t           dst_x  ,
               int16_t           dst_y  ,
               uint16_t          width  ,
               uint16_t          height  );

xcb_void_cookie_t
xcb_copy_plane_checked (xcb_connection_t *c  ,
                        xcb_drawable_t    src_drawable  ,
                        xcb_drawable_t    dst_drawable  ,
                        xcb_gcontext_t    gc  ,
                        int16_t           src_x  ,
                        int16_t           src_y  ,
                        int16_t           dst_x  ,
                        int16_t           dst_y  ,
                        uint16_t          width  ,
                        uint16_t          height  ,
                        uint32_t          bit_plane  );

xcb_void_cookie_t
xcb_copy_plane (xcb_connection_t *c  ,
                xcb_drawable_t    src_drawable  ,
                xcb_drawable_t    dst_drawable  ,
                xcb_gcontext_t    gc  ,
                int16_t           src_x  ,
                int16_t           src_y  ,
                int16_t           dst_x  ,
                int16_t           dst_y  ,
                uint16_t          width  ,
                uint16_t          height  ,
                uint32_t          bit_plane  );

int
xcb_poly_point_sizeof (const void  *_buffer  ,
                       uint32_t     points_len  );

xcb_void_cookie_t
xcb_poly_point_checked (xcb_connection_t  *c  ,
                        uint8_t            coordinate_mode  ,
                        xcb_drawable_t     drawable  ,
                        xcb_gcontext_t     gc  ,
                        uint32_t           points_len  ,
                        const xcb_point_t *points  );

xcb_void_cookie_t
xcb_poly_point (xcb_connection_t  *c  ,
                uint8_t            coordinate_mode  ,
                xcb_drawable_t     drawable  ,
                xcb_gcontext_t     gc  ,
                uint32_t           points_len  ,
                const xcb_point_t *points  );

int
xcb_poly_line_sizeof (const void  *_buffer  ,
                      uint32_t     points_len  );

xcb_void_cookie_t
xcb_poly_line_checked (xcb_connection_t  *c  ,
                       uint8_t            coordinate_mode  ,
                       xcb_drawable_t     drawable  ,
                       xcb_gcontext_t     gc  ,
                       uint32_t           points_len  ,
                       const xcb_point_t *points  );

xcb_void_cookie_t
xcb_poly_line (xcb_connection_t  *c  ,
               uint8_t            coordinate_mode  ,
               xcb_drawable_t     drawable  ,
               xcb_gcontext_t     gc  ,
               uint32_t           points_len  ,
               const xcb_point_t *points  );

void
xcb_segment_next (xcb_segment_iterator_t *i  );

xcb_generic_iterator_t
xcb_segment_end (xcb_segment_iterator_t i  );

int
xcb_poly_segment_sizeof (const void  *_buffer  ,
                         uint32_t     segments_len  );

xcb_void_cookie_t
xcb_poly_segment_checked (xcb_connection_t    *c  ,
                          xcb_drawable_t       drawable  ,
                          xcb_gcontext_t       gc  ,
                          uint32_t             segments_len  ,
                          const xcb_segment_t *segments  );

xcb_void_cookie_t
xcb_poly_segment (xcb_connection_t    *c  ,
                  xcb_drawable_t       drawable  ,
                  xcb_gcontext_t       gc  ,
                  uint32_t             segments_len  ,
                  const xcb_segment_t *segments  );

int
xcb_poly_rectangle_sizeof (const void  *_buffer  ,
                           uint32_t     rectangles_len  );

xcb_void_cookie_t
xcb_poly_rectangle_checked (xcb_connection_t      *c  ,
                            xcb_drawable_t         drawable  ,
                            xcb_gcontext_t         gc  ,
                            uint32_t               rectangles_len  ,
                            const xcb_rectangle_t *rectangles  );

xcb_void_cookie_t
xcb_poly_rectangle (xcb_connection_t      *c  ,
                    xcb_drawable_t         drawable  ,
                    xcb_gcontext_t         gc  ,
                    uint32_t               rectangles_len  ,
                    const xcb_rectangle_t *rectangles  );

int
xcb_poly_arc_sizeof (const void  *_buffer  ,
                     uint32_t     arcs_len  );

xcb_void_cookie_t
xcb_poly_arc_checked (xcb_connection_t *c  ,
                      xcb_drawable_t    drawable  ,
                      xcb_gcontext_t    gc  ,
                      uint32_t          arcs_len  ,
                      const xcb_arc_t  *arcs  );

xcb_void_cookie_t
xcb_poly_arc (xcb_connection_t *c  ,
              xcb_drawable_t    drawable  ,
              xcb_gcontext_t    gc  ,
              uint32_t          arcs_len  ,
              const xcb_arc_t  *arcs  );

int
xcb_fill_poly_sizeof (const void  *_buffer  ,
                      uint32_t     points_len  );

xcb_void_cookie_t
xcb_fill_poly_checked (xcb_connection_t  *c  ,
                       xcb_drawable_t     drawable  ,
                       xcb_gcontext_t     gc  ,
                       uint8_t            shape  ,
                       uint8_t            coordinate_mode  ,
                       uint32_t           points_len  ,
                       const xcb_point_t *points  );

xcb_void_cookie_t
xcb_fill_poly (xcb_connection_t  *c  ,
               xcb_drawable_t     drawable  ,
               xcb_gcontext_t     gc  ,
               uint8_t            shape  ,
               uint8_t            coordinate_mode  ,
               uint32_t           points_len  ,
               const xcb_point_t *points  );

int
xcb_poly_fill_rectangle_sizeof (const void  *_buffer  ,
                                uint32_t     rectangles_len  );

xcb_void_cookie_t
xcb_poly_fill_rectangle_checked (xcb_connection_t      *c  ,
                                 xcb_drawable_t         drawable  ,
                                 xcb_gcontext_t         gc  ,
                                 uint32_t               rectangles_len  ,
                                 const xcb_rectangle_t *rectangles  );

xcb_void_cookie_t
xcb_poly_fill_rectangle (xcb_connection_t      *c  ,
                         xcb_drawable_t         drawable  ,
                         xcb_gcontext_t         gc  ,
                         uint32_t               rectangles_len  ,
                         const xcb_rectangle_t *rectangles  );

int
xcb_poly_fill_arc_sizeof (const void  *_buffer  ,
                          uint32_t     arcs_len  );

xcb_void_cookie_t
xcb_poly_fill_arc_checked (xcb_connection_t *c  ,
                           xcb_drawable_t    drawable  ,
                           xcb_gcontext_t    gc  ,
                           uint32_t          arcs_len  ,
                           const xcb_arc_t  *arcs  );

xcb_void_cookie_t
xcb_poly_fill_arc (xcb_connection_t *c  ,
                   xcb_drawable_t    drawable  ,
                   xcb_gcontext_t    gc  ,
                   uint32_t          arcs_len  ,
                   const xcb_arc_t  *arcs  );

int
xcb_put_image_sizeof (const void  *_buffer  ,
                      uint32_t     data_len  );

xcb_void_cookie_t
xcb_put_image_checked (xcb_connection_t *c  ,
                       uint8_t           format  ,
                       xcb_drawable_t    drawable  ,
                       xcb_gcontext_t    gc  ,
                       uint16_t          width  ,
                       uint16_t          height  ,
                       int16_t           dst_x  ,
                       int16_t           dst_y  ,
                       uint8_t           left_pad  ,
                       uint8_t           depth  ,
                       uint32_t          data_len  ,
                       const uint8_t    *data  );

xcb_void_cookie_t
xcb_put_image (xcb_connection_t *c  ,
               uint8_t           format  ,
               xcb_drawable_t    drawable  ,
               xcb_gcontext_t    gc  ,
               uint16_t          width  ,
               uint16_t          height  ,
               int16_t           dst_x  ,
               int16_t           dst_y  ,
               uint8_t           left_pad  ,
               uint8_t           depth  ,
               uint32_t          data_len  ,
               const uint8_t    *data  );

int
xcb_get_image_sizeof (const void  *_buffer  );

xcb_get_image_cookie_t
xcb_get_image (xcb_connection_t *c  ,
               uint8_t           format  ,
               xcb_drawable_t    drawable  ,
               int16_t           x  ,
               int16_t           y  ,
               uint16_t          width  ,
               uint16_t          height  ,
               uint32_t          plane_mask  );

xcb_get_image_cookie_t
xcb_get_image_unchecked (xcb_connection_t *c  ,
                         uint8_t           format  ,
                         xcb_drawable_t    drawable  ,
                         int16_t           x  ,
                         int16_t           y  ,
                         uint16_t          width  ,
                         uint16_t          height  ,
                         uint32_t          plane_mask  );

uint8_t *
xcb_get_image_data (const xcb_get_image_reply_t *R  );

int
xcb_get_image_data_length (const xcb_get_image_reply_t *R  );

xcb_generic_iterator_t
xcb_get_image_data_end (const xcb_get_image_reply_t *R  );

xcb_get_image_reply_t *
xcb_get_image_reply (xcb_connection_t        *c  ,
                     xcb_get_image_cookie_t   cookie  ,
                     xcb_generic_error_t    **e  );

int
xcb_poly_text_8_sizeof (const void  *_buffer  ,
                        uint32_t     items_len  );

xcb_void_cookie_t
xcb_poly_text_8_checked (xcb_connection_t *c  ,
                         xcb_drawable_t    drawable  ,
                         xcb_gcontext_t    gc  ,
                         int16_t           x  ,
                         int16_t           y  ,
                         uint32_t          items_len  ,
                         const uint8_t    *items  );

xcb_void_cookie_t
xcb_poly_text_8 (xcb_connection_t *c  ,
                 xcb_drawable_t    drawable  ,
                 xcb_gcontext_t    gc  ,
                 int16_t           x  ,
                 int16_t           y  ,
                 uint32_t          items_len  ,
                 const uint8_t    *items  );

int
xcb_poly_text_16_sizeof (const void  *_buffer  ,
                         uint32_t     items_len  );

xcb_void_cookie_t
xcb_poly_text_16_checked (xcb_connection_t *c  ,
                          xcb_drawable_t    drawable  ,
                          xcb_gcontext_t    gc  ,
                          int16_t           x  ,
                          int16_t           y  ,
                          uint32_t          items_len  ,
                          const uint8_t    *items  );

xcb_void_cookie_t
xcb_poly_text_16 (xcb_connection_t *c  ,
                  xcb_drawable_t    drawable  ,
                  xcb_gcontext_t    gc  ,
                  int16_t           x  ,
                  int16_t           y  ,
                  uint32_t          items_len  ,
                  const uint8_t    *items  );

int
xcb_image_text_8_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_image_text_8_checked (xcb_connection_t *c  ,
                          uint8_t           string_len  ,
                          xcb_drawable_t    drawable  ,
                          xcb_gcontext_t    gc  ,
                          int16_t           x  ,
                          int16_t           y  ,
                          const char       *string  );

xcb_void_cookie_t
xcb_image_text_8 (xcb_connection_t *c  ,
                  uint8_t           string_len  ,
                  xcb_drawable_t    drawable  ,
                  xcb_gcontext_t    gc  ,
                  int16_t           x  ,
                  int16_t           y  ,
                  const char       *string  );

int
xcb_image_text_16_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_image_text_16_checked (xcb_connection_t   *c  ,
                           uint8_t             string_len  ,
                           xcb_drawable_t      drawable  ,
                           xcb_gcontext_t      gc  ,
                           int16_t             x  ,
                           int16_t             y  ,
                           const xcb_char2b_t *string  );

xcb_void_cookie_t
xcb_image_text_16 (xcb_connection_t   *c  ,
                   uint8_t             string_len  ,
                   xcb_drawable_t      drawable  ,
                   xcb_gcontext_t      gc  ,
                   int16_t             x  ,
                   int16_t             y  ,
                   const xcb_char2b_t *string  );

xcb_void_cookie_t
xcb_create_colormap_checked (xcb_connection_t *c  ,
                             uint8_t           alloc  ,
                             xcb_colormap_t    mid  ,
                             xcb_window_t      window  ,
                             xcb_visualid_t    visual  );

xcb_void_cookie_t
xcb_create_colormap (xcb_connection_t *c  ,
                     uint8_t           alloc  ,
                     xcb_colormap_t    mid  ,
                     xcb_window_t      window  ,
                     xcb_visualid_t    visual  );

xcb_void_cookie_t
xcb_free_colormap_checked (xcb_connection_t *c  ,
                           xcb_colormap_t    cmap  );

xcb_void_cookie_t
xcb_free_colormap (xcb_connection_t *c  ,
                   xcb_colormap_t    cmap  );

xcb_void_cookie_t
xcb_copy_colormap_and_free_checked (xcb_connection_t *c  ,
                                    xcb_colormap_t    mid  ,
                                    xcb_colormap_t    src_cmap  );

xcb_void_cookie_t
xcb_copy_colormap_and_free (xcb_connection_t *c  ,
                            xcb_colormap_t    mid  ,
                            xcb_colormap_t    src_cmap  );

xcb_void_cookie_t
xcb_install_colormap_checked (xcb_connection_t *c  ,
                              xcb_colormap_t    cmap  );

xcb_void_cookie_t
xcb_install_colormap (xcb_connection_t *c  ,
                      xcb_colormap_t    cmap  );

xcb_void_cookie_t
xcb_uninstall_colormap_checked (xcb_connection_t *c  ,
                                xcb_colormap_t    cmap  );

xcb_void_cookie_t
xcb_uninstall_colormap (xcb_connection_t *c  ,
                        xcb_colormap_t    cmap  );

int
xcb_list_installed_colormaps_sizeof (const void  *_buffer  );

xcb_list_installed_colormaps_cookie_t
xcb_list_installed_colormaps (xcb_connection_t *c  ,
                              xcb_window_t      window  );

xcb_list_installed_colormaps_cookie_t
xcb_list_installed_colormaps_unchecked (xcb_connection_t *c  ,
                                        xcb_window_t      window  );

xcb_colormap_t *
xcb_list_installed_colormaps_cmaps (const xcb_list_installed_colormaps_reply_t *R  );

int
xcb_list_installed_colormaps_cmaps_length (const xcb_list_installed_colormaps_reply_t *R  );

xcb_generic_iterator_t
xcb_list_installed_colormaps_cmaps_end (const xcb_list_installed_colormaps_reply_t *R  );

xcb_list_installed_colormaps_reply_t *
xcb_list_installed_colormaps_reply (xcb_connection_t                       *c  ,
                                    xcb_list_installed_colormaps_cookie_t   cookie  ,
                                    xcb_generic_error_t                   **e  );

xcb_alloc_color_cookie_t
xcb_alloc_color (xcb_connection_t *c  ,
                 xcb_colormap_t    cmap  ,
                 uint16_t          red  ,
                 uint16_t          green  ,
                 uint16_t          blue  );

xcb_alloc_color_cookie_t
xcb_alloc_color_unchecked (xcb_connection_t *c  ,
                           xcb_colormap_t    cmap  ,
                           uint16_t          red  ,
                           uint16_t          green  ,
                           uint16_t          blue  );

xcb_alloc_color_reply_t *
xcb_alloc_color_reply (xcb_connection_t          *c  ,
                       xcb_alloc_color_cookie_t   cookie  ,
                       xcb_generic_error_t      **e  );

int
xcb_alloc_named_color_sizeof (const void  *_buffer  );

xcb_alloc_named_color_cookie_t
xcb_alloc_named_color (xcb_connection_t *c  ,
                       xcb_colormap_t    cmap  ,
                       uint16_t          name_len  ,
                       const char       *name  );

xcb_alloc_named_color_cookie_t
xcb_alloc_named_color_unchecked (xcb_connection_t *c  ,
                                 xcb_colormap_t    cmap  ,
                                 uint16_t          name_len  ,
                                 const char       *name  );

xcb_alloc_named_color_reply_t *
xcb_alloc_named_color_reply (xcb_connection_t                *c  ,
                             xcb_alloc_named_color_cookie_t   cookie  ,
                             xcb_generic_error_t            **e  );

int
xcb_alloc_color_cells_sizeof (const void  *_buffer  );

xcb_alloc_color_cells_cookie_t
xcb_alloc_color_cells (xcb_connection_t *c  ,
                       uint8_t           contiguous  ,
                       xcb_colormap_t    cmap  ,
                       uint16_t          colors  ,
                       uint16_t          planes  );

xcb_alloc_color_cells_cookie_t
xcb_alloc_color_cells_unchecked (xcb_connection_t *c  ,
                                 uint8_t           contiguous  ,
                                 xcb_colormap_t    cmap  ,
                                 uint16_t          colors  ,
                                 uint16_t          planes  );

uint32_t *
xcb_alloc_color_cells_pixels (const xcb_alloc_color_cells_reply_t *R  );

int
xcb_alloc_color_cells_pixels_length (const xcb_alloc_color_cells_reply_t *R  );

xcb_generic_iterator_t
xcb_alloc_color_cells_pixels_end (const xcb_alloc_color_cells_reply_t *R  );

uint32_t *
xcb_alloc_color_cells_masks (const xcb_alloc_color_cells_reply_t *R  );

int
xcb_alloc_color_cells_masks_length (const xcb_alloc_color_cells_reply_t *R  );

xcb_generic_iterator_t
xcb_alloc_color_cells_masks_end (const xcb_alloc_color_cells_reply_t *R  );

xcb_alloc_color_cells_reply_t *
xcb_alloc_color_cells_reply (xcb_connection_t                *c  ,
                             xcb_alloc_color_cells_cookie_t   cookie  ,
                             xcb_generic_error_t            **e  );

int
xcb_alloc_color_planes_sizeof (const void  *_buffer  );

xcb_alloc_color_planes_cookie_t
xcb_alloc_color_planes (xcb_connection_t *c  ,
                        uint8_t           contiguous  ,
                        xcb_colormap_t    cmap  ,
                        uint16_t          colors  ,
                        uint16_t          reds  ,
                        uint16_t          greens  ,
                        uint16_t          blues  );

xcb_alloc_color_planes_cookie_t
xcb_alloc_color_planes_unchecked (xcb_connection_t *c  ,
                                  uint8_t           contiguous  ,
                                  xcb_colormap_t    cmap  ,
                                  uint16_t          colors  ,
                                  uint16_t          reds  ,
                                  uint16_t          greens  ,
                                  uint16_t          blues  );

uint32_t *
xcb_alloc_color_planes_pixels (const xcb_alloc_color_planes_reply_t *R  );

int
xcb_alloc_color_planes_pixels_length (const xcb_alloc_color_planes_reply_t *R  );

xcb_generic_iterator_t
xcb_alloc_color_planes_pixels_end (const xcb_alloc_color_planes_reply_t *R  );

xcb_alloc_color_planes_reply_t *
xcb_alloc_color_planes_reply (xcb_connection_t                 *c  ,
                              xcb_alloc_color_planes_cookie_t   cookie  ,
                              xcb_generic_error_t             **e  );

int
xcb_free_colors_sizeof (const void  *_buffer  ,
                        uint32_t     pixels_len  );

xcb_void_cookie_t
xcb_free_colors_checked (xcb_connection_t *c  ,
                         xcb_colormap_t    cmap  ,
                         uint32_t          plane_mask  ,
                         uint32_t          pixels_len  ,
                         const uint32_t   *pixels  );

xcb_void_cookie_t
xcb_free_colors (xcb_connection_t *c  ,
                 xcb_colormap_t    cmap  ,
                 uint32_t          plane_mask  ,
                 uint32_t          pixels_len  ,
                 const uint32_t   *pixels  );

void
xcb_coloritem_next (xcb_coloritem_iterator_t *i  );

xcb_generic_iterator_t
xcb_coloritem_end (xcb_coloritem_iterator_t i  );

int
xcb_store_colors_sizeof (const void  *_buffer  ,
                         uint32_t     items_len  );

xcb_void_cookie_t
xcb_store_colors_checked (xcb_connection_t      *c  ,
                          xcb_colormap_t         cmap  ,
                          uint32_t               items_len  ,
                          const xcb_coloritem_t *items  );

xcb_void_cookie_t
xcb_store_colors (xcb_connection_t      *c  ,
                  xcb_colormap_t         cmap  ,
                  uint32_t               items_len  ,
                  const xcb_coloritem_t *items  );

int
xcb_store_named_color_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_store_named_color_checked (xcb_connection_t *c  ,
                               uint8_t           flags  ,
                               xcb_colormap_t    cmap  ,
                               uint32_t          pixel  ,
                               uint16_t          name_len  ,
                               const char       *name  );

xcb_void_cookie_t
xcb_store_named_color (xcb_connection_t *c  ,
                       uint8_t           flags  ,
                       xcb_colormap_t    cmap  ,
                       uint32_t          pixel  ,
                       uint16_t          name_len  ,
                       const char       *name  );

void
xcb_rgb_next (xcb_rgb_iterator_t *i  );

xcb_generic_iterator_t
xcb_rgb_end (xcb_rgb_iterator_t i  );

int
xcb_query_colors_sizeof (const void  *_buffer  ,
                         uint32_t     pixels_len  );

xcb_query_colors_cookie_t
xcb_query_colors (xcb_connection_t *c  ,
                  xcb_colormap_t    cmap  ,
                  uint32_t          pixels_len  ,
                  const uint32_t   *pixels  );

xcb_query_colors_cookie_t
xcb_query_colors_unchecked (xcb_connection_t *c  ,
                            xcb_colormap_t    cmap  ,
                            uint32_t          pixels_len  ,
                            const uint32_t   *pixels  );

xcb_rgb_t *
xcb_query_colors_colors (const xcb_query_colors_reply_t *R  );

int
xcb_query_colors_colors_length (const xcb_query_colors_reply_t *R  );

xcb_rgb_iterator_t
xcb_query_colors_colors_iterator (const xcb_query_colors_reply_t *R  );

xcb_query_colors_reply_t *
xcb_query_colors_reply (xcb_connection_t           *c  ,
                        xcb_query_colors_cookie_t   cookie  ,
                        xcb_generic_error_t       **e  );

int
xcb_lookup_color_sizeof (const void  *_buffer  );

xcb_lookup_color_cookie_t
xcb_lookup_color (xcb_connection_t *c  ,
                  xcb_colormap_t    cmap  ,
                  uint16_t          name_len  ,
                  const char       *name  );

xcb_lookup_color_cookie_t
xcb_lookup_color_unchecked (xcb_connection_t *c  ,
                            xcb_colormap_t    cmap  ,
                            uint16_t          name_len  ,
                            const char       *name  );

xcb_lookup_color_reply_t *
xcb_lookup_color_reply (xcb_connection_t           *c  ,
                        xcb_lookup_color_cookie_t   cookie  ,
                        xcb_generic_error_t       **e  );

xcb_void_cookie_t
xcb_create_cursor_checked (xcb_connection_t *c  ,
                           xcb_cursor_t      cid  ,
                           xcb_pixmap_t      source  ,
                           xcb_pixmap_t      mask  ,
                           uint16_t          fore_red  ,
                           uint16_t          fore_green  ,
                           uint16_t          fore_blue  ,
                           uint16_t          back_red  ,
                           uint16_t          back_green  ,
                           uint16_t          back_blue  ,
                           uint16_t          x  ,
                           uint16_t          y  );

xcb_void_cookie_t
xcb_create_cursor (xcb_connection_t *c  ,
                   xcb_cursor_t      cid  ,
                   xcb_pixmap_t      source  ,
                   xcb_pixmap_t      mask  ,
                   uint16_t          fore_red  ,
                   uint16_t          fore_green  ,
                   uint16_t          fore_blue  ,
                   uint16_t          back_red  ,
                   uint16_t          back_green  ,
                   uint16_t          back_blue  ,
                   uint16_t          x  ,
                   uint16_t          y  );

xcb_void_cookie_t
xcb_create_glyph_cursor_checked (xcb_connection_t *c  ,
                                 xcb_cursor_t      cid  ,
                                 xcb_font_t        source_font  ,
                                 xcb_font_t        mask_font  ,
                                 uint16_t          source_char  ,
                                 uint16_t          mask_char  ,
                                 uint16_t          fore_red  ,
                                 uint16_t          fore_green  ,
                                 uint16_t          fore_blue  ,
                                 uint16_t          back_red  ,
                                 uint16_t          back_green  ,
                                 uint16_t          back_blue  );

xcb_void_cookie_t
xcb_create_glyph_cursor (xcb_connection_t *c  ,
                         xcb_cursor_t      cid  ,
                         xcb_font_t        source_font  ,
                         xcb_font_t        mask_font  ,
                         uint16_t          source_char  ,
                         uint16_t          mask_char  ,
                         uint16_t          fore_red  ,
                         uint16_t          fore_green  ,
                         uint16_t          fore_blue  ,
                         uint16_t          back_red  ,
                         uint16_t          back_green  ,
                         uint16_t          back_blue  );

xcb_void_cookie_t
xcb_free_cursor_checked (xcb_connection_t *c  ,
                         xcb_cursor_t      cursor  );

xcb_void_cookie_t
xcb_free_cursor (xcb_connection_t *c  ,
                 xcb_cursor_t      cursor  );

xcb_void_cookie_t
xcb_recolor_cursor_checked (xcb_connection_t *c  ,
                            xcb_cursor_t      cursor  ,
                            uint16_t          fore_red  ,
                            uint16_t          fore_green  ,
                            uint16_t          fore_blue  ,
                            uint16_t          back_red  ,
                            uint16_t          back_green  ,
                            uint16_t          back_blue  );

xcb_void_cookie_t
xcb_recolor_cursor (xcb_connection_t *c  ,
                    xcb_cursor_t      cursor  ,
                    uint16_t          fore_red  ,
                    uint16_t          fore_green  ,
                    uint16_t          fore_blue  ,
                    uint16_t          back_red  ,
                    uint16_t          back_green  ,
                    uint16_t          back_blue  );

xcb_query_best_size_cookie_t
xcb_query_best_size (xcb_connection_t *c  ,
                     uint8_t           _class  ,
                     xcb_drawable_t    drawable  ,
                     uint16_t          width  ,
                     uint16_t          height  );

xcb_query_best_size_cookie_t
xcb_query_best_size_unchecked (xcb_connection_t *c  ,
                               uint8_t           _class  ,
                               xcb_drawable_t    drawable  ,
                               uint16_t          width  ,
                               uint16_t          height  );

xcb_query_best_size_reply_t *
xcb_query_best_size_reply (xcb_connection_t              *c  ,
                           xcb_query_best_size_cookie_t   cookie  ,
                           xcb_generic_error_t          **e  );

int
xcb_query_extension_sizeof (const void  *_buffer  );

xcb_query_extension_cookie_t
xcb_query_extension (xcb_connection_t *c  ,
                     uint16_t          name_len  ,
                     const char       *name  );

xcb_query_extension_cookie_t
xcb_query_extension_unchecked (xcb_connection_t *c  ,
                               uint16_t          name_len  ,
                               const char       *name  );

xcb_query_extension_reply_t *
xcb_query_extension_reply (xcb_connection_t              *c  ,
                           xcb_query_extension_cookie_t   cookie  ,
                           xcb_generic_error_t          **e  );

int
xcb_list_extensions_sizeof (const void  *_buffer  );

xcb_list_extensions_cookie_t
xcb_list_extensions (xcb_connection_t *c  );

xcb_list_extensions_cookie_t
xcb_list_extensions_unchecked (xcb_connection_t *c  );

int
xcb_list_extensions_names_length (const xcb_list_extensions_reply_t *R  );

xcb_str_iterator_t
xcb_list_extensions_names_iterator (const xcb_list_extensions_reply_t *R  );

xcb_list_extensions_reply_t *
xcb_list_extensions_reply (xcb_connection_t              *c  ,
                           xcb_list_extensions_cookie_t   cookie  ,
                           xcb_generic_error_t          **e  );

int
xcb_change_keyboard_mapping_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_change_keyboard_mapping_checked (xcb_connection_t   *c  ,
                                     uint8_t             keycode_count  ,
                                     xcb_keycode_t       first_keycode  ,
                                     uint8_t             keysyms_per_keycode  ,
                                     const xcb_keysym_t *keysyms  );

xcb_void_cookie_t
xcb_change_keyboard_mapping (xcb_connection_t   *c  ,
                             uint8_t             keycode_count  ,
                             xcb_keycode_t       first_keycode  ,
                             uint8_t             keysyms_per_keycode  ,
                             const xcb_keysym_t *keysyms  );

int
xcb_get_keyboard_mapping_sizeof (const void  *_buffer  );

xcb_get_keyboard_mapping_cookie_t
xcb_get_keyboard_mapping (xcb_connection_t *c  ,
                          xcb_keycode_t     first_keycode  ,
                          uint8_t           count  );

xcb_get_keyboard_mapping_cookie_t
xcb_get_keyboard_mapping_unchecked (xcb_connection_t *c  ,
                                    xcb_keycode_t     first_keycode  ,
                                    uint8_t           count  );

xcb_keysym_t *
xcb_get_keyboard_mapping_keysyms (const xcb_get_keyboard_mapping_reply_t *R  );

int
xcb_get_keyboard_mapping_keysyms_length (const xcb_get_keyboard_mapping_reply_t *R  );

xcb_generic_iterator_t
xcb_get_keyboard_mapping_keysyms_end (const xcb_get_keyboard_mapping_reply_t *R  );

xcb_get_keyboard_mapping_reply_t *
xcb_get_keyboard_mapping_reply (xcb_connection_t                   *c  ,
                                xcb_get_keyboard_mapping_cookie_t   cookie  ,
                                xcb_generic_error_t               **e  );

int
xcb_change_keyboard_control_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_change_keyboard_control_checked (xcb_connection_t *c  ,
                                     uint32_t          value_mask  ,
                                     const uint32_t   *value_list  );

xcb_void_cookie_t
xcb_change_keyboard_control (xcb_connection_t *c  ,
                             uint32_t          value_mask  ,
                             const uint32_t   *value_list  );

xcb_get_keyboard_control_cookie_t
xcb_get_keyboard_control (xcb_connection_t *c  );

xcb_get_keyboard_control_cookie_t
xcb_get_keyboard_control_unchecked (xcb_connection_t *c  );

xcb_get_keyboard_control_reply_t *
xcb_get_keyboard_control_reply (xcb_connection_t                   *c  ,
                                xcb_get_keyboard_control_cookie_t   cookie  ,
                                xcb_generic_error_t               **e  );

xcb_void_cookie_t
xcb_bell_checked (xcb_connection_t *c  ,
                  int8_t            percent  );

xcb_void_cookie_t
xcb_bell (xcb_connection_t *c  ,
          int8_t            percent  );

xcb_void_cookie_t
xcb_change_pointer_control_checked (xcb_connection_t *c  ,
                                    int16_t           acceleration_numerator  ,
                                    int16_t           acceleration_denominator  ,
                                    int16_t           threshold  ,
                                    uint8_t           do_acceleration  ,
                                    uint8_t           do_threshold  );

xcb_void_cookie_t
xcb_change_pointer_control (xcb_connection_t *c  ,
                            int16_t           acceleration_numerator  ,
                            int16_t           acceleration_denominator  ,
                            int16_t           threshold  ,
                            uint8_t           do_acceleration  ,
                            uint8_t           do_threshold  );

xcb_get_pointer_control_cookie_t
xcb_get_pointer_control (xcb_connection_t *c  );

xcb_get_pointer_control_cookie_t
xcb_get_pointer_control_unchecked (xcb_connection_t *c  );

xcb_get_pointer_control_reply_t *
xcb_get_pointer_control_reply (xcb_connection_t                  *c  ,
                               xcb_get_pointer_control_cookie_t   cookie  ,
                               xcb_generic_error_t              **e  );

xcb_void_cookie_t
xcb_set_screen_saver_checked (xcb_connection_t *c  ,
                              int16_t           timeout  ,
                              int16_t           interval  ,
                              uint8_t           prefer_blanking  ,
                              uint8_t           allow_exposures  );

xcb_void_cookie_t
xcb_set_screen_saver (xcb_connection_t *c  ,
                      int16_t           timeout  ,
                      int16_t           interval  ,
                      uint8_t           prefer_blanking  ,
                      uint8_t           allow_exposures  );

xcb_get_screen_saver_cookie_t
xcb_get_screen_saver (xcb_connection_t *c  );

xcb_get_screen_saver_cookie_t
xcb_get_screen_saver_unchecked (xcb_connection_t *c  );

xcb_get_screen_saver_reply_t *
xcb_get_screen_saver_reply (xcb_connection_t               *c  ,
                            xcb_get_screen_saver_cookie_t   cookie  ,
                            xcb_generic_error_t           **e  );

int
xcb_change_hosts_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_change_hosts_checked (xcb_connection_t *c  ,
                          uint8_t           mode  ,
                          uint8_t           family  ,
                          uint16_t          address_len  ,
                          const uint8_t    *address  );

xcb_void_cookie_t
xcb_change_hosts (xcb_connection_t *c  ,
                  uint8_t           mode  ,
                  uint8_t           family  ,
                  uint16_t          address_len  ,
                  const uint8_t    *address  );

int
xcb_host_sizeof (const void  *_buffer  );

uint8_t *
xcb_host_address (const xcb_host_t *R  );

int
xcb_host_address_length (const xcb_host_t *R  );

xcb_generic_iterator_t
xcb_host_address_end (const xcb_host_t *R  );

void
xcb_host_next (xcb_host_iterator_t *i  );

xcb_generic_iterator_t
xcb_host_end (xcb_host_iterator_t i  );

int
xcb_list_hosts_sizeof (const void  *_buffer  );

xcb_list_hosts_cookie_t
xcb_list_hosts (xcb_connection_t *c  );

xcb_list_hosts_cookie_t
xcb_list_hosts_unchecked (xcb_connection_t *c  );

int
xcb_list_hosts_hosts_length (const xcb_list_hosts_reply_t *R  );

xcb_host_iterator_t
xcb_list_hosts_hosts_iterator (const xcb_list_hosts_reply_t *R  );

xcb_list_hosts_reply_t *
xcb_list_hosts_reply (xcb_connection_t         *c  ,
                      xcb_list_hosts_cookie_t   cookie  ,
                      xcb_generic_error_t     **e  );

xcb_void_cookie_t
xcb_set_access_control_checked (xcb_connection_t *c  ,
                                uint8_t           mode  );

xcb_void_cookie_t
xcb_set_access_control (xcb_connection_t *c  ,
                        uint8_t           mode  );

xcb_void_cookie_t
xcb_set_close_down_mode_checked (xcb_connection_t *c  ,
                                 uint8_t           mode  );

xcb_void_cookie_t
xcb_set_close_down_mode (xcb_connection_t *c  ,
                         uint8_t           mode  );

xcb_void_cookie_t
xcb_kill_client_checked (xcb_connection_t *c  ,
                         uint32_t          resource  );

xcb_void_cookie_t
xcb_kill_client (xcb_connection_t *c  ,
                 uint32_t          resource  );

int
xcb_rotate_properties_sizeof (const void  *_buffer  );

xcb_void_cookie_t
xcb_rotate_properties_checked (xcb_connection_t *c  ,
                               xcb_window_t      window  ,
                               uint16_t          atoms_len  ,
                               int16_t           delta  ,
                               const xcb_atom_t *atoms  );

xcb_void_cookie_t
xcb_rotate_properties (xcb_connection_t *c  ,
                       xcb_window_t      window  ,
                       uint16_t          atoms_len  ,
                       int16_t           delta  ,
                       const xcb_atom_t *atoms  );

xcb_void_cookie_t
xcb_force_screen_saver_checked (xcb_connection_t *c  ,
                                uint8_t           mode  );

xcb_void_cookie_t
xcb_force_screen_saver (xcb_connection_t *c  ,
                        uint8_t           mode  );

int
xcb_set_pointer_mapping_sizeof (const void  *_buffer  );

xcb_set_pointer_mapping_cookie_t
xcb_set_pointer_mapping (xcb_connection_t *c  ,
                         uint8_t           map_len  ,
                         const uint8_t    *map  );

xcb_set_pointer_mapping_cookie_t
xcb_set_pointer_mapping_unchecked (xcb_connection_t *c  ,
                                   uint8_t           map_len  ,
                                   const uint8_t    *map  );

xcb_set_pointer_mapping_reply_t *
xcb_set_pointer_mapping_reply (xcb_connection_t                  *c  ,
                               xcb_set_pointer_mapping_cookie_t   cookie  ,
                               xcb_generic_error_t              **e  );

int
xcb_get_pointer_mapping_sizeof (const void  *_buffer  );

xcb_get_pointer_mapping_cookie_t
xcb_get_pointer_mapping (xcb_connection_t *c  );

xcb_get_pointer_mapping_cookie_t
xcb_get_pointer_mapping_unchecked (xcb_connection_t *c  );

uint8_t *
xcb_get_pointer_mapping_map (const xcb_get_pointer_mapping_reply_t *R  );

int
xcb_get_pointer_mapping_map_length (const xcb_get_pointer_mapping_reply_t *R  );

xcb_generic_iterator_t
xcb_get_pointer_mapping_map_end (const xcb_get_pointer_mapping_reply_t *R  );

xcb_get_pointer_mapping_reply_t *
xcb_get_pointer_mapping_reply (xcb_connection_t                  *c  ,
                               xcb_get_pointer_mapping_cookie_t   cookie  ,
                               xcb_generic_error_t              **e  );

int
xcb_set_modifier_mapping_sizeof (const void  *_buffer  );

xcb_set_modifier_mapping_cookie_t
xcb_set_modifier_mapping (xcb_connection_t    *c  ,
                          uint8_t              keycodes_per_modifier  ,
                          const xcb_keycode_t *keycodes  );

xcb_set_modifier_mapping_cookie_t
xcb_set_modifier_mapping_unchecked (xcb_connection_t    *c  ,
                                    uint8_t              keycodes_per_modifier  ,
                                    const xcb_keycode_t *keycodes  );

xcb_set_modifier_mapping_reply_t *
xcb_set_modifier_mapping_reply (xcb_connection_t                   *c  ,
                                xcb_set_modifier_mapping_cookie_t   cookie  ,
                                xcb_generic_error_t               **e  );

int
xcb_get_modifier_mapping_sizeof (const void  *_buffer  );

xcb_get_modifier_mapping_cookie_t
xcb_get_modifier_mapping (xcb_connection_t *c  );

xcb_get_modifier_mapping_cookie_t
xcb_get_modifier_mapping_unchecked (xcb_connection_t *c  );

xcb_keycode_t *
xcb_get_modifier_mapping_keycodes (const xcb_get_modifier_mapping_reply_t *R  );

int
xcb_get_modifier_mapping_keycodes_length (const xcb_get_modifier_mapping_reply_t *R  );

xcb_generic_iterator_t
xcb_get_modifier_mapping_keycodes_end (const xcb_get_modifier_mapping_reply_t *R  );

xcb_get_modifier_mapping_reply_t *
xcb_get_modifier_mapping_reply (xcb_connection_t                   *c  ,
                                xcb_get_modifier_mapping_cookie_t   cookie  ,
                                xcb_generic_error_t               **e  );

xcb_void_cookie_t
xcb_no_operation_checked (xcb_connection_t *c  );

xcb_void_cookie_t
xcb_no_operation (xcb_connection_t *c  );


#ifdef __cplusplus
}
#endif

#endif
