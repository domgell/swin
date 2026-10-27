// Linux: cc minimal.c -lX11 -lXi
// Windows: clang minimal.c -luser32 -lgdi32
// MacOS: cc minimal.c -lm -framework Foundation -framework AppKit -framework CoreVideo
// MacOS: cc minimal.c

#define SWIN_IMPL
#include "../swin.h"

int main() {
    SwinInitInfo init = {};
    init.width = 400;
    init.height = 300;
    Swin swin = swinInit(init);

    while (swinPoll(swin)) {
    }
}
