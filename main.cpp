#include "cstdio"

#define SWIN_IMPL
#include "swin.h"

int main() {
    u32 width = 400, height = 300;
    Swin swin = swinInit({.width = width, .height = height});
    swinSetWindowFlags(swin, SwinWindow_Topmost);

    u32* pixels = (u32*)malloc(width * height * sizeof(u32));

    while (swinPoll(swin)) {
        if (swinIsKeyDown(swin, Swin_Esc)) break;

        if (swinIsWindowResized(swin)) {
            SwinSize size = swinGetWindowSize(swin);
            width = size.x;
            height = size.y;

            free(pixels);
            pixels = (u32*)malloc(width * height * sizeof(u32));
        }

        for (u32 y = 0; y < height; y++) {
            for (u32 x = 0; x < width; x++) {
                u8 r = (u8)(x * 255 / width);
                u8 g = (u8)(y * 255 / height);
                u8 b = 128;
                u8 a = 255;
                pixels[y * width + x] = (a << 24) | (b << 16) | (g << 8) | r;
            }
        }
        swinDraw(swin, pixels);
    }
}
