#pragma once

#ifdef SWIN_IMPL

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
    SwinInput input = swinInput(swin);
    return input.keys[key];
}

bool swinIsMouseDown(Swin swin, u8 button) {
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

#endif