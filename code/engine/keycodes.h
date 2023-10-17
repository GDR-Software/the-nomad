#ifndef __KEYCODES__
#define __KEYCODES__

#pragma once

typedef enum {
    KEY_MOUSE_LEFT = 1,
    KEY_MOUSE_MIDDLE,
    KEY_MOUSE_RIGHT,

    KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,

    KEY_ENTER,
    KEY_ESCAPE,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_SPACE,

    KEY_MINUS,
    KEY_EQUALS,
    KEY_LEFTBRACKET,
    KEY_RIGHTBRACKET,
    KEY_BACKSLASH,
    KEY_ISO_USB_BACKSLASH,
    KEY_SEMICOLON,
    KEY_APOSTROPHE,
    KEY_CONSOLE,
    KEY_COMMA,
    KEY_PERIOD,
    KEY_SLASH,
    KEY_CAPSLOCK,

    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,

    KEY_SCREENSHOT,
    KEY_SCROLLLOCK,
    KEY_PAUSE,
    KEY_INSERT,
    KEY_HOME,
    KEY_PAGEUP,
    KEY_DELETE,
    KEY_END,
    KEY_PAGEDOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_DOWN,
    KEY_UP,

    KEY_NUMLOCKCLEAR, /**< num lock on PC, clear on Mac keyboards
                                     */
    KEY_KP_DIVIDE,
    KEY_KP_MULTIPLY,
    KEY_KP_MINUS,
    KEY_KP_PLUS,
    KEY_KP_ENTER,
    KEY_KP_1,
    KEY_KP_2,
    KEY_KP_3,
    KEY_KP_4,
    KEY_KP_5,
    KEY_KP_6,
    KEY_KP_7,
    KEY_KP_8,
    KEY_KP_9,
    KEY_KP_0,
    KEY_KP_PERIOD,

    KEY_KP_EQUALS = 103,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,
    KEY_EXECUTE,
    KEY_HELP,
    KEY_MENU,
    KEY_SELECT,
    KEY_STOP,
    KEY_AGAIN,
    KEY_UNDO,
    KEY_CUT,
    KEY_COPY,
    KEY_PASTE,
    KEY_FIND,
    KEY_MUTE,
    KEY_VOLUMEUP,
    KEY_VOLUMEDOWN,

    KEY_KP_COMMA = 133,
    KEY_KP_EQUALSAS400,

    KEY_KP_00 = 176,
    KEY_KP_000,
    KEY_THOUSANDSSEPARATOR,
    KEY_DECIMALSEPARATOR,
    KEY_CURRENCYUNIT,
    KEY_CURRENCYSUBUNIT,
    KEY_KP_LEFTPAREN,
    KEY_KP_RIGHTPAREN,
    KEY_KP_LEFTBRACE,
    KEY_KP_RIGHTBRACE,
    KEY_KP_TAB,
    KEY_KP_BACKSPACE,
    KEY_KP_A,
    KEY_KP_B,
    KEY_KP_C,
    KEY_KP_D,
    KEY_KP_E,
    KEY_KP_F,
    KEY_KP_XOR,
    KEY_KP_POWER,
    KEY_KP_PERCENT,
    KEY_KP_LESS,
    KEY_KP_GREATER,
    KEY_KP_AMPERSAND,
    KEY_KP_DBLAMPERSAND,
    KEY_KP_VERTICALBAR,
    KEY_KP_DBLVERTICALBAR,
    KEY_KP_COLON,
    KEY_KP_HASH,
    KEY_KP_SPACE,
    KEY_KP_AT,
    KEY_KP_EXCLAM,
    KEY_KP_MEMSTORE,
    KEY_KP_MEMRECALL,
    KEY_KP_MEMCLEAR,
    KEY_KP_MEMADD,
    KEY_KP_MEMSUBTRACT,
    KEY_KP_MEMMULTIPLY,
    KEY_KP_MEMDIVIDE,
    KEY_KP_PLUSMINUS,
    KEY_KP_CLEAR,
    KEY_KP_CLEARENTRY,
    KEY_KP_BINARY,
    KEY_KP_OCTAL,
    KEY_KP_DECIMAL,
    KEY_KP_HEXADECIMAL,

    KEY_LCTRL = 224,
    KEY_LSHIFT,
    KEY_LALT,
    KEY_RCTRL = 228,
    KEY_RSHIFT,
    KEY_RALT,

    KEY_AUDIONEXT = 258,
    KEY_AUDIOPREV,
    KEY_AUDIOSTOP,
    KEY_AUDIOPLAY,
    KEY_AUDIOMUTE,

    KEY_BRIGHTNESSDOWN = 275,
    KEY_BRIGHTNESSUP,

    KEY_EJECT = 281,
    KEY_SLEEP,

    KEY_WHEEL_UP,
    KEY_WHEEL_DOWN,

    NUMKEYS
} keynum_t;

// The menu code needs to get both key and char events, but
// to avoid duplicating the paths, the char events are just
// distinguished by or'ing in K_CHAR_FLAG (ugly)
#define	K_CHAR_FLAG		1024

#endif
