#include "Launcher.h"
#include "config.h"
#include "Emojis.h" 

// Colores Cyberpunk (Máxima saturación RGB565)
#define CYBER_MAGENTA 0xF81F
#define CYBER_CYAN    0x07FF
#define CYBER_YELLOW  0xFFE0
#define CYBER_LIME    0x07E0
#define CYBER_DARK    0x10A2 // Gris azulado para fondos

// Motor de Renderizado Escalado: Multiplica píxeles sin gastar más memoria Flash
void dibujarBitmapEscalado(Arduino_Canvas* canvas, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t color, int escala) {
    int16_t byteWidth = (w + 7) / 8; // Bytes por fila
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            // Leer el bit específico de PROGMEM
            if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
                // Dibujar un rectángulo del tamaño de la escala en lugar de un solo píxel
                canvas->fillRect(x + i * escala, y + j * escala, escala, escala, color);
            }
        }
    }
}

void Launcher::dibujarTextoCentrado(Arduino_Canvas* canvas, const char* texto, int y, int size, uint16_t color) {
    canvas->setTextSize(size);
    canvas->setTextColor(color);
    int16_t x1, y1;
    uint16_t w, h;
    canvas->getTextBounds(texto, 0, y, &x1, &y1, &w, &h);
    canvas->setCursor((240 - w) / 2, y);
    canvas->print(texto);
}

void Launcher::dibujar(Arduino_Canvas* canvas, App** apps, int total, int seleccionado) {
    canvas->fillScreen(BLACK);

    // --- FLECHAS DE NAVEGACIÓN LATERALES (Centradas con el icono masivo) ---
    canvas->setTextSize(3);
    canvas->setTextColor(CYBER_LIME);
    canvas->setCursor(12, 80); canvas->print("<"); 
    canvas->setCursor(212, 80); canvas->print(">");

    // --- DECORACIONES DEL HUD TÁCTICO CENTRAL ---
    canvas->drawLine(30, 160, 210, 160, CYBER_DARK); // Línea divisoria
    canvas->fillRect(110, 159, 20, 3, CYBER_MAGENTA); // Acento central

    // --- ICONO CENTRAL ENMARCADO (ESCALADO A 64x64 PÍXELES) ---
    int cx = 120;
    int cy = 90; // Centro geométrico del visor

    // Brackets ajustados perfectamente para rodear el icono gigante de 64x64
    int offset = 40; // Caja total de 80x80
    int length = 16; // Esquinas más largas
    
    // Arriba Izquierda
    canvas->drawLine(cx - offset, cy - offset, cx - offset + length, cy - offset, CYBER_CYAN);
    canvas->drawLine(cx - offset, cy - offset, cx - offset, cy - offset + length, CYBER_CYAN);
    // Arriba Derecha
    canvas->drawLine(cx + offset, cy - offset, cx + offset - length, cy - offset, CYBER_CYAN);
    canvas->drawLine(cx + offset, cy - offset, cx + offset, cy - offset + length, CYBER_CYAN);
    // Abajo Izquierda
    canvas->drawLine(cx - offset, cy + offset, cx - offset + length, cy + offset, CYBER_CYAN);
    canvas->drawLine(cx - offset, cy + offset, cx - offset, cy + offset - length, CYBER_CYAN);
    // Abajo Derecha
    canvas->drawLine(cx + offset, cy + offset, cx + offset - length, cy + offset, CYBER_CYAN);
    canvas->drawLine(cx + offset, cy + offset, cx + offset, cy + offset - length, CYBER_CYAN);

    // Extraer el emoji y escalarlo matemáticamente x2
    const uint8_t* emoji = apps[seleccionado]->obtenerEmoji();
    if (emoji) {
        // Al multiplicar por 2, el ancho es 64. Lo restamos al centro (cx - 32)
        dibujarBitmapEscalado(canvas, cx - 32, cy - 32, emoji, 32, 32, CYBER_YELLOW, 2);
    }

    // --- NOMBRE DE LA APP (Ajustado abajo del HUD) ---
    dibujarTextoCentrado(canvas, apps[seleccionado]->obtenerNombre(), 172, 2, WHITE);

    // --- HUD INFERIOR: SALIDA SENCILLA ---
    // Flecha sencilla <- solicitada por el Ingeniero
    dibujarTextoCentrado(canvas, "MENU PRINCIPAL", 205, 1, IRON_RED);
    dibujarTextoCentrado(canvas, "(ironOS)", 215, 1, IRON_RED);
    
}