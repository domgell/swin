# swin

Small header-only windowing library written in C99

Prioritizes fast window startup and an minimal API inspired by [Raylib](https://github.com/raysan5/raylib)

```c
#define SWIN_IMPL
#include "swin.h"

int main() {
    SwinInitInfo init = {};
    init.width = 400;
    init.height = 300;
    Swin swin = swinInit(init);
    
    while (swinPoll(swin)) {
        if (swinIsKeyDown(swin, SwinKey_ESC) {
            swinQuit(swin);
        }
    }
}
```

Currently supports Windows, Linux X11

Support for Wayland and MacOS is WIP

