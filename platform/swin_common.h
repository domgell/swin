#pragma once

#ifdef SWIN_IMPL

// TODO:
// union SwinPlatformState { SwinPlatformType type; ... }
// SwinPlatformState swinGetPlatformState(Swin swin)

// -------------------------------------- :Macros --------------------------------------

#if !defined(SWIN_MALLOC) || !defined(SWIN_FREE)
#include <stdlib.h>
#define SWIN_MALLOC malloc
#define SWIN_FREE free
#endif

#ifndef SWIN_ASSERT
#include <assert.h>
#define SWIN_ASSERT assert
#endif

// -------------------------------------- :Input ---------------------------------------

bool swinIsKeyDown(Swin swin, u32 key) {
    // TODO: check is valid key
    SwinInput input = swinInput(swin);
    return input.keys[key];
}

bool swinIsMouseDown(Swin swin, u8 button) {
    // TODO: check is valid mouse button
    SwinInput input = swinInput(swin);
    return input.mouse[button];
}

SwinPoint swinMapKeys(Swin swin, u8 forward, u8 left, u8 backward, u8 right) {
    SwinPoint dir = {0, 0};
    SwinInput input = swinInput(swin);
    if (input.keys[forward]) dir.y += 1;
    if (input.keys[backward]) dir.y -= 1;
    if (input.keys[left]) dir.x -= 1;
    if (input.keys[right]) dir.x += 1;
    return dir;
}

SwinPoint swinGetMousePosition(Swin swin) {
    SwinInput input = swinInput(swin);
    SwinPoint pos = {input.mouseX, input.mouseY};
    return pos;
}

SwinPoint swinGetMouseDelta(Swin swin) {
    SwinInput input = swinInput(swin);
    SwinPoint delta = {input.mouseDeltaX, input.mouseDeltaY};
    return delta;
}

#endif