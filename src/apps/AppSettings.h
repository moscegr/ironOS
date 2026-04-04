#ifndef APPSETTINGS_H
#define APPSETTINGS_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include <Preferences.h>
#include "config.h" // Incluimos la paleta de colores y variables globales

extern Preferences preferencias;

class AppSettings : public App {
private:
    int sel = 0; 
    bool pantallaActualizada = false;
    unsigned long ultimoCambio = 0;
    
    // Variables para la animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

public:
    const char* obtenerNombre() override { return "AJUSTES"; }
    const uint8_t* obtenerEmoji() override { return emoji_settings; }
    
    void setup() override {}
    
    void alEntrar() override { 
        pantallaActualizada = false; 
        sel = 0; 
        anguloNeon = 0;
        ultimoFrameNeon = millis();
    }
    
    void alSalir() override {}

    void loop(Arduino_Canvas* canvas) override {
        // --- MOTOR UNIFICADO DE LECTURA ---
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);

        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool izq = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool der = joyInvHorizontal ? (valHoriz < 800) : (valHoriz > 3200);

        // --- ANIMACIÓN NEÓN GLOBAL ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; // Fuerza el redibujo continuo
        }

        // --- LÓGICA DEL MENÚ DE CALIBRACIÓN ---
        if (millis() - ultimoCambio > 200) {
            if (arriba) { sel--; ultimoCambio = millis(); pantallaActualizada = false; }
            if (abajo) { sel++; ultimoCambio = millis(); pantallaActualizada = false; }
            
            if (sel < 0) sel = 2;
            if (sel > 2) sel = 0;

            // Ajustar el valor seleccionado con los lados del joystick
            if (izq || der) {
                if (sel == 0) joyEjeY_esVertical = !joyEjeY_esVertical;
                if (sel == 1) joyInvVertical = !joyInvVertical;
                if (sel == 2) joyInvHorizontal = !joyInvHorizontal;

                preferencias.putBool("ejeYV", joyEjeY_esVertical);
                preferencias.putBool("invV", joyInvVertical);
                preferencias.putBool("invH", joyInvHorizontal);
                
                ultimoCambio = millis();
                pantallaActualizada = false;
            }
        }

        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);
            
            // --- ESTELA DE NEÓN CYAN TÁCTICO ---
            canvas->drawCircle(120, 120, 118, IRON_CYAN);
            canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
            canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
            canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 
            
            Launcher::dibujarTextoCentrado(canvas, "CALIBRAR MANDO", 35, 1, IRON_CYAN);
            canvas->drawLine(20, 45, 220, 45, IRON_DARK);

            char l1[32], l2[32], l3[32];
            sprintf(l1, "EJE VERTICAL: %s", joyEjeY_esVertical ? "Y" : "X");
            sprintf(l2, "INV. VERTICAL: %s", joyInvVertical ? "SI" : "NO");
            sprintf(l3, "INV. HORIZ: %s", joyInvHorizontal ? "SI" : "NO");

            for(int i=0; i<3; i++) {
                uint16_t color = (i == sel) ? WHITE : IRON_BLUE;
                int yPos = 80 + (i * 30);
                
                // Resaltar la opción seleccionada
                if (i == sel) {
                    canvas->fillRoundRect(10, yPos - 12, 220, 24, 3, IRON_DARK);
                    canvas->fillTriangle(15, yPos-4, 15, yPos+4, 20, yPos, IRON_CYAN); 
                    canvas->fillTriangle(225, yPos-4, 225, yPos+4, 220, yPos, IRON_CYAN); 
                }
                
                const char* txt = (i==0)?l1:(i==1)?l2:l3;
                Launcher::dibujarTextoCentrado(canvas, txt, yPos - 3, 1, color);
            }

            Launcher::dibujarTextoCentrado(canvas, "< Ajustar >", 175, 1, IRON_DARK);

            canvas->drawBitmap(120 - 45, 195, emoji_back, 16, 16, IRON_CYAN);
            canvas->setCursor(120 - 25, 195);
            canvas->setTextColor(IRON_RED);
            canvas->setTextSize(1);
            canvas->print("DOBLE CLICK");

            canvas->flush();
            pantallaActualizada = true;
        }
        
        delay(10);
    }
};
#endif