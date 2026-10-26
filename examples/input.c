// Linux: cc input.c -lX11 -lXi
// Windows: clang input.c -luser32 -lgdi32

#define SWIN_IMPL
#include "../swin.h"

#include <stdio.h>

int main() {
    SwinInitInfo init = {};
    init.width = 400;
    init.height = 300;
    Swin swin = swinInit(init);

    while (swinPoll(swin)) {
        // Close window with escape
        if (swinIsKeyDown(swin, SwinKey_Esc)) break;

        // Keyboard keys
        if (swinIsKeyJustPressed(swin, 'W')) printf("Key W\n");
        if (swinIsKeyJustPressed(swin, 'A')) printf("Key A\n");
        if (swinIsKeyJustPressed(swin, 'S')) printf("Key S\n");
        if (swinIsKeyJustPressed(swin, 'D')) printf("Key D\n");

        // Mouse buttons
        if (swinIsMouseJustPressed(swin, SwinMouseButton_Left)) printf("Left mouse button\n");
        if (swinIsMouseJustPressed(swin, SwinMouseButton_Right)) printf("Right mouse button\n");
        if (swinIsMouseJustPressed(swin, SwinMouseButton_Middle)) printf("Middle mouse button\n");

        SwinPoint mouseDelta = swinGetMouseDelta(swin);
        SwinPoint mousePosition = swinGetMousePosition(swin);
        if (mouseDelta.x != 0 || mouseDelta.y != 0) {
            printf("Mouse delta: %d, %d\n", mouseDelta.x, mouseDelta.y);
            printf("Mouse position: %d, %d\n", mousePosition.x, mousePosition.y);
        }
    }
}
