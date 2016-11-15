/*
*--------------------------------------------------------------------------
* Copyright (c) 2015-2016 Valve Corporation
* Copyright (c) 2015-2016 LunarG, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Rene Lindsay <rene@lunarg.com>
* (See: http://www.freebsddiary.org/APC/usb_hid_usages.php)
*
*--------------------------------------------------------------------------
*
* These are the standard, platform-independent USB HID keyboard codes,
* as defined at: http://www.freebsddiary.org/APC/usb_hid_usages.php  
*
* These Keycodes are returned by the OnKeyEvent, in the WSIWindow class, 
* whenever a key is pressed or released.
* In order to provide consistent results across all platforms, the WSIWindow
* class converts the native platform-specific scancodes to these cross-platform
* USB HID codes, before returning it in the OnKeyEvent.
*
* The layout of these keycodes are fixed, and the names correspond to a
* US keyboard layout. Unlike the key symbols, these keycodes do not change 
* with international keyboard layout settings.
*
* eg. KEY_Z corresponds to the lower left key, which produces a 'z' character
* on US and UK qwerty keyboards, but a 'y' on German keyboards, and a ';' on 
* dvorak keyboads, even though it is the same physical key.
*
* Therefore, use these keycodes for game controls, to ensure a consistent layout,
* but when text input is required, use the OnTextEvent instead, to get the correct
* text symbol, according to the current configured keyboard layout settings.
*/

#ifndef KEYCODE_H
#define KEYCODE_H
enum eKeycode{
    KEY_NONE = 0,           //Undefined. (No event)
    KEY_A = 4,
    KEY_B = 5,
    KEY_C = 6,
    KEY_D = 7,
    KEY_E = 8,
    KEY_F = 9,
    KEY_G = 10,
    KEY_H = 11,
    KEY_I = 12,
    KEY_J = 13,
    KEY_K = 14,
    KEY_L = 15,
    KEY_M = 16,
    KEY_N = 17,
    KEY_O = 18,
    KEY_P = 19,
    KEY_Q = 20,
    KEY_R = 21,
    KEY_S = 22,
    KEY_T = 23,
    KEY_U = 24,
    KEY_V = 25,
    KEY_W = 26,
    KEY_X = 27,
    KEY_Y = 28,
    KEY_Z = 29,
    KEY_1 = 30,             // 1 and !
    KEY_2 = 31,             // 2 and @
    KEY_3 = 32,             // 3 and #
    KEY_4 = 33,             // 4 and $
    KEY_5 = 34,             // 5 and %
    KEY_6 = 35,             // 6 and ^
    KEY_7 = 36,             // 7 and &
    KEY_8 = 37,             // 8 and *
    KEY_9 = 38,             // 9 and (
    KEY_0 = 39,             // 0 and )
    KEY_Enter  = 40,        // (Return)
    KEY_Escape = 41,
    KEY_Delete = 42,
    KEY_Tab    = 43,
    KEY_Space  = 44,
    KEY_Minus  = 45,        // - and (underscore)
    KEY_Equals = 46,        // = and +
    KEY_LeftBracket  = 47,  // [ and {
    KEY_RightBracket = 48,  // ] and } 
    KEY_Backslash = 49,     // \ and |
  //KEY_NonUSHash = 50,     // # and ~
    KEY_Semicolon = 51,     // ; and :
    KEY_Quote     = 52,     // ' and " 
    KEY_Grave     = 53,
    KEY_Comma     = 54,     // , and <
    KEY_Period    = 55,     // . and >
    KEY_Slash     = 56,     // / and ?
    KEY_CapsLock  = 57,
    KEY_F1  = 58,
    KEY_F2  = 59,
    KEY_F3  = 60,
    KEY_F4  = 61,
    KEY_F5  = 62,
    KEY_F6  = 63,
    KEY_F7  = 64,
    KEY_F8  = 65,
    KEY_F9  = 66,
    KEY_F10 = 67,
    KEY_F11 = 68,
    KEY_F12 = 69,
    KEY_PrintScreen = 70,
    KEY_ScrollLock = 71,
    KEY_Pause  = 72,
    KEY_Insert = 73,
    KEY_Home   = 74,
    KEY_PageUp = 75,
    KEY_DeleteForward = 76,
    KEY_End = 77,
    KEY_PageDown = 78,
    KEY_Right = 79,         // Right arrow
    KEY_Left  = 80,         // Left arrow
    KEY_Down  = 81,         // Down arrow
    KEY_Up    = 82,         // Up arrow
    KP_NumLock = 83,
    KP_Divide = 84,
    KP_Multiply = 85,
    KP_Subtract = 86,
    KP_Add = 87,
    KP_Enter = 88,
    KP_1 = 89,
    KP_2 = 90,
    KP_3 = 91,
    KP_4 = 92,
    KP_5 = 93,
    KP_6 = 94,
    KP_7 = 95,
    KP_8 = 96,
    KP_9 = 97,
    KP_0 = 98,
    KP_Point = 99,             // . and Del
  //KEY_NonUSBackslash = 100,  // \ and |
    KP_Equals = 103,
    KEY_F13 = 104,
    KEY_F14 = 105,
    KEY_F15 = 106,
    KEY_F16 = 107,
    KEY_F17 = 108,
    KEY_F18 = 109,
    KEY_F19 = 110,
    KEY_F20 = 111,
    KEY_F21 = 112,
    KEY_F22 = 113,
    KEY_F23 = 114,
    KEY_F24 = 115,
  //KEY_Help = 117,
    KEY_Menu = 118,
    KEY_Mute       = 127,
    KEY_VolumeUp   = 128,
    KEY_VolumeDown = 129,
    KEY_LeftControl = 224,  //WARNING : Android has no Ctrl keys.
    KEY_LeftShift   = 225,
    KEY_LeftAlt     = 226,
    KEY_LeftGUI     = 227,
    KEY_RightControl= 228,
    KEY_RightShift  = 229,  //WARNING : Win32 fails to send a WM_KEYUP message if both shift keys are pressed, and then one is released.
    KEY_RightAlt    = 230,
    KEY_RightGUI    = 231
};
#endif
