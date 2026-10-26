// Linux: cc drawing.c -lX11 -lXi
// Windows: clang drawing.c -luser32 -lgdi32

#define SWIN_IMPL
#include <stdio.h>

#include "../swin.h"

#include <stdlib.h>

int main() {
    u32 width = 400;
    u32 height = 300;

    SwinInitInfo init = {};
    init.width = width;
    init.height = height;
    Swin swin = swinInit(init);
    swinSetWindowFlags(swin, SwinWindow_Resizable);

    u32* pixels = (u32*)malloc(width * height * sizeof(u32));

    while (swinPoll(swin)) {
        // Resize pixels array on window resize
        if (swinIsWindowResized(swin)) {
            SwinSize size = swinGetWindowSize(swin);
            width = size.x;
            height = size.y;

            free(pixels);
            pixels = (u32*)malloc(width * height * sizeof(u32));
        }

        // Draw to pixels array
        for (u32 y = 0; y < height; y++) {
            for (u32 x = 0; x < width; x++) {
                f32 u = (f32)x / (f32)width;
                f32 v = (f32)y / (f32)height;
                u8 r = (u8)(u * 255.0f);
                u8 g = (u8)(v * 255.0f);
                u8 b = 128;
                u8 a = 255;
                pixels[y * width + x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
        }
        // Draw pixels array to screen
        swinDraw(swin, pixels);
    }
}
