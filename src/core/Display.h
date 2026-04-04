#ifndef DISPLAY_H
#define DISPLAY_H
#include "config.h"
#include <Arduino_GFX_Library.h>

class Display {
public:
    static Arduino_Canvas* canvas;
    static Arduino_GFX* gfx;
    static void init();
    static void refresh();
};
#endif