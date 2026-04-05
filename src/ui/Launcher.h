#ifndef LAUNCHER_H
#define LAUNCHER_H
#include "core/Display.h"
#include "App.h"

class Launcher {
private:
    static float indiceAnimado;
    static unsigned long ultimoFrame;

public:
    static void dibujarTextoCentrado(Arduino_Canvas* canvas, const char* texto, int y, int size, uint16_t color);
    static void dibujar(Arduino_Canvas* canvas, App** apps, int total, int seleccionado);
};
#endif