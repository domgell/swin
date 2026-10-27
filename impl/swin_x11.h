#pragma once

#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <string.h>

struct Swin_t {
    // X11
    Display* display;
    Window window;
    GC gc;
    XImage* gcImage;
    i32 screen;
    // Window
    u32 width;
    u32 height;
    bool resized;
    SwinWindowFlagBits windowFlags;
    // XInput2
    i32 xiOpcode;
    // Input
    SwinInput input;
    SwinInput prevInput;
    SwinMouseFlagBits mouseFlags;
    SwinPoint lockedCursorPosition;
    // Time
    f64 timeCurrent;
    f64 timeStart;
    f32 deltaTime;
    f32 totalTime;
};

SwinPlatformState swinGetPlatformState(Swin swin) {
    SwinPlatformState state = {};
    state.platform = SwinPlatform_X11;
    state.x11.display = swin->display;
    state.x11.window = swin->window;
    return state;
}

// *DEPRECATED*
SwinX11State swinGetX11State(Swin swin) {
    SwinPlatformState state = swinGetPlatformState(swin);
    return state.x11;
}

void swinX11InitImage(Swin swin) {
    if (swin->gcImage)
        XDestroyImage(swin->gcImage);

    Visual* visual = DefaultVisual(swin->display, swin->screen);
    i32 depth = DefaultDepth(swin->display, swin->screen);

    swin->gcImage = XCreateImage(
        swin->display, visual, depth, ZPixmap, 0, NULL,
        swin->width, swin->height,
        32, 0
    );
    SWIN_ASSERT(swin->gcImage && "Failed to create XImage");

    size_t byteSize = swin->gcImage->bytes_per_line * swin->gcImage->height;
    swin->gcImage->data = (char*)SWIN_MALLOC(byteSize);
    SWIN_ASSERT(swin->gcImage->data && "Failed to allocate XImage data");
    memset(swin->gcImage->data, 0, byteSize);
}

u8 swinX11ConvertKey(KeySym key) {
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

    case XK_space: return Swin_Space;

    default: break;
    }

    if (key >= XK_0 && key <= XK_9) return (char)key;
    if (key >= XK_a && key <= XK_z) return (char)key;
    if (key >= XK_A && key <= XK_Z) return (char)(key + ('a' - 'A')); // TODO: NOT WORKING

    return 0;
}

void swinX11SetMouseButton(Swin swin, u32 btn, bool value) {
    if (btn == Button1) swin->input.mouse[SwinMouseButton_Left] = value;
    else if (btn == Button2) swin->input.mouse[SwinMouseButton_Middle] = value;
    else if (btn == Button3) swin->input.mouse[SwinMouseButton_Right] = value;
    else if (btn == Button4) swin->input.mouse[SwinMouseButton_Side1] = value;
    else if (btn == Button5) swin->input.mouse[SwinMouseButton_Side2] = value;
}

void swinX11Resize(Swin swin, u32 width, u32 height) {
    if (swin->width == width && swin->height == height) return;

    swin->width = width;
    swin->height = height;
    swin->resized = true;

    if (swin->gc) swinX11InitImage(swin);
}

void swinInitXInput2(Swin swin) {
    i32 event, error, major = 2, minor = 0;

    SWIN_ASSERT(XQueryExtension(swin->display, "XInputExtension",
        &swin->xiOpcode, &event, &error) && "XInput extension not available");
    SWIN_ASSERT(XIQueryVersion(swin->display, &major, &minor) == Success && "XInput2 not available");

    u8 mask[XIMaskLen(XI_RawMotion)] = {};
    XISetMask(mask, XI_RawMotion);

    XIEventMask evmask = {};
    evmask.deviceid = XIAllMasterDevices;
    evmask.mask_len = sizeof(mask);
    evmask.mask = mask;

    XISelectEvents(swin->display, DefaultRootWindow(swin->display), &evmask, 1);
}

// --------------------------------------- :Init ---------------------------------------


Swin swinInit(SwinInitInfo info) {
    Swin swin = (Swin)SWIN_MALLOC(sizeof(struct Swin_t));
    memset(swin, 0, sizeof(struct Swin_t));

    if (info.width == 0) info.width = 400;
    if (info.height == 0) info.height = 300;
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

    swinInitXInput2(swin);

    swinSetWindowFlags(swin, info.windowFlags);
    swinSetMouseFlags(swin, info.mouseFlags);

    return swin;
}

bool swinPoll(Swin swin) {
    // Update time
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    f64 currentTime = (f64)ts.tv_sec + (f64)ts.tv_nsec / 1000000000.0;
    swin->deltaTime = (f32)(currentTime - swin->timeCurrent);
    swin->totalTime = (f32)(currentTime - swin->timeStart);
    swin->timeCurrent = currentTime;

    // Reset input deltas
    swin->input.mouseDeltaX = 0;
    swin->input.mouseDeltaY = 0;
    swin->input.scrollDelta = 0;

    // Set prev input
    memcpy(&swin->prevInput, &swin->input, sizeof(SwinInput));

    while (XPending(swin->display)) {
        XEvent event;
        XNextEvent(swin->display, &event);

        if (event.type == ConfigureNotify) {
            swinX11Resize(swin, event.xconfigure.width, event.xconfigure.height);
        }
        else if (event.type == KeyPress) {
            u8 key = swinX11ConvertKey(XLookupKeysym(&event.xkey, 0));
            if (key) swin->input.keys[key] = true;
        }
        else if (event.type == KeyRelease) {
            u8 key = swinX11ConvertKey(XLookupKeysym(&event.xkey, 0));
            if (key) swin->input.keys[key] = false;
        }
        else if (event.type == ButtonPress) {
            swinX11SetMouseButton(swin, event.xbutton.button, true);
        }
        else if (event.type == ButtonRelease) {
            swinX11SetMouseButton(swin, event.xbutton.button, false);
        }
        else if (event.type == MotionNotify) {
            swin->input.mouseX = event.xmotion.x;
            swin->input.mouseY = event.xmotion.y;

            // Only update cursor lock when mouse moved to avoid infinite move events
            if (swin->mouseFlags & SwinMouse_Locked &&
                (swin->input.mouseDeltaX != 0 || swin->input.mouseDeltaY != 0)) {
                swinSetMousePosition(swin, swin->lockedCursorPosition);
            }
        }
        // Raw input
        else if (event.xcookie.extension == swin->xiOpcode) {
            if (!XGetEventData(swin->display, &event.xcookie)) continue;

            XIRawEvent* data = (XIRawEvent*)event.xcookie.data;
            i32 value_index = 0;

            for (u32 i = 0; i < data->valuators.mask_len * 8; i++) {
                if (!XIMaskIsSet(data->valuators.mask, i)) continue;

                // Mouse delta
                f64 value = data->raw_values[value_index++];
                if (i == 0) swin->input.mouseDeltaX += value;
                else if (i == 1) swin->input.mouseDeltaY += value;
            }

            XFreeEventData(swin->display, &event.xcookie);
        }
    }

    return true;
}

void swinDraw(Swin swin, const u32* pixels) {
    // Initialize graphics context on first call
    if (!swin->gc) {
        swin->gc = XCreateGC(swin->display, swin->window, 0, NULL);
        SWIN_ASSERT(swin->gc && "Failed to create X11 graphics context");
        swinX11InitImage(swin);
    }

    memcpy(swin->gcImage->data, pixels, swin->width * swin->height * sizeof(u32));
    XPutImage(swin->display, swin->window, swin->gc, swin->gcImage, 0, 0, 0, 0, swin->width, swin->height);
    XFlush(swin->display);
}

// -------------------------------------- :Input ---------------------------------------

SwinInput swinInput(Swin swin) {
    return swin->input;
}

SwinInput swinPrevInput(Swin swin) {
    return swin->prevInput;
}

void swinSetMouseFlags(Swin swin, SwinMouseFlagBits flags) {
    if (swin->mouseFlags == flags) return;

    // Hidden
    if ((swin->mouseFlags ^ flags) & SwinMouse_Hidden) {
        if (flags & SwinMouse_Hidden) {
            Pixmap cursorBitmap = XCreateBitmapFromData(swin->display, swin->window, "\0", 1, 1);
            XColor color = {};
            Cursor invisibleCursor = XCreatePixmapCursor(swin->display,
                cursorBitmap, cursorBitmap, &color, &color, 0, 0);
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
        if (flags & SwinMouse_Locked) {
            swin->lockedCursorPosition = swinGetMousePosition(swin);
            swinSetMousePosition(swin, swin->lockedCursorPosition);
        }
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
    // XFlush(swin->display);
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
    // XFlush(swin->display);
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
    XFlush(swin->display);
}