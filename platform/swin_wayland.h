#pragma once

#ifdef SWIN_IMPL

#include <wayland-client.h>

#include "../external/wayland/xdg-shell-client-protocol.h"
#include "../external/wayland/relative-pointer-unstable-v1-client-protocol.h"
#include "../external/wayland/pointer-constraints-unstable-v1-client-protocol.h"
#include "../external/wayland/xdg-decoration-unstable-v1.h"

#include "../external/wayland/xdg-shell-protocol.c"
#include "../external/wayland/relative-pointer-unstable-v1-protocol.c"
#include "../external/wayland/pointer-constraints-unstable-v1-protocol.c"
#include "../external/wayland/xdg-decoration-unstable-v1-protocol.c"

#include <linux/input-event-codes.h>

#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

struct Swin_t {
    // Wayland
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor* compositor;
    struct wl_surface* surface;
    struct xdg_wm_base* wmBase;
    struct xdg_surface* xdgSurface;
    struct xdg_toplevel* toplevel;
    struct wl_registry_listener registryListener;
    struct xdg_wm_base_listener wmListener;
    struct xdg_surface_listener surfaceListener;
    struct xdg_toplevel_listener toplevelListener;
    // Buffer
    struct wl_shm* shm;
    struct wl_buffer* buffer;
    struct wl_buffer_listener bufferListener;
    void* shmData;
    size_t shmSize;
    bool bufferBusy;
    bool shouldClose;
    // Decoration
    struct zxdg_toplevel_decoration_v1_listener decorationListener;
    struct zxdg_decoration_manager_v1* decorationManager;
    struct zxdg_toplevel_decoration_v1* toplevelDecoration;
    // Input
    struct wl_seat* seat;
    struct wl_pointer* pointer;
    struct wl_keyboard* keyboard;
    struct wl_seat_listener seatListener;
    struct wl_pointer_listener pointerListener;
    struct wl_keyboard_listener keyboardListener;

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

// -------------------------------------- :Input ---------------------------------------

void swinWaylandPointerEnter(
    void* userdata, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface, wl_fixed_t sx,
    wl_fixed_t sy
) {
}

void swinWaylandPointerLeave(
    void* userdata, struct wl_pointer* pointer, uint32_t serial, struct wl_surface* surface
) {
}

void swinWaylandPointerAxis(
    void* userdata, struct wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value
) {
    Swin swin = (Swin)userdata;
    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL) {
        swin->input.scrollDelta += wl_fixed_to_double(value);
    }
}

void swinWaylandPointerFrame(void* userdata, struct wl_pointer* pointer) {
}

void swinWaylandPointerAxisSource(void* userdata, struct wl_pointer* pointer, uint32_t axis_source) {
}

void swinWaylandPointerAxisStop(void* userdata, struct wl_pointer* pointer, uint32_t time, uint32_t axis) {
}

void swinWaylandPointerAxisDiscrete(void* userdata, struct wl_pointer* pointer, uint32_t axis, int32_t discrete) {
}

void swinWaylandKeyboardKeymap(
    void* userdata, struct wl_keyboard* keyboard, uint32_t format, int fd, uint32_t size
) {
}

void swinWaylandKeyboardEnter(
    void* userdata, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface, struct wl_array* keys
) {
}

void swinWaylandKeyboardLeave(
    void* userdata, struct wl_keyboard* keyboard, uint32_t serial, struct wl_surface* surface
) {
}

void swinWaylandKeyboardModifiers(
    void* userdata, struct wl_keyboard* keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched,
    uint32_t mods_locked, uint32_t group
) {
}

void swinWaylandKeyboardRepeatInfo(void* userdata, struct wl_keyboard* keyboard, int32_t rate, int32_t delay) {
}

void swinWaylandPointerMotion(void* userdata, struct wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {
    Swin swin = (Swin)userdata;
    i32 x = wl_fixed_to_int(sx);
    i32 y = wl_fixed_to_int(sy);
    swin->input.mouseDeltaX += x - swin->input.mouseX;
    swin->input.mouseDeltaY += y - swin->input.mouseY;
    swin->input.mouseX = x;
    swin->input.mouseY = y;
}

void swinWaylandPointerButton(
    void* userdata, struct wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state
) {
    Swin swin = (Swin)userdata;
    bool down = state == WL_POINTER_BUTTON_STATE_PRESSED;
    if (button == BTN_LEFT) swin->input.mouse[0] = down;
    else if (button == BTN_RIGHT) swin->input.mouse[1] = down;
    else if (button == BTN_MIDDLE) swin->input.mouse[2] = down;
}

char swinWaylandConvertKey(u32 key) {
    switch (key) {
    case KEY_A: return 'a';
    case KEY_B: return 'b';
    case KEY_C: return 'c';
    case KEY_D: return 'd';
    case KEY_E: return 'e';
    case KEY_F: return 'f';
    case KEY_G: return 'g';
    case KEY_H: return 'h';
    case KEY_I: return 'i';
    case KEY_J: return 'j';
    case KEY_K: return 'k';
    case KEY_L: return 'l';
    case KEY_M: return 'm';
    case KEY_N: return 'n';
    case KEY_O: return 'o';
    case KEY_P: return 'p';
    case KEY_Q: return 'q';
    case KEY_R: return 'r';
    case KEY_S: return 's';
    case KEY_T: return 't';
    case KEY_U: return 'u';
    case KEY_V: return 'v';
    case KEY_W: return 'w';
    case KEY_X: return 'x';
    case KEY_Y: return 'y';
    case KEY_Z: return 'z';

    case KEY_0: return '0';
    case KEY_1: return '1';
    case KEY_2: return '2';
    case KEY_3: return '3';
    case KEY_4: return '4';
    case KEY_5: return '5';
    case KEY_6: return '6';
    case KEY_7: return '7';
    case KEY_8: return '8';
    case KEY_9: return '9';

    case KEY_ESC: return Swin_Esc;
    case KEY_LEFTCTRL: return Swin_CtrlLeft;
    case KEY_RIGHTCTRL: return Swin_CtrlRight;
    case KEY_LEFTSHIFT: return Swin_ShiftLeft;
    case KEY_RIGHTSHIFT: return Swin_ShiftRight;
    case KEY_LEFT: return Swin_ArrowLeft;
    case KEY_RIGHT: return Swin_ArrowRight;
    case KEY_UP: return Swin_ArrowUp;
    case KEY_DOWN: return Swin_ArrowDown;

    default: return 0;
    }
}

void swinWaylandKeyboardKey(
    void* userdata, struct wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state
) {
    Swin swin = (Swin)userdata;
    bool down = state == WL_KEYBOARD_KEY_STATE_PRESSED;
    swin->input.keys[key] = down;
}

void swinWaylandSeatCapabilities(
    void* userdata,
    struct wl_seat* seat,
    uint32_t capabilities
) {
    Swin swin = (Swin)userdata;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        swin->pointer = wl_seat_get_pointer(seat);

        swin->pointerListener.enter = swinWaylandPointerEnter;
        swin->pointerListener.leave = swinWaylandPointerLeave;
        swin->pointerListener.motion = swinWaylandPointerMotion;
        swin->pointerListener.button = swinWaylandPointerButton;
        swin->pointerListener.axis = swinWaylandPointerAxis;
        swin->pointerListener.frame = swinWaylandPointerFrame;
        swin->pointerListener.axis_source = swinWaylandPointerAxisSource;
        swin->pointerListener.axis_stop = swinWaylandPointerAxisStop;
        swin->pointerListener.axis_discrete = swinWaylandPointerAxisDiscrete;

        wl_pointer_add_listener(swin->pointer, &swin->pointerListener, swin);
    }

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        swin->keyboard = wl_seat_get_keyboard(seat);

        swin->keyboardListener.keymap = swinWaylandKeyboardKeymap;
        swin->keyboardListener.enter = swinWaylandKeyboardEnter;
        swin->keyboardListener.leave = swinWaylandKeyboardLeave;
        swin->keyboardListener.key = swinWaylandKeyboardKey;
        swin->keyboardListener.modifiers = swinWaylandKeyboardModifiers;
        swin->keyboardListener.repeat_info = swinWaylandKeyboardRepeatInfo;

        wl_keyboard_add_listener(swin->keyboard, &swin->keyboardListener, swin);
    }
}

// -------------------------------------------------------------------------------------

void swinWaylandRegistryGlobal(
    void* userdata,
    struct wl_registry* registry,
    uint32_t name,
    const char* interface,
    uint32_t version
) {
    Swin swin = (Swin)userdata;

    if (strcmp(interface, "wl_compositor") == 0) {
        swin->compositor = (struct wl_compositor*)wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, "xdg_wm_base") == 0) {
        swin->wmBase = (struct xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    }
    else if (strcmp(interface, "wl_shm") == 0) {
        swin->shm = (struct wl_shm*)wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, "zxdg_decoration_manager_v1") == 0) {
        swin->decorationManager = (struct zxdg_decoration_manager_v1*)wl_registry_bind(registry, name,
            &zxdg_decoration_manager_v1_interface, 1);
    }
    else if (strcmp(interface, "wl_seat") == 0) {
        swin->seat = (struct wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
    }
}

void swinWaylandRegistryGlobalRemove(void* userdata, struct wl_registry* registry, uint32_t name) {
}

void swinWaylandWmPing(void* userdata, struct xdg_wm_base* wmBase, uint32_t serial) {
    xdg_wm_base_pong(wmBase, serial);
}

void swinWaylandSurfaceConfigure(void* userdata, struct xdg_surface* surface, uint32_t serial) {
    xdg_surface_ack_configure(surface, serial);
}

void swinWaylandTopLevelConfigure(
    void* userdata, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states
) {
    Swin swin = (Swin)userdata;
    if (width > 0 && height > 0) {
        swin->width = width;
        swin->height = height;
        swin->resized = true;
    }
}

void swinWaylandTopLevelClose(void* userdata, struct xdg_toplevel* toplevel) {
    Swin swin = (Swin)userdata;
    swin->shouldClose = true;
}

void swinWaylandBufferRelease(void* userdata, struct wl_buffer* buffer) {
    Swin swin = (Swin)userdata;
    swin->bufferBusy = false;
}

void swinWaylandBufferInit(Swin swin) {
    i32 stride = swin->width * 4;
    i32 size = stride * swin->height;

    char name[64];
    snprintf(name, sizeof(name), "/swin-%d-%p", getpid(), (void*)swin);
    i32 fd = shm_open(name, O_CREAT | O_RDWR, 0600);
    shm_unlink(name);

    SWIN_ASSERT(fd >= 0);
    SWIN_ASSERT(ftruncate(fd, size) >= 0);

    swin->shmData = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    SWIN_ASSERT(swin->shmData && "Failed to mmap Wayland shared memory");

    swin->shmSize = size;
    struct wl_shm_pool* pool = wl_shm_create_pool(swin->shm, fd, size);
    SWIN_ASSERT(pool && "Failed to create Wayland shared memory pool");

    swin->buffer = wl_shm_pool_create_buffer(pool, 0, swin->width, swin->height, stride,
        WL_SHM_FORMAT_ARGB8888);
    SWIN_ASSERT(swin->buffer && "Failed to create Wayland buffer");

    wl_shm_pool_destroy(pool);
    close(fd);
}

void swinWaylandDecorationHandleConfigure(
    void* data, struct zxdg_toplevel_decoration_v1* decoration, u32 mode
) {
}

Swin swinInit(SwinInitInfo info) {
    Swin swin = (Swin)malloc(sizeof(struct Swin_t));
    SWIN_ASSERT(swin);
    memset(swin, 0, sizeof(struct Swin_t));

    SWIN_ASSERT(info.width > 0 && info.height > 0 && "Must provide info.width and info.height");
    swin->width = info.width;
    swin->height = info.height;

    swin->display = wl_display_connect(NULL);
    SWIN_ASSERT(swin->display && "Failed to connect to Wayland display");

    // Registry
    swin->registry = wl_display_get_registry(swin->display);
    swin->registryListener.global = swinWaylandRegistryGlobal;
    swin->registryListener.global_remove = swinWaylandRegistryGlobalRemove;
    wl_registry_add_listener(swin->registry, &swin->registryListener, swin);

    // Display
    SWIN_ASSERT(wl_display_roundtrip(swin->display) >= 0 && "Wayland registry roundtrip failed");
    SWIN_ASSERT(swin->compositor && "Failed to get wl_compositor");
    SWIN_ASSERT(swin->wmBase && "Failed to get xdg_wm_base");
    SWIN_ASSERT(swin->shm && "Failed to get wl_shm");
    SWIN_ASSERT(swin->seat && "Failed to get wl_seat");

    // Seat
    swin->seatListener.capabilities = swinWaylandSeatCapabilities;
    wl_seat_add_listener(swin->seat, &swin->seatListener, swin);

    // WM
    swin->wmListener.ping = swinWaylandWmPing;
    xdg_wm_base_add_listener(swin->wmBase, &swin->wmListener, swin);

    // Surface
    swin->surface = wl_compositor_create_surface(swin->compositor);
    SWIN_ASSERT(swin->surface && "Failed to create Wayland surface");

    // XDG surface
    swin->xdgSurface = xdg_wm_base_get_xdg_surface(swin->wmBase, swin->surface);
    SWIN_ASSERT(swin->xdgSurface && "Failed to create xdg_surface");
    swin->surfaceListener.configure = swinWaylandSurfaceConfigure;
    xdg_surface_add_listener(swin->xdgSurface, &swin->surfaceListener, swin);

    // Toplevel
    swin->toplevelListener.configure = swinWaylandTopLevelConfigure;
    swin->toplevelListener.close = swinWaylandTopLevelClose;
    swin->toplevel = xdg_surface_get_toplevel(swin->xdgSurface);
    SWIN_ASSERT(swin->toplevel && "Failed to create xdg_toplevel");
    xdg_toplevel_add_listener(swin->toplevel, &swin->toplevelListener, swin);

    // Decoration
    // SWIN_ASSERT(swin->decorationManager && "Failed to get zxdg_decoration_manager");
    swin->decorationListener.configure = swinWaylandDecorationHandleConfigure;
    if (swin->decorationManager) {
        swin->toplevelDecoration = zxdg_decoration_manager_v1_get_toplevel_decoration(
            swin->decorationManager, swin->toplevel);
        zxdg_toplevel_decoration_v1_add_listener(swin->toplevelDecoration, &swin->decorationListener, NULL);

        zxdg_toplevel_decoration_v1_set_mode(swin->toplevelDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
    }

    // Title
    const char* title = info.title ? info.title : "swin";
    xdg_toplevel_set_title(swin->toplevel, title);
    xdg_toplevel_set_app_id(swin->toplevel, title);

    // Configure
    wl_surface_commit(swin->surface);
    SWIN_ASSERT(wl_display_roundtrip(swin->display) >= 0 && "Wayland initial configure failed");

    // Buffer
    swinWaylandBufferInit(swin);
    swin->bufferListener.release = swinWaylandBufferRelease;
    wl_buffer_add_listener(swin->buffer, &swin->bufferListener, swin);

    // First frame
    memset(swin->shmData, 0xFF, swin->width * swin->height * sizeof(u32));
    wl_surface_attach(swin->surface, swin->buffer, 0, 0);
    wl_surface_damage_buffer(swin->surface, 0, 0, swin->width, swin->height);
    wl_surface_commit(swin->surface);
    swin->bufferBusy = true;
    SWIN_ASSERT(wl_display_flush(swin->display) >= 0 || errno == EAGAIN);

    return swin;
}

void swinQuit(Swin swin) {
    swin->shouldClose = true;
    // if (!swin) return;
    //
    // if (swin->buffer) wl_buffer_destroy(swin->buffer);
    // if (swin->shmData) munmap(swin->shmData, swin->shmSize);
    // if (swin->surface) wl_surface_destroy(swin->surface);
    // if (swin->xdgSurface) xdg_surface_destroy(swin->xdgSurface);
    // if (swin->toplevel) xdg_toplevel_destroy(swin->toplevel);
    // if (swin->wmBase) xdg_wm_base_destroy(swin->wmBase);
    // if (swin->compositor) wl_compositor_destroy(swin->compositor);
    // if (swin->registry) wl_registry_destroy(swin->registry);
    // if (swin->display) wl_display_disconnect(swin->display);
    //
    // free(swin);
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

    // Wait until the previous buffer is available
    while (swin->bufferBusy) {
        if (wl_display_dispatch(swin->display) < 0) return false;
        if (swin->shouldClose) return false;
    }

    // Process events that arrived with the buffer release
    if (wl_display_dispatch_pending(swin->display) < 0) return false;

    if (swin->shouldClose) return false;

    // Flush any requests generated while processing events
    if (wl_display_flush(swin->display) < 0 && errno != EAGAIN) return false;

    return true;
}

void swinDraw(Swin swin, const u32* pixels) {
    SWIN_ASSERT(!swin->bufferBusy); // TODO: wait

    memcpy(swin->shmData, pixels, swin->width * swin->height * sizeof(u32));
    wl_surface_attach(swin->surface, swin->buffer, 0, 0);
    wl_surface_damage_buffer(swin->surface, 0, 0, swin->width, swin->height);
    wl_surface_commit(swin->surface);
    swin->bufferBusy = true;

    if (wl_display_flush(swin->display) < 0 && errno != EAGAIN)
        return;
}

SwinInput swinInput(Swin swin) {
    return swin->input;
}

// -------------------------------------- :Window --------------------------------------

void swinSetWindowFlags(Swin swin, SwinWindowFlagBits flags) {
    if (flags & SwinWindow_Default) return;

    SWIN_ASSERT(!(flags & SwinWindow_Topmost) && "SwinWindow_Topmost is not supported on Wayland");

    swin->windowFlags = flags;

    // Fullscreen
    if (flags & SwinWindow_Fullscreen) xdg_toplevel_set_fullscreen(swin->toplevel, NULL);
    else xdg_toplevel_unset_fullscreen(swin->toplevel);

    // Resizable
    if (flags & SwinWindow_Resizable) {
        // No size limit
        xdg_toplevel_set_min_size(swin->toplevel, 0, 0);
        xdg_toplevel_set_max_size(swin->toplevel, 0, 0);
    }
    else {
        // Size limit is current size
        xdg_toplevel_set_min_size(swin->toplevel, swin->width, swin->height);
        xdg_toplevel_set_max_size(swin->toplevel, swin->width, swin->height);
    }

    // Decorations
    if (flags & SwinWindow_NoTopbar)
        zxdg_toplevel_decoration_v1_set_mode(swin->toplevelDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_CLIENT_SIDE);
    else zxdg_toplevel_decoration_v1_set_mode(swin->toplevelDecoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);

    wl_display_flush(swin->display);
}

void swinSetWindowSize(Swin swin, SwinSize size) {
    // TODO

    // wl_surface_commit(swin->surface);
    //
    // if (wl_display_flush(swin->display) < 0) {
    //     SWIN_ASSERT(errno == EAGAIN && "Failed to flush Wayland display");
    // }
}

#endif
