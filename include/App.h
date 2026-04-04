#ifndef APP_H
#define APP_H
#include <Arduino_GFX_Library.h>
#include "config.h" // Colores disponibles

class App {
public:
    virtual ~App() {}
    virtual void setup() = 0;                  // Configuración al encender el dispositivo
    virtual void alEntrar() {}                 // Lógica al abrir la app desde el menú
    virtual void alSalir() {}                  // Lógica al cerrar la app
    virtual void loop(Arduino_Canvas* canvas) = 0; // Bucle principal de la app
    virtual const char* obtenerNombre() = 0;   // Nombre mostrado debajo del icono
    virtual const uint8_t* obtenerEmoji() = 0; // Arreglo de datos del emoji (16x16)
};
#endif