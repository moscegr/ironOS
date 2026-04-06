#ifndef APPARCADE_H
#define APPARCADE_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"

// 📦 IMPORTAMOS TUS JUEGOS EXTERNOS
#include "AppNaves.h"
#include "AppSnake.h"

class AppArcade : public App {
private:
    enum EstadoArcade { MENU_ARCADE, EN_JUEGO };
    EstadoArcade estado = MENU_ARCADE;

    int indiceMenu = 0;
    String opciones[2] = {"1. GALAXY WAR", "2. RETRO SNAKE"};

    // Instanciamos los juegos dentro del Hub
    AppNaves naves;
    AppSnake snake;
    int juegoActivo = -1;

    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

public:
    const char* obtenerNombre() override { return "ARCADE HUB"; }
    const uint8_t* obtenerEmoji() override { return emoji_snake; } 
    
    void setup() override {
        // Inicializamos los sensores de ambos juegos
        naves.setup();
        snake.setup();
    }

    void alEntrar() override {
        estado = MENU_ARCADE;
        indiceMenu = 0;
        juegoActivo = -1;
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        ultimoMovimiento = millis();
        pantallaActualizada = false;
        btnAnterior = (digitalRead(JOY_SW) == LOW);
    }
    
    void alSalir() override {
        // Si el usuario da "doble clic" para salir de IronOS, apagamos el juego activo
        if (juegoActivo == 0) naves.alSalir();
        if (juegoActivo == 1) snake.alSalir();
    }

    void loop(Arduino_Canvas* canvas) override {
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // ==========================================
        // DIBUJAMOS Y CONTROLAMOS EL SUBMENÚ DE JUEGOS
        // ==========================================
        if (estado == MENU_ARCADE) {
            int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
            bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
            bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);

            if (millis() - ultimoFrameNeon > 30) {
                anguloNeon = (anguloNeon + 8) % 360; 
                ultimoFrameNeon = millis();
                pantallaActualizada = false; 
            }

            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenu--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenu++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenu < 0) indiceMenu = 1;
                if (indiceMenu > 1) indiceMenu = 0;
            }

            // Seleccionar un Juego
            if (clicSimple) {
                juegoActivo = indiceMenu;
                estado = EN_JUEGO;
                
                // Entramos al juego seleccionado y bajamos la bandera
                if (juegoActivo == 0) {
                    naves.regresarArcade = false;
                    naves.alEntrar();
                } else if (juegoActivo == 1) {
                    snake.regresarArcade = false;
                    snake.alEntrar();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, 0x07E0); // Verde Arcade
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, 0x07FF); 

                Launcher::dibujarTextoCentrado(canvas, "ARCADE ROOM", 40, 2, 0x07E0);
                canvas->drawLine(20, 60, 220, 60, 0x07FF);

                for (int i = 0; i < 2; i++) {
                    int yPos = 100 + (i * 40);
                    if (i == indiceMenu) {
                        canvas->fillRoundRect(20, yPos - 12, 200, 24, 3, 0xF800);
                        canvas->fillTriangle(25, yPos-4, 25, yPos+4, 30, yPos, WHITE);
                        Launcher::dibujarTextoCentrado(canvas, opciones[i].c_str(), yPos - 3, 1, WHITE);
                    } else {
                        Launcher::dibujarTextoCentrado(canvas, opciones[i].c_str(), yPos - 3, 1, 0x07FF);
                    }
                }
                
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, 0x07FF);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(0xF800);
                canvas->print("MANTENER CLICK");
                canvas->flush();
                pantallaActualizada = true;
            }
            btnAnterior = botonAhora; // Solo actualizamos botón si estamos en el menú
        } 
        // ==========================================
        // DELEGAMOS EL CONTROL AL JUEGO ACTIVO
        // ==========================================
        else if (estado == EN_JUEGO) {
            
            if (juegoActivo == 0) {
                naves.loop(canvas); // La app Arcade se duerme y Naves toma el control
                if (naves.regresarArcade) { // Si Naves levanta la bandera
                    naves.alSalir();
                    estado = MENU_ARCADE;
                    pantallaActualizada = false;
                    btnAnterior = botonAhora; 
                }
            } 
            else if (juegoActivo == 1) {
                snake.loop(canvas); // La app Arcade se duerme y Snake toma el control
                if (snake.regresarArcade) { // Si Snake levanta la bandera
                    snake.alSalir();
                    estado = MENU_ARCADE;
                    pantallaActualizada = false;
                    btnAnterior = botonAhora;
                }
            }
        }
    }
};
#endif