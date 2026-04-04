#ifndef APPINFO_H
#define APPINFO_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" // Importante para usar los colores y el joystick unificado

class AppInfo : public App {
private:
    int indiceScroll = 0;
    unsigned long ultimoMovimiento = 0;
    
    // Variables para la animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    bool pantallaActualizada = false;

    // --- DATOS DEL PROYECTO ---
    const int NUM_LINEAS = 8;
    String lineasInfo[8] = {
        "OS: IronOS v1.0",
        "DEV: Ing. CEGR",
        "TIPO: Ciberdefensa",
        "ENTORNO: Hacker",
        "MCU: ESP32-S3 WROOM",
        "MEM: 16MB FL / 8MB OPI",
        "DISP: 1.28\" TFT GC9A01",
        "UI: Motor Unificado"
    };

public:
    const char* obtenerNombre() override { return "INFORMACION"; }
    const uint8_t* obtenerEmoji() override { return emoji_info; }
    
    void setup() override {}
    
    void alEntrar() override {
        indiceScroll = 0;
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        ultimoMovimiento = millis();
        pantallaActualizada = false;
    }
    
    void alSalir() override {}

    void loop(Arduino_Canvas* canvas) override {
        
        // --- MOTOR UNIFICADO DE LECTURA ---
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);

        // --- ANIMACIÓN NEÓN GLOBAL ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; // Fuerza el redibujo continuo
        }

        // --- LÓGICA DE SCROLL CON JOYSTICK ---
        if (millis() - ultimoMovimiento > 200) {
            if (arriba) { indiceScroll--; pantallaActualizada = false; ultimoMovimiento = millis(); }
            if (abajo) {  indiceScroll++; pantallaActualizada = false; ultimoMovimiento = millis(); }
            
            // Loop infinito de la lista
            if (indiceScroll < 0) indiceScroll = NUM_LINEAS - 1;
            if (indiceScroll >= NUM_LINEAS) indiceScroll = 0;
        }

        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);
            
            // Efecto Neón Cyan (Tono Informativo y Seguro)
            canvas->drawCircle(120, 120, 118, IRON_CYAN);
            canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
            canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
            canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 

            Launcher::dibujarTextoCentrado(canvas, "SISTEMA IRON OS", 35, 1, IRON_CYAN);
            canvas->drawLine(30, 45, 210, 45, IRON_DARK);

            // --- RENDERIZADO DE LA LISTA DINÁMICA ---
            // Calculamos qué elementos mostrar para que el seleccionado siempre esté visible
            int inicio = indiceScroll - 1;
            if (inicio < 0) inicio = 0;
            if (NUM_LINEAS > 4 && inicio > NUM_LINEAS - 4) inicio = NUM_LINEAS - 4;

            // Dibujamos máximo 4 líneas a la vez para que quepan en el centro de la pantalla
            for (int i = 0; i < 4 && (inicio + i) < NUM_LINEAS; i++) {
                int idx = inicio + i;
                int yPos = 65 + (i * 28); // Espaciado entre líneas

                if (idx == indiceScroll) {
                    // Resaltado de la línea seleccionada
                    canvas->fillRoundRect(15, yPos - 7, 210, 20, 3, IRON_DARK);
                    canvas->fillTriangle(20, yPos-1, 20, yPos+7, 25, yPos+3, IRON_CYAN);
                    canvas->setTextColor(WHITE);
                } else {
                    // Líneas inactivas
                    canvas->setTextColor(IRON_BLUE);
                }

                canvas->setTextSize(1);
                canvas->setCursor(30, yPos);
                canvas->print(lineasInfo[idx]);
            }

            // --- INSTRUCCIÓN DE SALIDA ---
            canvas->drawBitmap(120 - 45, 200, emoji_back, 16, 16, IRON_CYAN);
            canvas->setCursor(120 - 25, 200);
            canvas->setTextColor(IRON_RED);
            canvas->print("DOBLE CLICK");

            canvas->flush();
            pantallaActualizada = true;
        }
        
        delay(10);
    }
};
#endif