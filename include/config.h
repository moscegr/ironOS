#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- PINES DE PANTALLA ---
#define TFT_SCLK 7
#define TFT_MOSI 6
#define TFT_CS   1 //cambio de 1 a 13 se puede probar
#define TFT_DC   4
#define TFT_RST  5

// --- PINES DE JOYSTICK ---
#define JOY_X  2 ///cambio de 2 a 14, se puede probar
#define JOY_Y  3
#define JOY_SW 8

// --- PINES DE TARJETA SD (Módulo Integrado ESP32-S3) ---
// Configuración nativa 1-Bit que evita el conflicto con la PSRAM
#define SD_MMC_CMD 38
#define SD_MMC_CLK 39
#define SD_MMC_D0  40

// Cambia esto:
// #define TIEMPO_DOBLE_CLICK 350 

// Por esto (800 milisegundos es el tiempo ideal para evitar salidas accidentales):
#define TIEMPO_PRESION_LARGA 800

// ============================================================
//  MARAUDER OS — Paleta de color dinámica por app
//  Cada app define su propio color en obtenerColor().
//  Si la app no implementa obtenerColor(), se usa CYBER_CYAN.
// ============================================================
#define CYBER_CYAN    0x07FF  // #00FFFF — acento principal
#define CYBER_LIME    0x07E0  // #00FF00 — estado / OK
#define CYBER_YELLOW  0xFFE0  // #FFFF00 — icono activo
#define CYBER_MAGENTA 0xF81F  // #FF00FF — separador HUD
#define CYBER_DARK    0x0841  // #080808 — fondo del círculo
#define CYBER_DIMGRAY 0x39C7  // #383838 — grid BG lines
#define IRON_RED      0xF800  // #FF0000 — alerta / footer
#define COLOR_WHITE   0xFFFF
#define COLOR_BLACK   0x0000
#define IRON_CYAN 0x07FF
#define IRON_BLUE 0x001F
#define IRON_DARK 0xD6BA
#define IRON_GREEN 0x07E0

// --- MOTOR DE CALIBRACIÓN DE JOYSTICK Y PANTALLA ---
extern bool joyEjeY_esVertical;
extern bool joyInvVertical;
extern bool joyInvHorizontal;
extern int rotacionPantalla; // NUEVO: Variable global de rotación

#endif