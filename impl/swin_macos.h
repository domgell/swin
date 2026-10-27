#pragma once

// Based on: https://github.com/ColleagueRiley/Cocoa-in-Pure-C/blob/main/example.c

#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreVideo/CVDisplayLink.h>
#include <ApplicationServices/ApplicationServices.h>

#include "../swin.h"

#ifdef __arm64__
/* ARM just uses objc_msgSend */
#define abi_objc_msgSend_stret objc_msgSend
#define abi_objc_msgSend_fpret objc_msgSend
#else /* __i386__ */
/* x86 just uses abi_objc_msgSend_fpret and (NSColor *)objc_msgSend_id respectively */
#define abi_objc_msgSend_stret objc_msgSend_stret
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#endif

typedef CGRect NSRect;
typedef CGPoint NSPoint;
typedef CGSize NSSize;

typedef void NSEvent;
typedef void NSString;
typedef void NSWindow;
typedef void NSApplication;

typedef unsigned long NSUInteger;
typedef long NSInteger;

#define NS_ENUM(type, name) type name; enum

// TODO: rename
enum {
    SWIN_STYLE_TITLED = 1 << 0,
    SWIN_STYLE_CLOSABLE = 1 << 1,
    SWIN_STYLE_MINIMIZABLE = 1 << 2,
    SWIN_STYLE_RESIZABLE = 1 << 3,
    SWIN_BACKING_BUFFERED = 2
};

// TODO: not needed
#define objc_msgSend_id				((id (*)(id, SEL))objc_msgSend)
#define objc_msgSend_id_id			((id (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_id_rect		((id (*)(id, SEL, NSRect))objc_msgSend)
#define objc_msgSend_uint			((NSUInteger (*)(id, SEL))objc_msgSend)
#define objc_msgSend_int			((NSInteger (*)(id, SEL))objc_msgSend)
#define objc_msgSend_SEL			((SEL (*)(id, SEL))objc_msgSend)
#define objc_msgSend_float			((CGFloat (*)(id, SEL))abi_objc_msgSend_fpret)
#define objc_msgSend_bool			((BOOL (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void			((void (*)(id, SEL))objc_msgSend)
#define objc_msgSend_double			((double (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id		((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_void_uint		((void (*)(id, SEL, NSUInteger))objc_msgSend)
#define objc_msgSend_void_int		((void (*)(id, SEL, NSInteger))objc_msgSend)
#define objc_msgSend_void_bool		((void (*)(id, SEL, BOOL))objc_msgSend)
#define objc_msgSend_void_float		((void (*)(id, SEL, CGFloat))objc_msgSend)
#define objc_msgSend_void_double	((void (*)(id, SEL, double))objc_msgSend)
#define objc_msgSend_void_SEL		((void (*)(id, SEL, SEL))objc_msgSend)
#define objc_msgSend_id_char_const	((id (*)(id, SEL, char const *))objc_msgSend)

#define SWIN_OBJC_MSG(ret, name, ...) \
    ((ret (*)(id, SEL, ##__VA_ARGS__))objc_msgSend)

#define SWIN_OBJC_SEND(obj, selector, ret, ...) \
    SWIN_OBJC_MSG(ret, selector, ##__VA_ARGS__)( \
        (id)(obj), sel_registerName(#selector) \
    )

#define SWIN_OBJC_ALLOC(cls) \
    SWIN_OBJC_SEND(objc_getClass(cls), alloc, id)

#define SWIN_OBJC_INIT(obj) \
    SWIN_OBJC_SEND(obj, init, id)

#define SWIN_OBJC_RELEASE(obj) \
    SWIN_OBJC_SEND(obj, release, void)

struct Swin_t {
    // Cocoa
    NSWindow* window;
    NSApplication* app;
    bool running;
    //
    bool resized;
};

void swinMacOSOnClose(void* self) {
    Swin swin = NULL;
    object_getInstanceVariable(self, "swin", (void**)&swin);

    if (swin != NULL) swin->running = false;
    return true;
}

NSSize swinMacOSOnResize(void* self, SEL sel, NSSize frameSize) {
    Swin swin = NULL;
    object_getInstanceVariable(self, "swinPtr", (void**)&swin);

    if (swin != NULL) {
        swin->width = (int)frameSize.width;
        swin->height = (int)frameSize.height;
        swin->resized = true;
    }

    return frameSize;
}

Swin swinInit(SwinInitInfo info) {
    Swin swin = (Swin)SWIN_MALLOC(sizeof(struct Swin_t));
    memset(swin, 0, sizeof(struct Swin_t));

    // Default window size
    if (info.width == 0) info.width = 400;
    if (info.height == 0) info.height = 300;
    swin->width = info.width;
    swin->height = info.height;

    // Default title
    if (!info.title) info.title = "swin";

    // App
    swin->app = SWIN_OBJC_SEND(
        objc_getClass("NSApplication"),
        sharedApplication,
        id
    );
    SWIN_ASSERT(swin->app && "Error creating NSApplication");

    SWIN_OBJC_MSG(void, setActivationPolicy:, NSInteger)(
        (id)swin->app,
        sel_registerName("setActivationPolicy:"),
        0 /* NSApplicationActivationPolicyRegular */
    );

    // Window
    NSRect rect = {};
    rect.origin.x = 100;
    rect.origin.y = 100;
    rect.size.width = info.width;
    rect.size.height = info.height;
    NSUInteger style = SWIN_STYLE_TITLED | SWIN_STYLE_CLOSABLE | SWIN_STYLE_MINIMIZABLE | SWIN_STYLE_RESIZABLE;
    id windowAlloc = SWIN_OBJC_ALLOC("NSWindow");
    SWIN_ASSERT(windowAlloc && "Error allocating NSWindow");
    swin->window = SWIN_OBJC_MSG(id, initWithContentRect:styleMask:backing:defer:, NSRect, NSUInteger, NSUInteger, bool)(
        windowAlloc,
        sel_registerName("initWithContentRect:styleMask:backing:defer:"),
        rect, style, SWIN_BACKING_BUFFERED, false
    );
}

bool swinPoll(Swin swin) {
    if (!swin->running) return false;

    id pool = SWIN_OBJC_INIT(SWIN_OBJC_ALLOC("NSAutoreleasePool"));

    id distantPast = SWIN_OBJC_SEND(
        objc_getClass("NSDate"),
        distantPast,
        id
    );

    id mode = SWIN_OBJC_MSG(id, stringWithUTF8String:, const char*)(
        objc_getClass("NSString"),
        sel_registerName("stringWithUTF8String:"),
        "kCFRunLoopDefaultMode"
    );

    id event;

    while (true) {
        event = SWIN_OBJC_MSG(id,nextEventMatchingMask:untilDate:inMode:dequeue:,NSUInteger, id, id, bool)(
            (id)swin->app,
            sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),
            (NSUInteger)-1,
            distantPast,
            mode,
            true
        );
        // No more events
        if (!event) break;

        SWIN_OBJC_MSG(void, sendEvent:, id)(
            (id)swin->app,
            sel_registerName("sendEvent:"),
            event
        );

        if (!swin->running) break;
    }

    SWIN_OBJC_MSG(void, updateWindows)(
        (id)swin->app,
        sel_registerName("updateWindows")
    );

    SWIN_OBJC_RELEASE(pool);

    return swin->running;
}