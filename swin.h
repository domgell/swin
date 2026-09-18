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
// SWIN_WIN32, SWIN_MACOS, SWIN_WAYLAND, SWIN_X11
#if defined(_WIN32)
#define SWIN_WIN32
#elif defined(__APPLE__) && defined(__MACH__)
#define SWIN_MACOS
#elif defined(__linux__) && !defined(SWIN_WAYLAND)
#define SWIN_X11
#else
#error "Unsupported platform"
#endif

// All public functions are prefixed with SWIN_FN
#ifndef SWIN_FN
#ifdef SWIN_WIN32
#ifdef __cplusplus
#define SWIN_FN extern "C" __declspec(dllexport)
#else
#define SWIN_FN __declspec(dllexport)
#endif
#elif defined(SWIN_WAYLAND) || defined(SWIN_X11) || defined(SWIN_MACOS)
#ifdef __cplusplus
#define SWIN_FN extern "C" __attribute__((visibility("default")))
#else
#define SWIN_FN __attribute__((visibility("default")))
#endif
#endif
#endif

// TODO: window size should always be drawable surface excluding topbar

typedef struct Swin_t* Swin;

typedef struct {
    // TODO: default width, height if not provided
    u32 width;
    u32 height;
    const char* title;
    // Window flags applied at startup
    u32 windowFlags;
    // Mouse flags applied at startup
    u32 mouseFlags;
} SwinInitInfo;

SWIN_FN Swin swinInit(SwinInitInfo info);
SWIN_FN void swinQuit(Swin swin);

SWIN_FN bool swinPoll(Swin swin);

// typedef enum {
// SwinPixel_RGBA8unorm,
// SwinPixel_RGBA8srgb,
// SwinPixel_RGB8unorm,
// SwinPixel_BGRA8unorm,
// SwinPixel_BGR8unorm,
// } SwinPixelFormat;

SWIN_FN void swinDraw(Swin swin, const u32* pixels);

// TODO: either, control vsync?
// bool swinFlip(Swin swin);
// bool swinWindowSync(Swin swin);

typedef struct {
    u32 x;
    u32 y;
} SwinSize;

typedef struct {
    i32 x;
    i32 y;
} SwinPoint;

/* --------------------------------- :Input --------------------------------- */

// TEMP
typedef enum {
    Swin_Esc = 0x1B,
    Swin_CtrlLeft = 0xA2,
    Swin_CtrlRight = 0xA3,
    Swin_ShiftLeft = 0xA0,
    Swin_ShiftRight = 0xA1,
    Swin_ArrowLeft = 0x25,
    Swin_ArrowRight = 0x27,
    Swin_ArrowUp = 0x26,
    Swin_ArrowDown = 0x28,
} SwinKey;

typedef enum {
    Swin_MouseLeft,
    Swin_MouseRight,
    Swin_MouseMiddle,
    // Swin_Mouse4,
    // Swin_Mouse5,
} SwinMouseButton;

// TODO: only internal
// use `void swinGetKeyState(bool outKeys*)` `SWIN_MAX_KEYS`
typedef struct {
    i32 mouseX;
    i32 mouseY;
    // Raw mouse input
    i32 mouseDeltaX;
    i32 mouseDeltaY;
    i32 scrollDelta;
    bool keys[256];
    bool mouse[5];
} SwinInput;

SWIN_FN SwinInput swinInput(Swin swin);

SWIN_FN bool swinIsKeyDown(Swin swin, u32 key);
SWIN_FN bool swinIsMouseDown(Swin swin, u8 button);
// Map the keys forward, left, backward and right to a direction.
// The resulting direction is *not* normalized.
SWIN_FN SwinPoint swinMapKeys(Swin swin, u8 forward, u8 left, u8 backward, u8 right);

typedef enum {
    SwinMouse_Default = 0,
    // Hidden when within window bounds
    SwinMouse_Hidden = 1 << 1,
    // Cannot exit window bounds
    SwinMouse_Capture = 1 << 2,
    // Locked in place (still updates mouse delta input)
    SwinMouse_Locked = 1 << 3,
} SwinMouseFlag;

typedef u32 SwinMouseFlagBits;

SWIN_FN void swinSetMouseFlags(Swin swin, SwinMouseFlagBits flags);

SWIN_FN void swinSetMousePosition(Swin swin, SwinPoint position);
SWIN_FN SwinPoint swinGetMousePosition(Swin swin);
SWIN_FN SwinPoint swinGetMouseDelta(Swin swin);

/* --------------------------------- :Window -------------------------------- */

typedef enum {
    SwinWindow_Default = 0,
    SwinWindow_Fullscreen = 1 << 0,
    SwinWindow_Resizable = 1 << 1,
    // Window is always on top of other windows.
    // *Not supported on Wayland*
    SwinWindow_Topmost = 1 << 2,
    SwinWindow_NoTopbar = 1 << 5,
} SwinWindowFlag;

typedef u32 SwinWindowFlagBits;

SWIN_FN void swinSetWindowFlags(Swin swin, SwinWindowFlagBits flags);
SWIN_FN SwinSize swinGetWindowSize(Swin swin);
SWIN_FN void swinSetWindowSize(Swin swin, SwinSize size);
SWIN_FN bool swinIsWindowResized(Swin swin);
SWIN_FN SwinPoint swinGetWindowPosition(Swin swin);
SWIN_FN void swinSetWindowPosition(Swin swin, SwinPoint position);
SWIN_FN void swinSetWindowTitle(Swin swin, const char* title);

SWIN_FN SwinPoint swinWindowToScreen(Swin swin, SwinPoint point);
SWIN_FN SwinPoint swinScreenToWindow(Swin swin, SwinPoint point);

/* ---------------------------------- :Time --------------------------------- */

SWIN_FN f32 swinTime(Swin swin);
SWIN_FN f32 swinDeltaTime(Swin swin);
// u64 swinTimeNS(Swin swin);

// typedef enum {
// SwinWindowEvent_Minimize,
// SwinWindowEvent_Maximize,
// SwinWindowEvent_Focus,
// } SwinWindowEvent;

// void swinDoWindowEvent(Swin swin, SwinWindowEvent event);

// SwinSize swinGetScreenSize(Swin swin, u32 screenIndex);
// u32 swinGetCurrentScreen(Swin swin);
// u32 swinGetScreenCount(Swin swin);
// void swinSetScreen(u32 screenIndex);

// ------------------------------------- :Platform -------------------------------------

#include "platform/swin_common.h"

#if defined(SWIN_WIN32)
#include "platform/swin_win32.h"
#elif defined(SWIN_X11)
#include "platform/swin_x11.h"
#elif defined(SWIN_WAYLAND)
#include "platform/swin_wayland.h"
#endif

#endif