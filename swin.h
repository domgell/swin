#ifndef SWIN_H
#define SWIN_H

#ifndef SWIN_NO_TYPEDEF
#include <stdbool.h>
#include <stdint.h>
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;
typedef int64_t i64;
typedef int32_t i32;
typedef float f32;
typedef double f64;
#endif

// Infer platform used for windowing
// Possible values:
// SWIN_PLATFORM_WIN32, SWIN_PLATFORM_MACOS, SWIN_PLATFORM_X11
#ifndef SWIN_NO_PLATFORM_INFER
#if defined(_WIN32)
#define SWIN_PLATFORM_WIN32
#elif defined(__APPLE__) && defined(__MACH__)
#define SWIN_PLATFORM_MACOS
#elif defined(__linux__)
#define SWIN_PLATFORM_X11
#else
#error "Cannot infer platform. Define SWIN_NO_PLATFORM_INFER to set platform manually."
#endif
#endif

// All public functions are prefixed with SWIN_FN
#ifndef SWIN_FN
#ifdef SWIN_PLATFORM_WIN32
#ifdef __cplusplus
#define SWIN_FN extern "C" __declspec(dllexport)
#else
#define SWIN_FN __declspec(dllexport)
#endif
#elif defined(SWIN_PLATFORM_WAYLAND) || defined(SWIN_PLATFORM_X11) || defined(SWIN_PLATFORM_MACOS)
#ifdef __cplusplus
#define SWIN_FN extern "C" __attribute__((visibility("default")))
#else
#define SWIN_FN __attribute__((visibility("default")))
#endif
#endif
#endif

// TODO: window size should always be drawable surface excluding topbar
// TODO: allow disabling swinDraw to avoid linking with gdi32 on win32

typedef struct Swin_t* Swin;

typedef u32 SwinWindowFlagBits;
typedef u32 SwinMouseFlagBits;

typedef struct {
    // Window width, defaults to 400 if not provided
    u32 width;
    // Window height, defaults to 300 if not provided
    u32 height;
    // Window title, defaults to "swin" if not provided
    const char* title;
    // Window flags applied at startup
    SwinWindowFlagBits windowFlags;
    // Mouse flags applied at startup
    SwinMouseFlagBits mouseFlags;
    // TODO: initial window position
    // SwinPoint position;
} SwinInitInfo;

SWIN_FN Swin swinInit(SwinInitInfo info);
SWIN_FN void swinQuit(Swin swin);

// Update window and input events. Should be called once per frame.
// Returns true while window is active.
// Returns false when window wants to close.
SWIN_FN bool swinPoll(Swin swin);

// CPU-copy pixels directly onto screen buffer without external graphics APIs.
// `pixels` is an array of RGBA8UNORM pixels with at least window width * height elements.
// Alpha component is ignored
SWIN_FN void swinDraw(Swin swin, const u32* pixels);

typedef struct {
    u32 x;
    u32 y;
} SwinSize;

typedef struct {
    i32 x;
    i32 y;
} SwinPoint;

/* --------------------------------- :Input --------------------------------- */

typedef enum {
    SwinKey_Esc = 0x1B,
    SwinKey_CtrlLeft = 0xA2,
    SwinKey_CtrlRight = 0xA3,
    SwinKey_ShiftLeft = 0xA0,
    SwinKey_ShiftRight = 0xA1,
    SwinKey_ArrowLeft = 0x25,
    SwinKey_ArrowRight = 0x27,
    SwinKey_ArrowUp = 0x26,
    SwinKey_ArrowDown = 0x28,
    SwinKey_Space = 0x20,
} SwinKey;

// Either direct `SwinKey` enum value or char value for letters.
// Example usage: `swinIsKeyDown(swin, 'A')` or `swinIsKeyDown(swin, 'a')`
typedef u32 SwinKeyCode;

typedef enum {
    SwinMouseButton_Left,
    SwinMouseButton_Right,
    SwinMouseButton_Middle,
    SwinMouseButton_Side1,
    SwinMouseButton_Side2,
} SwinMouseButton;

typedef struct {
    i32 mouseX;
    i32 mouseY;
    i32 mouseDeltaX;
    i32 mouseDeltaY;
    i32 scrollDelta;
    bool keys[256];
    bool mouse[5];
} SwinInput;

// Get the raw input state this frame.
SWIN_FN SwinInput swinInput(Swin swin);
// Get the raw input state of the previous frame.
SWIN_FN SwinInput swinPrevInput(Swin swin);

SWIN_FN bool swinIsKeyDown(Swin swin, SwinKeyCode key);
SWIN_FN bool swinIsMouseDown(Swin swin, SwinMouseButton button);

// Returns true only for the frame that the key was pressed
SWIN_FN bool swinIsKeyJustPressed(Swin swin, SwinKeyCode key);
// Returns true only for the frame that the key was released
SWIN_FN bool swinIsKeyJustReleased(Swin swin, SwinKeyCode key);
// Returns true only for the frame that the mouse button was pressed
SWIN_FN bool swinIsMouseJustPressed(Swin swin, SwinMouseButton button);
// Returns true only for the frame that the mouse button was released
SWIN_FN bool swinIsMouseJustReleased(Swin swin, SwinMouseButton button);
// Map the `forward`, `left`, `backward` and `right` keys to a direction.
// The resulting direction is *not* normalized.
// Example usage: `SwinPoint move = swinMapKeys(swin, 'W', 'A', 'S', 'D')`
SWIN_FN SwinPoint swinMapKeys(Swin swin, SwinKeyCode forward, SwinKeyCode left, SwinKeyCode back, SwinKeyCode right);

typedef enum {
    SwinMouse_Default = 0,
    // Cursor is invisible
    SwinMouse_Hidden = 1 << 1,
    // Cannot exit window bounds
    SwinMouse_Capture = 1 << 2,
    // Locked in place (still updates mouse delta)
    SwinMouse_Locked = 1 << 3,
} SwinMouseFlag;

SWIN_FN void swinSetMouseFlags(Swin swin, SwinMouseFlagBits flags);
SWIN_FN SwinMouseFlagBits swinGetMouseFlags(Swin swin);

// Set mouse/cursor position to the window coordinate
// TODO: When using X11 under XWayland,
//  mouse position can only be set while `SwinMouse_Hidden` is set :-(
//  https://github.com/libsdl-org/SDL/issues/9539
SWIN_FN void swinSetMousePosition(Swin swin, SwinPoint position);
SWIN_FN SwinPoint swinGetMousePosition(Swin swin);
SWIN_FN SwinPoint swinGetMouseDelta(Swin swin);

/* --------------------------------- :Window -------------------------------- */

typedef enum {
    SwinWindow_Default = 0,
    SwinWindow_Fullscreen = 1 << 0,
    // Window can be resized by the user.
    SwinWindow_Resizable = 1 << 1,
    // Window is always on top of other windows.
    SwinWindow_Topmost = 1 << 2,
    // Window has no top menu bar.
    SwinWindow_NoTopbar = 1 << 5,
    // TODO: not implemented
    // SwinWindow_Transparent = 1 << 6
} SwinWindowFlag;

typedef u32 SwinWindowFlagBits;

SWIN_FN void swinSetWindowFlags(Swin swin, SwinWindowFlagBits flags);
SWIN_FN SwinWindowFlagBits swinGetWindowFlags(Swin swin);
SWIN_FN SwinSize swinGetWindowSize(Swin swin);
SWIN_FN void swinSetWindowSize(Swin swin, SwinSize size);
// Returns true if the window resized until the next call to `swinIsWindowResized()`.
SWIN_FN bool swinIsWindowResized(Swin swin);
SWIN_FN SwinPoint swinGetWindowPosition(Swin swin);
SWIN_FN void swinSetWindowPosition(Swin swin, SwinPoint position);
SWIN_FN void swinSetWindowTitle(Swin swin, const char* title);

SWIN_FN SwinPoint swinWindowToScreen(Swin swin, SwinPoint point);
SWIN_FN SwinPoint swinScreenToWindow(Swin swin, SwinPoint point);

// TODO: not implemented
// typedef enum {
// SwinWindowEvent_Minimize,
// SwinWindowEvent_Maximize,
// SwinWindowEvent_Focus,
// } SwinWindowEvent;
// void swinDoWindowEvent(Swin swin, SwinWindowEvent event);

/* ---------------------------------- :Time --------------------------------- */

// Returns the time since the window was created in miliseconds
SWIN_FN f32 swinTime(Swin swin);
SWIN_FN f32 swinDeltaTime(Swin swin);

// TODO: not implemented
// Returns the time since the window was created in nanoseconds
// SWIN_FN u64 swinTimeNano(Swin swin);

// -------------------------------------- :Screen --------------------------------------

// TODO: not implemented
// SwinSize swinGetScreenSize(Swin swin, u32 screenIndex);
// u32 swinGetCurrentScreen(Swin swin);
// u32 swinGetScreenCount(Swin swin);
// void swinSetScreen(u32 screenIndex);

// ------------------------------------- :Platform -------------------------------------

typedef enum {
    SwinPlatform_Win32,
    SwinPlatform_X11,
    SwinPlatform_MacOS,
} SwinPlatform;

typedef struct {
    void* instance; // HINSTANCE
    void* wnd; // HWND
} SwinWin32State;

typedef struct {
    void* display; // Display*
    u64 window; // Window
} SwinX11State;

typedef struct {
    // TODO: not implemented
} SwinMacOSState;

typedef struct {
    SwinPlatform platform;

    union {
        SwinWin32State win32;
        SwinX11State x11;
        SwinMacOSState macos;
    };
} SwinPlatformState;

SwinPlatformState swinGetPlatformState(Swin swin);

// ------------------------------------ :Deprecated ------------------------------------

// *DEPRECATED* use `SwinKey` enum with `SwinKey_` prefix
#define Swin_Esc SwinKey_Esc
#define Swin_CtrlLeft SwinKey_CtrlLeft
#define Swin_CtrlRight SwinKey_CtrlRight
#define Swin_ShiftLeft SwinKey_ShiftLeft
#define Swin_ShiftRight SwinKey_ShiftRight
#define Swin_ArrowLeft SwinKey_ArrowLeft
#define Swin_ArrowRight SwinKey_ArrowRight
#define Swin_ArrowUp SwinKey_ArrowUp
#define Swin_ArrowDown SwinKey_ArrowDown
#define Swin_Space SwinKey_Space

// *DEPRECATED* use `SwinMouseButton` enum with `SwinMouseButton_` prefix
#define Swin_MouseLeft SwinMouseButton_Left
#define Swin_MouseRight SwinMouseButton_Right
#define Swin_MouseMiddle SwinMouseButton_Middle

// *DERECATED* use `SwinGetPlatformState()`
SWIN_FN SwinWin32State swinGetWin32State(Swin swin);
SWIN_FN SwinX11State swinGetX11State(Swin swin);

// --------------------------------------- :Impl ---------------------------------------

#ifdef SWIN_IMPL

#if !defined(SWIN_MALLOC) || !defined(SWIN_FREE)
#include <stdlib.h>
#define SWIN_MALLOC malloc
#define SWIN_FREE free
#endif

#ifndef SWIN_ASSERT
#include <assert.h>
#define SWIN_ASSERT assert
#endif

#include "impl/swin_common.h"

// Platform specific implementation
#if defined(SWIN_PLATFORM_WIN32)
#include "impl/swin_win32.h"
#elif defined(SWIN_PLATFORM_X11)
#include "impl/swin_x11.h"
#elif defined(SWIN_PLATFORM_MACOS)
#import "impl/swin_macos.m"
#endif

// TODO: macOS not implemented

#endif // SWIN_IMPL
#endif // SWIN_H