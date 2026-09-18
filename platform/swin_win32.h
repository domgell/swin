#pragma once

typedef struct {
    void* instance; // HINSTANCE
    void* hwd; // HWND
} SwinWin32State;

SWIN_FN SwinWin32State SwinGetWin32State(Swin swin);

// TODO: allow override winproc
// swinSetWin32Proc(Swin swin, SwinWin32Proc proc);

#ifdef SWIN_IMPL

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Swin_t {
    // Win32
    HINSTANCE instance;
    HWND wnd;
    HDC dc;
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

#ifndef SWIN_WIN32_PROC
#define SWIN_WIN32_PROC(hwnd, msg, wparam, lparam) false
#endif

LRESULT CALLBACK swinWin32Proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (SWIN_WIN32_PROC(hwnd, msg, wparam, lparam)) return true;

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
    memset(swin, 0, sizeof(struct Swin_t));

    SWIN_ASSERT(info.width > 0 && info.height > 0 && "Must provide info.width and info.height");
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
    swin->wnd = CreateWindowA(
        "Swin", info.title,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        info.width, info.height, 0, 0,
        swin->instance, 0
    );

    SetWindowLongPtrA(swin->wnd, GWLP_USERDATA, (LONG_PTR)swin);

    // Raw mouse input
    RAWINPUTDEVICE rid = {
        .usUsagePage = 0x01, // Generic Desktop Controls
        .usUsage = 0x02, // Mouse
        .dwFlags = 0,
        .hwndTarget = swin->wnd
    };
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    // Set default flags
    swinSetWindowFlags(swin, info.windowFlags);
    swinSetMouseFlags(swin, info.mouseFlags);

    // Launch window
    ShowWindow(swin->wnd, SW_SHOW);
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

    if (!swin->dc) swin->dc = GetDC(swin->wnd);

    SetDIBitsToDevice(
        swin->dc,
        0, 0,
        swin->width, swin->height,
        0, 0,
        0, swin->height,
        pixels,
        (BITMAPINFO*)&bmi,
        DIB_RGB_COLORS
    );
}

void swinQuit(Swin swin) {
    if (!swin) return;

    if (swin->dc) ReleaseDC(swin->wnd, swin->dc);
    DestroyWindow(swin->wnd);
    SWIN_FREE(swin);
}

/* ---------------------------- :Impl.Win32.Input --------------------------- */

SwinInput swinInput(Swin swin) {
    return swin->input;
}

void swinWin32ClipCursor(Swin swin) {
    RECT rect;
    GetClientRect(swin->wnd, &rect);
    MapWindowPoints(swin->wnd, NULL, (POINT*)&rect, 2);
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
    POINT p = {position.x, position.y};
    ClientToScreen(swin->wnd, &p);
    SetCursorPos(p.x, p.y);
}

SwinPoint swinGetMousePosition(Swin swin) {
    return (SwinPoint){
        swin->input.mouseX, swin->input.mouseY
    };
}

SwinPoint swinGetMouseDelta(Swin swin) {
    return (SwinPoint){
        swin->input.mouseDeltaX, swin->input.mouseDeltaY
    };
}

/* --------------------------- :Impl.Win32.Window --------------------------- */

SwinSize swinGetWindowSize(Swin swin) {
    return (SwinSize){
        swin->width, swin->height
    };
}

void swinSetWindowSize(Swin swin, SwinSize size) {
    RECT rect = {.right = (LONG)size.x, .bottom = (LONG)size.y,};

    // Resize
    DWORD style = (DWORD)GetWindowLongA(swin->wnd, GWL_STYLE);
    AdjustWindowRect(&rect, style, false);

    // Adjust position
    int cx = rect.right - rect.left;
    int cy = rect.bottom - rect.top;
    SetWindowPos(swin->wnd, NULL,
        0, 0, cx, cy,
        SWP_NOMOVE | SWP_NOZORDER);
}

bool swinIsWindowResized(Swin swin) {
    if (!swin->resized) return false;

    swin->resized = false;
    return true;
}

void swinSetWindowTitle(Swin swin, const char* title) {
    SetWindowTextA(swin->wnd, title);
}

SwinPoint swinWindowToScreen(Swin swin, SwinPoint point) {
    POINT p = {point.x, point.y};
    ClientToScreen(swin->wnd, &p);
    return (SwinPoint){
        p.x, p.y
    };
}

SwinPoint swinScreenToWindow(Swin swin, SwinPoint point) {
    POINT p = {point.x, point.y};
    ScreenToClient(swin->wnd, &p);
    return (SwinPoint){
        p.x, p.y
    };
}

SwinPoint swinGetWindowPosition(Swin swin) {
    RECT rect;
    GetWindowRect(swin->wnd, &rect);
    return (SwinPoint){
        rect.left, rect.top
    };
}

void swinSetWindowPosition(Swin swin, SwinPoint position) {
    SetWindowPos(swin->wnd, NULL,
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

    SetWindowLongA(swin->wnd, GWL_STYLE, style);

    if (fullscreen) {
        HMONITOR monitor = MonitorFromWindow(
            swin->wnd,
            MONITOR_DEFAULTTONEAREST
        );

        MONITORINFO info = {
            .cbSize = sizeof(info)
        };

        GetMonitorInfoA(monitor, &info);

        SetWindowPos(
            swin->wnd,
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
            swin->wnd,
            topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
            0, 0,
            rect.right - rect.left,
            rect.bottom - rect.top,
            SWP_FRAMECHANGED |
            SWP_NOMOVE |
            SWP_SHOWWINDOW
        );
    }

    swin->windowFlags = flags;
    swin->initialized = true;
}

SwinWindowFlagBits swinGetWindowFlags(Swin swin) {
    return swin->windowFlags;
}

/* ---------------------------- :Impl.Win32.Time ---------------------------- */

f32 swinTime(Swin swin) {
    return swin->totalTime;
}

f32 swinDeltaTime(Swin swin) {
    return swin->deltaTime;
}

#endif