#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// --- PINES DE PANTALLA ---
#define TFT_SCLK 7
#define TFT_MOSI 6
#define TFT_CS   1
#define TFT_DC   4
#define TFT_RST  5

// --- PINES DE JOYSTICK ---
#define JOY_X  2
#define JOY_Y  3
#define JOY_SW 8

// --- PINES DE TARJETA SD (Módulo Externo SPI Seguro) ---
// Conecta tu módulo externo a estos pines para evitar el conflicto con la RAM
#define SD_SCK  12
#define SD_MISO 13
#define SD_MOSI 11
#define SD_CS   10

#define TIEMPO_DOBLE_CLICK 350 

#define IRON_CYAN 0x07FF
#define IRON_BLUE 0x001F
#define IRON_RED  0xF800
#define IRON_DARK 0xD6BA
#define IRON_GREEN 0x07E0

// --- MOTOR DE CALIBRACIÓN DE JOYSTICK ---
extern bool joyEjeY_esVertical;
extern bool joyInvVertical;
extern bool joyInvHorizontal;

#endif