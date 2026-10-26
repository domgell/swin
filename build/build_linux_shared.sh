#!/bin/bash
clang -fPIC -shared -g -std=c99 dummy.c -o libswin.so -D_GNU_SOURCE -D_POSIX_C_SOURCE=199309L