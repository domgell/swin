#pragma once

typedef struct {
    void* display; // Display*
    void* window; // Window
} SwinX11State;

SWIN_FN SwinX11State swinGetX11State(Swin swin);

#ifdef SWIN_IMPL

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <string.h>
#include <time.h>

struct Swin_t {
    // X11
    Display* display;
    Window window;
    GC gc;
    XImage* image;
    i32 screen;
    // Window
    u32 width;
    u32 height;
    bool resized;
    SwinWindowFlagBits windowFlags;
    // Input
    SwinInput input;
    SwinMouseFlagBits mouseFlags;
    SwinPoint lockedCursorPosition;
    // Time
    f64 timeCurrent;
    f64 timeStart;
    f32 deltaTime;
    f32 totalTime;
};

void swinX11InitImage(Swin swin) {
    if (swin->image)
        XDestroyImage(swin->image);

    Visual* visual = DefaultVisual(swin->display, swin->screen);
    i32 depth = DefaultDepth(swin->display, swin->screen);

    swin->image = XCreateImage(
        swin->display, visual, depth, ZPixmap, 0, NULL,
        swin->width, swin->height,
        32, 0
    );
    SWIN_ASSERT(swin->image && "Failed to create XImage");

    size_t byteSize = swin->image->bytes_per_line * swin->image->height;
    swin->image->data = (char*)SWIN_MALLOC(byteSize);
    SWIN_ASSERT(swin->image->data && "Failed to allocate XImage data");
    memset(swin->image->data, 0, byteSize);
}

char swinX11ConvertKey(KeySym key) {
    switch (key) {
    case XK_Escape: return Swin_Esc;
    case XK_Control_L: return Swin_CtrlLeft;
    case XK_Control_R: return Swin_CtrlRight;

    case XK_Shift_L: return Swin_ShiftLeft;
    case XK_Shift_R: return Swin_ShiftRight;

    case XK_Left: return Swin_ArrowLeft;
    case XK_Right: return Swin_ArrowRight;
    case XK_Up: return Swin_ArrowUp;
    case XK_Down: return Swin_ArrowDown;

    default: break;
    }

    if (key >= XK_0 && key <= XK_9) return (char)key;
    if (key >= XK_a && key <= XK_z) return (char)key;
    if (key >= XK_A && key <= XK_Z) return (char)(key + ('a' - 'A'));

    return 0;
}

void swinX11SetMouseButton(Swin swin, u32 btn, bool value) {
    // TODO: other buttons
    if (btn == Button1 || btn == Button2 || btn == Button3) {
        u8 swinBtn = btn - 1;
        swin->input.mouse[swinBtn] = value;
    }
}

void swinX11Resize(Swin swin, u32 width, u32 height) {
    if (swin->width == width && swin->height == height) return;

    swin->width = width;
    swin->height = height;
    swin->resized = true;
    swinX11InitImage(swin);
}

// --------------------------------------- :Init ---------------------------------------


Swin swinInit(SwinInitInfo info) {
    Swin swin = (Swin)SWIN_MALLOC(sizeof(struct Swin_t));
    memset(swin, 0, sizeof(struct Swin_t));

    swin->width = info.width;
    swin->height = info.height;

    // Display
    swin->display = XOpenDisplay(NULL);
    SWIN_ASSERT(swin->display && "Failed to open X11 display");

    // Window
    swin->screen = DefaultScreen(swin->display); // TODO: screen
    swin->window = XCreateSimpleWindow(
        swin->display,
        RootWindow(swin->display, swin->screen),
        0, 0,
        info.width,
        info.height,
        1,
        BlackPixel(swin->display, swin->screen),
        WhitePixel(swin->display, swin->screen)
    );

    // Title
    if (!info.title) info.title = "swin";
    XStoreName(swin->display, swin->window, info.title);

    swin->gc = XCreateGC(swin->display, swin->window, 0,NULL);
    SWIN_ASSERT(swin->gc && "Failed to create X11 GC");
    swinX11InitImage(swin);

    XSelectInput(swin->display, swin->window,
        ExposureMask |
        KeyPressMask |
        KeyReleaseMask |
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask
    );
    XMapWindow(swin->display, swin->window);
    XFlush(swin->display);

    swinSetWindowFlags(swin, info.windowFlags);

    return swin;
}

bool swinPoll(Swin swin) {
    // Update time
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    f64 currentTime = (f64)time.tv_sec + (f64)time.tv_nsec / 1000000000.0;
    swin->deltaTime = (f32)(currentTime - swin->timeCurrent);
    swin->totalTime = (f32)(currentTime - swin->timeStart);
    swin->timeCurrent = currentTime;

    // Reset input deltas
    swin->input.mouseDeltaX = 0;
    swin->input.mouseDeltaY = 0;
    swin->input.scrollDelta = 0;

    while (XPending(swin->display)) {
        XEvent event;
        XNextEvent(swin->display, &event);

        if (event.type == ConfigureNotify) {
            swinX11Resize(swin, event.xconfigure.width, event.xconfigure.height);
        }
        else if (event.type == KeyPress) {
            char key = swinX11ConvertKey(XLookupKeysym(&event.xkey, 0));
            if (key) swin->input.keys[key] = true;
        }
        else if (event.type == KeyRelease) {
            char key = swinX11ConvertKey(XLookupKeysym(&event.xkey, 0));
            if (key) swin->input.keys[key] = false;
        }
        else if (event.type == ButtonPress) {
            swinX11SetMouseButton(swin, event.xbutton.button, true);
        }
        else if (event.type == ButtonRelease) {
            swinX11SetMouseButton(swin, event.xbutton.button, false);
        }
        else if (event.type == MotionNotify) {
            swin->input.mouseDeltaX = event.xmotion.x - swin->input.mouseX;
            swin->input.mouseDeltaY = event.xmotion.y - swin->input.mouseY;
            swin->input.mouseX = event.xmotion.x;
            swin->input.mouseY = event.xmotion.y;

            // Only update cursor lock when mouse moved to avoid infinite move events
            if (swin->mouseFlags & SwinMouse_Locked &&
                (swin->input.mouseDeltaX > 0 || swin->input.mouseDeltaY > 0)) {
                swinSetMousePosition(swin, swin->lockedCursorPosition);
            }
        }
    }

    return true;
}

void swinDraw(Swin swin, const u32* pixels) {
    memcpy(swin->image->data, pixels, swin->width * swin->height * sizeof(u32));
    XPutImage(swin->display, swin->window, swin->gc, swin->image, 0, 0, 0, 0, swin->width, swin->height);
    XFlush(swin->display);
}

// -------------------------------------- :Input ---------------------------------------

SwinInput swinInput(Swin swin) {
    return swin->input;
}

void swinSetMouseFlags(Swin swin, SwinMouseFlagBits flags) {
    if (swin->mouseFlags == flags) return;

    // Hidden
    if ((swin->mouseFlags ^ flags) & SwinMouse_Hidden) {
        if (flags & SwinMouse_Hidden) {
            Pixmap cursorBitmap = XCreateBitmapFromData(swin->display, swin->window, "\0", 1, 1);
            XColor color = {};
            Cursor invisibleCursor = XCreatePixmapCursor(swin->display, cursorBitmap, cursorBitmap, &color, &color, 0,
                0);
            XDefineCursor(swin->display, swin->window, invisibleCursor);

            XFreeCursor(swin->display, invisibleCursor);
            XFreePixmap(swin->display, cursorBitmap);
        }
        else {
            XUndefineCursor(swin->display, swin->window);
        }
    }

    // Capture
    if ((swin->mouseFlags ^ flags) & SwinMouse_Capture) {
        if (flags & SwinMouse_Capture) {
            XGrabPointer(swin->display, swin->window, True, ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync, GrabModeAsync, swin->window, None, CurrentTime);
        }
        else {
            XUngrabPointer(swin->display, CurrentTime);
        }
    }

    // Locked
    if ((swin->mouseFlags ^ flags) & SwinMouse_Locked) {
        if (flags & SwinMouse_Locked) swinSetMousePosition(swin, swin->lockedCursorPosition);
        else swin->lockedCursorPosition = swinGetMousePosition(swin);
    }

    swin->mouseFlags = flags;
    XFlush(swin->display);
}

SwinMouseFlagBits swinGetMouseFlags(Swin swin) {
    return swin->mouseFlags;
}

void swinSetMousePosition(Swin swin, SwinPoint position) {
    // TODO: does not work for XWayland
    swin->input.mouseX = position.x;
    swin->input.mouseY = position.y;
    XWarpPointer(swin->display, None, swin->window, 0, 0, 0, 0, position.x, position.y);
    XFlush(swin->display);
}

// --------------------------------------- :Time ---------------------------------------

f32 swinTime(Swin swin) {
    return swin->totalTime;
}

f32 swinDeltaTime(Swin swin) {
    return swin->deltaTime;
}

// -------------------------------------- :Window --------------------------------------

void swinSetWindowTitle(Swin swin, const char* title) {
    XStoreName(swin->display, swin->window, title);
    XFlush(swin->display);
}

bool swinIsWindowResized(Swin swin) {
    if (!swin->resized) return false;

    swin->resized = false;
    return true;
}

SwinSize swinGetWindowSize(Swin swin) {
    SwinSize size = {};
    size.x = swin->width;
    size.y = swin->height;
    return size;
}

void swinSetWindowFlags(Swin swin, SwinWindowFlagBits flags) {
    // TODO: only change if flags are different

    Atom wmState = XInternAtom(swin->display, "_NET_WM_STATE", false);

    // Fullscreen
    Atom fullscreenAtom = XInternAtom(swin->display, "_NET_WM_STATE_FULLSCREEN", false);
    XEvent fsEv = {};
    fsEv.type = ClientMessage;
    fsEv.xclient.window = swin->window;
    fsEv.xclient.message_type = wmState;
    fsEv.xclient.format = 32;
    fsEv.xclient.data.l[0] = flags & SwinWindow_Fullscreen;
    fsEv.xclient.data.l[1] = fullscreenAtom;
    XSendEvent(swin->display, DefaultRootWindow(swin->display), false,
        SubstructureRedirectMask | SubstructureNotifyMask, &fsEv);

    // Resizable
    XSizeHints sizeHints = {};
    if (flags & SwinWindow_Resizable) {
        sizeHints.flags = PMinSize;
    }
    else {
        sizeHints.min_width = swin->width;
        sizeHints.min_height = swin->height;
        sizeHints.max_width = swin->width;
        sizeHints.max_height = swin->height;
        sizeHints.flags = PMinSize | PMaxSize;
    }
    XSetWMNormalHints(swin->display, swin->window, &sizeHints);

    // Topmost
    Atom topmostAtom = XInternAtom(swin->display, "_NET_WM_STATE_ABOVE", false);
    XEvent topEv = {};
    topEv.type = ClientMessage;
    topEv.xclient.window = swin->window;
    topEv.xclient.message_type = wmState;
    topEv.xclient.format = 32;
    topEv.xclient.data.l[0] = (flags & SwinWindow_Topmost) ? 1 : 0;
    topEv.xclient.data.l[1] = (long)topmostAtom;
    topEv.xclient.data.l[2] = 0;
    topEv.xclient.data.l[3] = 1;
    XSendEvent(swin->display, DefaultRootWindow(swin->display), false,
        SubstructureNotifyMask | SubstructureRedirectMask, &topEv);

    // NoTopbar
    Atom property = XInternAtom(swin->display, "_MOTIF_WM_HINTS", false);
    struct {
        u64 flags;
        u64 functions;
        u64 decorations;
        i64 input_mode;
        u64 status;
    } hints = {};
    hints.flags = (1L << 1);
    hints.decorations = (flags & SwinWindow_NoTopbar) ? 0 : 1;
    XChangeProperty(swin->display, swin->window, property, property, 32, PropModeReplace, (u8*)&hints, 5);

    swin->windowFlags = flags;
}

void swinSetWindowSize(Swin swin, SwinSize size) {
    XResizeWindow(swin->display, swin->window, size.x, size.y);
}

void swinSetWindowPosition(Swin swin, SwinPoint position) {
    XMoveWindow(swin->display, swin->window, position.x, position.y);
}

#endif
