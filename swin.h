#pragma once

#include <stdint.h>
#define u64 uint64_t
#define u32 uint32_t
#define u8 uint8_t
#define i32 int32_t
#define f32 float

// Infer platform used for windowing
// Possible values:
// SWIN_WIN32, SWIN_MACOS, SWIN_WAYLAND
#if defined(_WIN32)
#define SWIN_WIN32
#elif defined(__APPLE__) && defined(__MACH__)
#define SWIN_MACOS
#elif defined(__linux__)
#define SWIN_WAYLAND
#else
#error "Unsupported platform"
#endif

// TODO: window size should always be drawable surface excluding topbar

typedef struct Swin_t* Swin;

typedef struct {
    u32 width;
    u32 height;
    const char* title;
    // Window flags applied at startup
    u32 windowFlags;
    // Mouse flags applied at startup
    u32 mouseFlags;
} SwinInitInfo;

Swin swinInit(SwinInitInfo info);
void swinQuit(Swin swin);

bool swinPoll(Swin swin);

// typedef enum {
// SwinPixel_RGBA8unorm,
// SwinPixel_RGBA8srgb,
// SwinPixel_RGB8unorm,
// SwinPixel_BGRA8unorm,
// SwinPixel_BGR8unorm,
// } SwinPixelFormat;

void swinDraw(Swin swin, const u32* pixels);

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

SwinInput swinInput(Swin swin);

bool swinIsKeyDown(Swin swin, u32 key);
bool swinIsMouseDown(Swin swin, u8 button);

typedef enum {
    SwinMouse_Default = 0,
    // Hide mouse cursor within window bounds
    SwinMouse_Hidden = 1 << 1,
    // Cannot exit window bounds
    SwinMouse_Capture = 1 << 2,
    // Locked in place (still updates mouse delta input)
    SwinMouse_Locked = 1 << 3,
} SwinMouseFlag;

typedef u32 SwinMouseFlagBits;

void swinSetMouseFlags(Swin swin, SwinMouseFlagBits flags);

void swinSetMousePosition(Swin swin, SwinPoint position);
SwinPoint swinGetMousePosition(Swin swin);
SwinPoint swinGetMouseDelta(Swin swin);

/* --------------------------------- :Window -------------------------------- */

typedef enum {
    SwinWindow_Default = 0,
    SwinWindow_Fullscreen = 1 << 0,
    SwinWindow_Resizable = 1 << 1,
    SwinWindow_Topmost = 1 << 2,
    // SwinWindow_NoTopbar = 1 << 5,
} SwinWindowFlag;

typedef u32 SwinWindowFlagBits;

void swinSetWindowFlags(Swin swin, SwinWindowFlagBits flags);
SwinSize swinGetWindowSize(Swin swin);
void swinSetWindowSize(Swin swin, SwinSize size);
bool swinIsWindowResized(Swin swin);
SwinPoint swinGetWindowPosition(Swin swin);
void swinSetWindowPosition(Swin swin, SwinPoint position);
void swinSetWindowTitle(Swin swin, const char* title);

SwinPoint swinWindowToScreen(Swin swin, SwinPoint point);
SwinPoint swinScreenToWindow(Swin swin, SwinPoint point);

/* ---------------------------------- :Time --------------------------------- */

f32 swinTime(Swin swin);
f32 swinDeltaTime(Swin swin);
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



/* ---------------------------------- :Impl --------------------------------- */

// TEMP
#define SWIN_IMPL
#ifdef SWIN_IMPL

// TODO: ifdef
#include <stdlib.h>
#define SWIN_MALLOC malloc

// TODO: ifdef
#include <assert.h>
#define SWIN_ASSERT assert

bool swinIsKeyDown(Swin swin, u32 key) {
    SwinInput input = swinInput(swin);
    return input.keys[key];
}

bool swinIsMouseDown(Swin swin, u8 button) {
    SwinInput input = swinInput(swin);
    return input.mouse[button];
}

#endif

/* --------------------------------- :Win32 --------------------------------- */

#ifdef SWIN_WIN32

typedef struct {
    // `HINSTANCE`
    void* instance;
    // `HWND`
    void* hwnd;
} SwinWin32State;

SwinWin32State swinGetWin32State(Swin swin);

/* ------------------------------- :Impl.Win32 ------------------------------ */

#ifdef SWIN_IMPL
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Swin_t {
    // Win32
    HINSTANCE instance;
    HWND hwnd;
    // Window
    u32 width;
    u32 height;
    bool resized;
    bool initialized;
    SwinWindowFlagBits windowFlags;
    // Input
    SwinInput input;
    SwinMouseFlagBits mouseFlags;
    SwinPoint lockedCursorPosition;
    // Time
    LARGE_INTEGER timeStart;
    LARGE_INTEGER timeCurrent;
    LARGE_INTEGER timeFrequency;
    f32 deltaTime;
    f32 totalTime;
};

SwinWin32State swinGetWin32State(Swin swin) {
    return (SwinWin32State) {
        .instance = swin->instance,
        .hwnd = swin->hwnd
    };
}

LRESULT CALLBACK swinWin32Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Swin swin = (Swin)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

    // Window closed
    if (msg == WM_DESTROY) PostQuitMessage(0);
    // Window resize
    else if (msg == WM_SIZE) {
        u32 w = LOWORD(lparam);
        u32 h = HIWORD(lparam);
        if ((w > 0 && h > 0) && (w != swin->width || h != swin->height)) {
            swin->width = w;
            swin->height = h;
            if (swin->initialized) swin->resized = true;
        }
    }
    // Input
    else if (msg == WM_KEYDOWN) swin->input.keys[wparam] = true;
    else if (msg == WM_KEYUP) swin->input.keys[wparam] = false;
    else if (msg == WM_LBUTTONDOWN) swin->input.mouse[0] = true;
    else if (msg == WM_LBUTTONUP) swin->input.mouse[0] = false;
    else if (msg == WM_RBUTTONDOWN) swin->input.mouse[1] = true;
    else if (msg == WM_RBUTTONUP) swin->input.mouse[1] = false;
    else if (msg == WM_MBUTTONDOWN) swin->input.mouse[2] = true;
    else if (msg == WM_MBUTTONUP) swin->input.mouse[2] = false;
    else if (msg == WM_MOUSEWHEEL) swin->input.scrollDelta += GET_WHEEL_DELTA_WPARAM(wparam);
    else if (msg == WM_MOUSEMOVE) {
        swin->input.mouseX = (int)(short)LOWORD(lparam);
        swin->input.mouseY = (int)(short)HIWORD(lparam);
    }
    // Raw input
    else if (msg == WM_INPUT) {
        UINT size = sizeof(RAWINPUT);
        RAWINPUT raw;

        UINT inputSize = GetRawInputData(
                (HRAWINPUT)lparam,
                RID_INPUT,
                &raw,
                &size,
                sizeof(RAWINPUTHEADER)
            );

        if (inputSize == size) {
            if (raw.header.dwType == RIM_TYPEMOUSE) {
                swin->input.mouseDeltaX += raw.data.mouse.lLastX;
                swin->input.mouseDeltaY += raw.data.mouse.lLastY;
            }
        }
    }
    // Unhandled
    else {
        return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

    return 0;
}

Swin swinInit(SwinInitInfo info) {
    Swin swin = (Swin)malloc(sizeof(struct Swin_t));
    SWIN_ASSERT(swin);
    *swin = (struct Swin_t) {};
    swin->width = info.width;
    swin->height = info.height;

    swin->instance = GetModuleHandle(NULL);

    // Window class
    WNDCLASSA wc = {
        .lpfnWndProc = swinWin32Proc,
        .hInstance = swin->instance,
        .lpszClassName = "Swin"
    };
    RegisterClassA(&wc);

    // Window
    swin->hwnd = CreateWindowA(
            "Swin", info.title,
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            info.width, info.height, 0, 0,
            swin->instance, 0
        );

    SetWindowLongPtrA(swin->hwnd, GWLP_USERDATA, (LONG_PTR)swin);

    // Raw mouse input
    RAWINPUTDEVICE rid = {
        .usUsagePage = 0x01, // Generic Desktop Controls
        .usUsage = 0x02,     // Mouse
        .dwFlags = 0,
        .hwndTarget = swin->hwnd
    };
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    // Set default flags
    swinSetWindowFlags(swin, info.windowFlags);
    swinSetMouseFlags(swin, info.mouseFlags);

    // Launch window
    ShowWindow(swin->hwnd, SW_SHOW);
    swin->initialized = true;

    // Time
    QueryPerformanceFrequency(&swin->timeFrequency);
    QueryPerformanceCounter(&swin->timeStart);

    return swin;
}

bool swinPoll(Swin swin) {
    // Update time
    LARGE_INTEGER timeCurrent;
    QueryPerformanceCounter(&timeCurrent);
    swin->totalTime = (f32)(timeCurrent.QuadPart - swin->timeStart.QuadPart)
        / (f32)swin->timeFrequency.QuadPart;
    swin->deltaTime = (f32)(timeCurrent.QuadPart - swin->timeCurrent.QuadPart)
        / (f32)swin->timeFrequency.QuadPart;
    swin->timeCurrent = timeCurrent;

    // Reset input deltas
    swin->input.mouseDeltaX = 0;
    swin->input.mouseDeltaY = 0;
    swin->input.scrollDelta = 0;

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        // TODO: free?

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return true;
}

typedef struct {
    BITMAPINFOHEADER header;
    DWORD masks[3];
} SwinWin32BitMapInfo;

void swinDraw(Swin swin, const u32* pixels) {
    SwinWin32BitMapInfo bmi = {
        .header = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = (LONG)swin->width,
            .biHeight = -(LONG)swin->height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_BITFIELDS,
        },
        .masks = {
            0x000000FF, // R
            0x0000FF00, // G
            0x00FF0000, // B
        }
    };

    HDC dc = GetDC(swin->hwnd);
    StretchDIBits(
        dc,
        0, 0, swin->width, swin->height,
        0, 0, swin->width, swin->height,
        pixels,
        (BITMAPINFO*)&bmi,
        DIB_RGB_COLORS,
        SRCCOPY
    );
    ReleaseDC(swin->hwnd, dc);
}

void swinQuit(Swin swin) {
    if (!swin) return;

    DestroyWindow(swin->hwnd);
    free(swin);
}

/* ---------------------------- :Impl.Win32.Input --------------------------- */

SwinInput swinInput(Swin swin) {
    return swin->input;
}

void swinWin32ClipCursor(Swin swin) {
    RECT rect;
    GetClientRect(swin->hwnd, &rect);
    MapWindowPoints(swin->hwnd, NULL, (POINT*)&rect, 2);
    ClipCursor(&rect);
}

void swinSetMouseFlags(Swin swin, SwinMouseFlagBits flags) {
    // Hidden
    bool wasHidden = (swin->mouseFlags & SwinMouse_Hidden) != 0;
    bool isHidden = (flags & SwinMouse_Hidden) != 0;
    if (wasHidden != isHidden) {
        ShowCursor(!isHidden);
    }

    // Capture
    bool wasCaptured = (swin->mouseFlags & SwinMouse_Capture) != 0;
    bool isCaptured = (flags & SwinMouse_Capture) != 0;
    if (wasCaptured != isCaptured) {
        if (isCaptured) swinWin32ClipCursor(swin);
        else ClipCursor(NULL);
    }

    // Locked
    if (flags & SwinMouse_Locked) {
        swinSetMousePosition(swin, swin->lockedCursorPosition);
    }
    else {
        swin->lockedCursorPosition = swinGetMousePosition(swin);
    }

    swin->mouseFlags = flags;
}

// TODO: not sync with input.mouseX, input.mouseY?
void swinSetMousePosition(Swin swin, SwinPoint position) {
    POINT p = { position.x, position.y };
    ClientToScreen(swin->hwnd, &p);
    SetCursorPos(p.x, p.y);
}

SwinPoint swinGetMousePosition(Swin swin) {
    return (SwinPoint) {
        swin->input.mouseX, swin->input.mouseY
    };
}

SwinPoint swinGetMouseDelta(Swin swin) {
    return (SwinPoint) {
        swin->input.mouseDeltaX, swin->input.mouseDeltaY
    };
}

/* --------------------------- :Impl.Win32.Window --------------------------- */

SwinSize swinGetWindowSize(Swin swin) {
    return (SwinSize) {
        swin->width, swin->height
    };
}

void swinSetWindowSize(Swin swin, SwinSize size) {
    RECT rect = { .right = (LONG)size.x, .bottom = (LONG)size.y, };

    // Resize
    DWORD style = (DWORD)GetWindowLongA(swin->hwnd, GWL_STYLE);
    AdjustWindowRect(&rect, style, false);

    // Adjust position
    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;
    SetWindowPos(swin->hwnd, NULL,
        0, 0, cx, cy,
        SWP_NOMOVE | SWP_NOZORDER);
}

bool swinIsWindowResized(Swin swin) {
    if (!swin->resized) return false;

    swin->resized = false;
    return true;
}

void swinSetWindowTitle(Swin swin, const char* title) {
    SetWindowTextA(swin->hwnd, title);
}

SwinPoint swinWindowToScreen(Swin swin, SwinPoint point) {
    POINT p = { point.x, point.y };
    ClientToScreen(swin->hwnd, &p);
    return (SwinPoint) {
        p.x, p.y
    };
}

SwinPoint swinScreenToWindow(Swin swin, SwinPoint point) {
    POINT p = { point.x, point.y };
    ScreenToClient(swin->hwnd, &p);
    return (SwinPoint) {
        p.x, p.y
    };
}

SwinPoint swinGetWindowPosition(Swin swin) {
    RECT rect;
    GetWindowRect(swin->hwnd, &rect);
    return (SwinPoint) {
        rect.left, rect.top
    };
}

void swinSetWindowPosition(Swin swin, SwinPoint position) {
    SetWindowPos(swin->hwnd, NULL,
        position.x, position.y, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER);
}

void swinSetWindowFlags(Swin swin, SwinWindowFlagBits flags) {
    bool fullscreen = flags & SwinWindow_Fullscreen;
    bool resizable = flags & SwinWindow_Resizable;
    bool topmost = flags & SwinWindow_Topmost;

    DWORD style = WS_OVERLAPPEDWINDOW;

    if (!resizable)
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    if (fullscreen)
        style = WS_POPUP;

    SetWindowLongA(swin->hwnd, GWL_STYLE, style);

    if (fullscreen) {
        HMONITOR monitor = MonitorFromWindow(
                swin->hwnd,
                MONITOR_DEFAULTTONEAREST
            );

        MONITORINFO info = {
            .cbSize = sizeof(info)
        };

        GetMonitorInfoA(monitor, &info);

        SetWindowPos(
            swin->hwnd,
            topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
            info.rcMonitor.left,
            info.rcMonitor.top,
            info.rcMonitor.right - info.rcMonitor.left,
            info.rcMonitor.bottom - info.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW
        );
    }
    else {
        RECT rect = {
            0, 0,
            (LONG)swin->width,
            (LONG)swin->height
        };

        AdjustWindowRect(
            &rect,
            style,
            FALSE
        );

        SetWindowPos(
            swin->hwnd,
            topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
            0, 0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_FRAMECHANGED |
            SWP_NOMOVE |
            SWP_SHOWWINDOW
        );
    }

    swin->initialized = true;
}

/* ---------------------------- :Impl.Win32.Time ---------------------------- */

f32 swinTime(Swin swin) {
    return swin->totalTime;
}

f32 swinDeltaTime(Swin swin) {
    return swin->deltaTime;
}

#endif
#endif