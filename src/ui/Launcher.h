#ifndef LAUNCHER_H
#define LAUNCHER_H
#include "App.h"
#include "Emojis.h"

class Launcher {
public:
    // Dibuja el menú de lista vertical estilo smartwatch
    static void dibujar(Arduino_Canvas* canvas, App** apps, int totalApps, int currentIndex);
    
    // Función recuperada para que las apps puedan centrar sus propios textos
    static void dibujarTextoCentrado(Arduino_Canvas* canvas, const char* texto, int y, int tamano, uint16_t color);
    
private:
    // Utilidad privada para el menú
    static void dibujarItemListaCentrado(Arduino_Canvas* canvas, const uint8_t* icon, const char* text, int y_center, int size, uint16_t color, bool selected);
};
#endif