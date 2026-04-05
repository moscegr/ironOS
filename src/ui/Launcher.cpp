#include "Launcher.h"
#include "config.h"
#include "Emojis.h"



// ============================================================
// COLOR ÚNICO POR APP (Mantenemos tu lógica de índices)
// ============================================================
static const uint16_t APP_COLORS[] = {
    0x4FEA,  // 0  Wi-Fi Scanner  (Verde)
    0x65FF,  // 1  BLE Scanner   (Azul)
    0xFF84,  // 2  SD Card       (Ámbar)
    0x07F9,  // 3  Telemetría    (Cian)
    0xF80A,  // 4  WiFi Marauder (Rojo)
    0xC81F,  // 5  BLE Marauder  (Magenta)
    0xFCC4,  // 6  Evil Twin     (Naranja)
    0x4FEF,  // 7  Iron Ducky    (Verde brillante)
    0x7C5F,  // 8  WiFi Sniffer  (Azul cielo)
    0xB41F   // 9  Settings      (Violeta)
};
#define N_APP_COLORS (int)(sizeof(APP_COLORS) / sizeof(APP_COLORS[0]))

static inline uint16_t colorDeApp(int idx) {
    if (idx >= 0 && idx < N_APP_COLORS) return APP_COLORS[idx];
    return CYBER_CYAN;
}

// ============================================================
// Motor de renderizado escalado
// ============================================================
void dibujarBitmapEscalado(Arduino_Canvas* canvas, int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color, int escala) {
    int16_t byteWidth = (w + 7) / 8;
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (pgm_read_byte(bitmap + j * byteWidth + i / 8) & (128 >> (i & 7))) {
                canvas->fillRect(x + i * escala, y + j * escala, escala, escala, color);
            }
        }
    }
}

void Launcher::dibujarTextoCentrado(Arduino_Canvas* canvas, const char* texto, int y, int size, uint16_t color) {
    canvas->setTextSize(size);
    canvas->setTextColor(color);
    int16_t x1, y1; uint16_t w, h;
    canvas->getTextBounds(texto, 0, y, &x1, &y1, &w, &h);
    canvas->setCursor((240 - w) / 2, y);
    canvas->print(texto);
}

// Crosshair táctico de 4 esquinas
static void cornerFrame(Arduino_Canvas* canvas, int cx, int cy, int r, uint16_t color) {
    const int L = 10;
    canvas->drawLine(cx-r, cy-r, cx-r+L, cy-r, color);
    canvas->drawLine(cx-r, cy-r, cx-r, cy-r+L, color);
    canvas->drawLine(cx+r, cy-r, cx+r-L, cy-r, color);
    canvas->drawLine(cx+r, cy-r, cx+r, cy-r+L, color);
    canvas->drawLine(cx-r, cy+r, cx-r+L, cy+r, color);
    canvas->drawLine(cx-r, cy+r, cx-r, cy+r-L, color);
    canvas->drawLine(cx+r, cy+r, cx+r-L, cy+r, color);
    canvas->drawLine(cx+r, cy+r, cx+r, cy+r-L, color);
}

// 8 ticks de graduación
static void tickRing(Arduino_Canvas* canvas, int cx, int cy, int r, uint16_t color) {
    static const int8_t DX[8] = { 0, 71, 100, 71, 0, -71, -100, -71 };
    static const int8_t DY[8] = { -100, -71, 0, 71, 100, 71, 0, -71 };
    for (int i = 0; i < 8; i++) {
        int x1 = cx + (DX[i] * r) / 100;
        int y1 = cy + (DY[i] * r) / 100;
        int x2 = cx + (DX[i] * (r + 6)) / 100;
        int y2 = cy + (DY[i] * (r + 6)) / 100;
        canvas->drawLine(x1, y1, x2, y2, color);
    }
}

// ============================================================
// RENDER PRINCIPAL
// ============================================================
void Launcher::dibujar(Arduino_Canvas* canvas, App** apps, int total, int seleccionado) {
    const uint16_t appColor = colorDeApp(seleccionado);

    // 1. Fondo Negro + Scanlines CRT
    canvas->fillScreen(BLACK);
    for (int y = 0; y < 240; y += 6)
        canvas->drawLine(0, y, 239, y, CYBER_DIMGRAY);

    // 2. Geometría Central
    const int CX = 120;
    const int CY = 120;
    const int R  = 84; 

    canvas->drawCircle(CX, CY, R + 12, CYBER_DARK);
    canvas->drawCircle(CX, CY, R + 8, CYBER_DARK);
    canvas->fillCircle(CX, CY, R - 1, 0x0082); // Sutil resplandor azul fondo
    canvas->drawCircle(CX, CY, R, appColor);
    canvas->drawCircle(CX, CY, R - 4, CYBER_DARK);

    tickRing(canvas, CX, CY, R, appColor);
    cornerFrame(canvas, CX, CY, R - 6, CYBER_CYAN);

    // 3. Icono Centrado (Escala 3 = 96x96 px)
    const uint8_t* emoji = apps[seleccionado]->obtenerEmoji();
    if (emoji) {
        dibujarBitmapEscalado(canvas, CX - 48, CY - 38 - 10, emoji, 32, 32, appColor, 3);
    }

    // 4. Nombre de la App (Ajustado dentro del anillo)
    canvas->fillRect(CX - 68, CY + 46, 136, 20, BLACK);
    canvas->drawLine(CX - 52, CY + 44, CX + 52, CY + 44, appColor);
    canvas->fillRect(CX - 8, CY + 36, 16, 2, CYBER_MAGENTA);
    dibujarTextoCentrado(canvas, apps[seleccionado]->obtenerNombre(), CY + 50, 2, WHITE);

    // 5. Header HUD (Y: 10 - 30)
    canvas->fillRect(30, 10, 180, 22, BLACK); // Caja negra para que no se vea el scanline tras el texto
    canvas->drawLine(40, 32, 200, 32, CYBER_LIME);
    
    canvas->setTextSize(1);
    canvas->setTextColor(CYBER_LIME);
    canvas->setCursor(70, 18);
    canvas->print("MENU");

    canvas->setTextColor(CYBER_CYAN);
    canvas->setCursor(140, 18);
    char idxStr[8];
    snprintf(idxStr, sizeof(idxStr), "%02d/%02d", seleccionado + 1, total);
    canvas->print(idxStr);
    canvas->fillRect(116, 18, 8, 2, CYBER_MAGENTA);

    // 6. Flechas de Navegación
    canvas->setTextSize(4);
    canvas->setTextColor(appColor);
    canvas->setCursor(6, CY - 15);   canvas->print("<");
    canvas->setCursor(215, CY - 15);  canvas->print(">");

    // 7. Footer HUD (Y: 210 - 230)
    //canvas->fillRect(30, 205, 180, 22, BLACK);
    canvas->drawLine(40, 210, 200, 210, CYBER_LIME);

    canvas->setTextSize(1);
    canvas->setTextColor(IRON_RED);
    canvas->setCursor(105, 220);
    canvas->print("IRONOS");

   
}