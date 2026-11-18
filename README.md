# swin
Small header-only windowing library written in C99

Prioritizes fast window startup and a minimal API inspired by [Raylib](https://github.com/raysan5/raylib)

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

Currently supports Windows and Linux (X11)

Support for Wayland and MacOS is WIP

## building
Swin is a header only library. To use it just include the `swin.h` header and use `#define SWIN_IMPL` *once* to generate the function implementations.

There are no build dependencies. You just need to link the underlying platform window libraries when compiling your program.

### Windows
```
cl main.c user32.lib gdi32.lib
```
### Linux (X11)
```
cc main.c -lX11 -lXi
```
