// clang++ -std=c++20 swintest.cpp -o test.exe -luser32 -lgdi32 ; ./test.exe

#include "swin.h"
#include <cstdlib>
#include <stdio.h>

int main() {
    Swin swin = swinInit({.width = 400, .height = 300, .title = "Test"});

    SwinSize size = swinGetWindowSize(swin);
    u32* pixels = (u32*)malloc(size.x * size.y * sizeof(u32));
    for (u32 i = 0; i < size.x * size.y; i++) {
        pixels[i] = 0xFF00FFFF;
    }

    while (swinPoll(swin)) {
        SwinInput input = swinInput(swin);
        if (swinIsKeyDown(swin, Swin_Esc)) break;

        if (swinIsWindowResized(swin)) {
            free(pixels);

            SwinSize size = swinGetWindowSize(swin);
            pixels = (u32*)malloc(size.x * size.y * sizeof(u32));
            for (u32 i = 0; i < size.x * size.y; i++) {
                pixels[i] = 0xFF00FFFF;
            }
        }

        char title[64];
        sprintf(title, "%.2f FPS", 1.0 / swinDeltaTime(swin));
        swinSetWindowTitle(swin, title);

        // if (swinIsMouseDown(swin, Swin_MouseLeft)) {
        //     swinSetMouseFlags(swin, SwinMouse_Capture | SwinMouse_Hidden);
        // } else {
        //     swinSetMouseFlags(swin, SwinMouse_Default);
        // }

        // SwinPoint mousePos = swinGetMousePosition(swin);
        // printf("x%d, y%d\n", mousePos.x, mousePos.y);
        // printf("x%d, y%d\n", input.mouseX, input.mouseY);
        // printf("x%d, y%d\n", input.mouseDeltaX, input.mouseDeltaY);

        swinDraw(swin, pixels);
    }

    swinQuit(swin);
}