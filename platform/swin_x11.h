#pragma once

typedef struct {
    void* display; // Display*
    void* window; // Window
} SwinX11State;

SWIN_FN SwinX11State swinGetX11State(Swin swin);

#ifdef SWIN_IMPL

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

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

char swinX11ConvertMouseButton(u32 button) {
    switch (button) {
    case Button1: return Swin_MouseLeft;
    case Button2: return Swin_MouseMiddle;
    case Button3: return Swin_MouseRight;
    default: return 0;
    }
}

void swinX11Resize(Swin swin, u32 width, u32 height) {
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
    i32 screen = DefaultScreen(swin->display);
    swin->window = XCreateSimpleWindow(
        swin->display,
        RootWindow(swin->display, screen),
        0, 0,
        info.width,
        info.height,
        1,
        BlackPixel(swin->display, screen),
        WhitePixel(swin->display, screen)
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

    return swin;
}

bool swinPoll(Swin swin) {
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
            char btn = swinX11ConvertMouseButton(event.xbutton.button);
            if (btn) swin->input.mouse[btn] = true;
        }
        else if (event.type == ButtonRelease) {
            char btn = swinX11ConvertMouseButton(event.xbutton.button);
            if (btn) swin->input.mouse[btn] = false;
        }
    }

    return true;
}

void swinDraw(Swin swin, const u32* pixels) {
    memcpy(swin->image->data, pixels, swin->width * swin->height * sizeof(u32));
    XPutImage(swin->display, swin->window, swin->gc, swin->image, 0, 0, 0, 0, swin->width, swin->height);
    XFlush(swin->display);
}

SwinInput swinInput(Swin swin) {
    return swin->input;
}

// -------------------------------------- :Window --------------------------------------

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
    XSendEvent(
        swin->display, DefaultRootWindow(swin->display), false,
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
}

#endif
