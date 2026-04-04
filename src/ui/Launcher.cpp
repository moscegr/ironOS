#include "ui/Launcher.h"

// --- LA FUNCIÓN RECUPERADA ---
void Launcher::dibujarTextoCentrado(Arduino_Canvas* canvas, const char* texto, int y, int tamano, uint16_t color) {
    int16_t x1, y1;
    uint16_t w, h;
    canvas->setTextSize(tamano);
    canvas->getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
    canvas->setTextColor(color);
    canvas->setCursor(120 - (w / 2), y);
    canvas->print(texto);
}

// --- FUNCIÓN DEL MENÚ (CARRUSEL) ---
void Launcher::dibujarItemListaCentrado(Arduino_Canvas* canvas, const uint8_t* icon, const char* text, int y_center, int size, uint16_t color, bool selected) {
    int16_t x1, y1;
    uint16_t w_text, h_text;
    canvas->setTextSize(size);
    canvas->getTextBounds(text, 0, 0, &x1, &y1, &w_text, &h_text);

    int icono_w = 16, icono_h = 16, padding = 8;
    int total_w = icono_w + padding + w_text;
    int start_x = 120 - (total_w / 2);

    canvas->drawBitmap(start_x, y_center - (icono_h / 2), icon, icono_w, icono_h, color);
    canvas->setTextColor(color);
    canvas->setCursor(start_x + icono_w + padding, y_center - (h_text / 2));
    canvas->print(text);

    if (selected) {
        // === MOTOR DE ANIMACIÓN ===
        // Usamos millis() para crear un movimiento fluido. 
        // El número divisor (15) controla qué tan rápido gira.
        int tiempo = millis();
        int anguloAnimado = (tiempo / 15) % 360; 

        // Dibujamos los arcos originales sumándoles el ángulo animado
        canvas->fillArc(120, 120, 118, 112, anguloAnimado - 40, anguloAnimado + 40, color); 
        canvas->fillArc(120, 120, 118, 112, anguloAnimado + 140, anguloAnimado + 220, color);
    }
}

void Launcher::dibujar(Arduino_Canvas* canvas, App** apps, int totalApps, int index) {
    canvas->fillScreen(BLACK);
    
    // Anillos Exteriores
    canvas->drawCircle(120, 120, 118, IRON_CYAN);
    canvas->drawArc(120, 120, 115, 105, 0, 360, IRON_DARK);
    
    // Lineas decorativas "Ventana"
    canvas->drawLine(30, 85, 210, 85, IRON_BLUE);
    canvas->drawLine(30, 155, 210, 155, IRON_BLUE);

    int prevIdx = (index - 1 < 0) ? totalApps - 1 : index - 1;
    int nextIdx = (index + 1 >= totalApps) ? 0 : index + 1;

    // Elementos de la lista
    dibujarItemListaCentrado(canvas, apps[prevIdx]->obtenerEmoji(), apps[prevIdx]->obtenerNombre(), 60, 1, IRON_BLUE, false);
    dibujarItemListaCentrado(canvas, apps[nextIdx]->obtenerEmoji(), apps[nextIdx]->obtenerNombre(), 180, 1, IRON_BLUE, false);
    dibujarItemListaCentrado(canvas, apps[index]->obtenerEmoji(), apps[index]->obtenerNombre(), 120, 3, WHITE, true);
    
    // Título Superior
    canvas->setTextSize(2);
    canvas->setTextColor(IRON_GREEN);
    canvas->setCursor(61, 25);
    canvas->print("MOS ironOS");
}