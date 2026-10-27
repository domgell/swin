#pragma once
#include "../swin.h"

// TODO:
// union SwinPlatformState { SwinPlatformType type; ... }
// SwinPlatformState swinGetPlatformState(Swin swin)

// -------------------------------------- :Input ---------------------------------------

bool swinIsKeyDown(Swin swin, SwinKeyCode key) {
    if (key >= 'A' && key <= 'Z') key += ('a' - 'A');

    // TODO: check is valid key
    SwinInput input = swinInput(swin);
    return input.keys[key];
}

bool swinIsKeyJustPressed(Swin swin, SwinKeyCode key) {
    if (key >= 'A' && key <= 'Z') key += ('a' - 'A');

    SwinInput input = swinInput(swin);
    SwinInput prevInput = swinPrevInput(swin);
    return input.keys[key] && !prevInput.keys[key];
}

bool swinIsKeyJustReleased(Swin swin, SwinKeyCode key) {
    SwinInput input = swinInput(swin);
    SwinInput prevInput = swinPrevInput(swin);
    return !input.keys[key] && prevInput.keys[key];
}

bool swinIsMouseDown(Swin swin, SwinMouseButton button) {
    SwinInput input = swinInput(swin);
    return input.mouse[button];
}

bool swinIsMouseJustPressed(Swin swin, SwinMouseButton button) {
    SwinInput input = swinInput(swin);
    SwinInput prevInput = swinPrevInput(swin);
    return input.mouse[button] && !prevInput.mouse[button];
}

bool swinIsMouseJustReleased(Swin swin, SwinMouseButton button) {
    SwinInput input = swinInput(swin);
    SwinInput prevInput = swinPrevInput(swin);
    return !input.mouse[button] && prevInput.mouse[button];
}

SwinPoint swinMapKeys(Swin swin, SwinKeyCode forward, SwinKeyCode left, SwinKeyCode back, SwinKeyCode right) {
    SwinPoint dir = {0, 0};
    SwinInput input = swinInput(swin);
    if (input.keys[forward]) dir.y += 1;
    if (input.keys[back]) dir.y -= 1;
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

// SwinKeyCode swinImplNormalizeKeyCode(SwinKeyCode key) {
//     if (key >= 'A' && key <= 'Z') key += ('a' - 'A');
//     return key;
// }