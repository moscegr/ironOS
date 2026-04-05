#ifndef APPSNAKE_H
#define APPSNAKE_H
#include "App.h"
#include "ui/Launcher.h"

#define SNAKE_GREEN 0x07E0
#define SNAKE_RED   0xF800
#define SNAKE_BG    0x0000

class AppSnake : public App {
private:
    enum EstadoJuego { JUGANDO, GAMEOVER };
    EstadoJuego estado = JUGANDO;

    int snakeX[100], snakeY[100];
    int snakeLength = 3;
    int dirX = 1, dirY = 0;
    int comidaX, comidaY;
    int puntuacion = 0;
    
    unsigned long ultimoMovimientoSnake = 0;
    int velocidad = 150; 

    bool btnAnterior = false;
    bool pantallaActualizada = false;
    
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    unsigned long ultimoBoton = 0;

    void generarComida() {
        comidaX = random(3, 21) * 10;
        comidaY = random(4, 20) * 10;
    }

    void iniciarJuego() {
        snakeLength = 3;
        puntuacion = 0;
        velocidad = 150;
        dirX = 1; dirY = 0;
        snakeX[0] = 120; snakeY[0] = 120;
        snakeX[1] = 110; snakeY[1] = 120;
        snakeX[2] = 100; snakeY[2] = 120;
        generarComida();
        
        estado = JUGANDO;
        regresarArcade = false;
        pantallaActualizada = false;
        ultimoBoton = millis();
    }

public:
    // 🚩 BANDERA DE SALIDA
    bool regresarArcade = false; 

    const char* obtenerNombre() override { return "RETRO SNAKE"; }
    const uint8_t* obtenerEmoji() override { return nullptr; } 

    void setup() override {}

    void alEntrar() override {
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        iniciarJuego();
    }
    
    void alSalir() override {}

    void loop(Arduino_Canvas* canvas) override {
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);

        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool izq = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool der = joyInvHorizontal ? (valHoriz < 800) : (valHoriz > 3200);
        
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // Anillo Neón Rotatorio
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false;
        }

        if (estado == JUGANDO) {
            // Control de Dirección
            if (arriba && dirY == 0) { dirX = 0; dirY = -1; }
            if (abajo && dirY == 0)  { dirX = 0; dirY = 1; }
            if (izq && dirX == 0)    { dirX = -1; dirY = 0; }
            if (der && dirX == 0)    { dirX = 1; dirY = 0; }

            if (millis() - ultimoMovimientoSnake > velocidad) {
                ultimoMovimientoSnake = millis();

                // Desplazar cuerpo
                for (int i = snakeLength - 1; i > 0; i--) {
                    snakeX[i] = snakeX[i - 1];
                    snakeY[i] = snakeY[i - 1];
                }
                
                // Mover cabeza
                snakeX[0] += dirX * 10;
                snakeY[0] += dirY * 10;

                // Colisiones Muro (Cuadro jugable 20x220)
                if (snakeX[0] < 20 || snakeX[0] >= 220 || snakeY[0] < 30 || snakeY[0] >= 210) {
                    estado = GAMEOVER;
                    pantallaActualizada = false;
                    ultimoBoton = millis();
                }

                // Colisión Cuerpo
                for (int i = 1; i < snakeLength; i++) {
                    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
                        estado = GAMEOVER;
                        pantallaActualizada = false;
                        ultimoBoton = millis();
                    }
                }

                // Alimentación
                if (snakeX[0] == comidaX && snakeY[0] == comidaY) {
                    if (snakeLength < 100) snakeLength++;
                    puntuacion += 10;
                    velocidad = max(50, velocidad - 3); // Más rápido
                    generarComida();
                }
            }

            // --- RENDERIZADO ---
            if (!pantallaActualizada) {
                canvas->fillScreen(SNAKE_BG);
                
                // Anillo y Marco
                canvas->drawCircle(120, 120, 118, 0x10A2);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, SNAKE_GREEN); 
                canvas->drawRect(19, 29, 202, 182, 0x3186); // Zona segura visible

                // Comida
                canvas->fillRect(comidaX, comidaY, 10, 10, SNAKE_RED);

                // Serpiente
                for (int i = 0; i < snakeLength; i++) {
                    uint16_t color = (i == 0) ? WHITE : SNAKE_GREEN;
                    canvas->fillRect(snakeX[i], snakeY[i], 10, 10, color);
                    canvas->drawRect(snakeX[i], snakeY[i], 10, 10, SNAKE_BG); // Sombra interna
                }

                // HUD Centrado
                char hudStr[32];
                sprintf(hudStr, "SCORE: %d", puntuacion);
                Launcher::dibujarTextoCentrado(canvas, hudStr, 15, 1, WHITE);

                canvas->flush();
                pantallaActualizada = true;
            }
        }
        else if (estado == GAMEOVER) {
            // Lógica de Revancha o Salida
            if (clicSimple && millis() - ultimoBoton > 300) {
                iniciarJuego();
            }
            if (izq && millis() - ultimoBoton > 300) {
                regresarArcade = true;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(SNAKE_BG);
                canvas->drawCircle(120, 120, 118, SNAKE_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 

                Launcher::dibujarTextoCentrado(canvas, "GAME OVER", 60, 2, SNAKE_RED);
                char scStr[32]; sprintf(scStr, "SCORE FINAL: %d", puntuacion);
                Launcher::dibujarTextoCentrado(canvas, scStr, 100, 1, WHITE);

                canvas->fillRoundRect(25, 140, 190, 25, 3, SNAKE_RED);
                Launcher::dibujarTextoCentrado(canvas, "CLIC = REINTENTAR", 149, 1, WHITE);
                
                Launcher::dibujarTextoCentrado(canvas, "< IZQ = Menu Arcade", 185, 1, SNAKE_GREEN);
                
                canvas->flush();
                pantallaActualizada = true;
            }
        }

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif