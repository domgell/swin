// cc minimal.c -lX11 -lXi

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
