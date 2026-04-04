#include "core/Display.h"

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED);
Arduino_GFX *Display::gfx = new Arduino_GC9A01(bus, TFT_RST, 0, true);
// Usamos el canvas de 240x240 para gráficos fluidos sin parpadeo
Arduino_Canvas *Display::canvas = new Arduino_Canvas(240, 240, Display::gfx);

void Display::init() {
    gfx->begin();
    canvas->begin();
}

void Display::refresh() {
    canvas->flush();
}