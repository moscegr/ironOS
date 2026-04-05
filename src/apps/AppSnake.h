#ifndef APPSNAKE_H
#define APPSNAKE_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 

class AppSnake : public App {
private:
    int snakeX[100], snakeY[100];
    int longitud = 3;
    int dirX = 1, dirY = 0;
    int foodX, foodY;
    bool gameOver = false;
    unsigned long ultimoMovimiento = 0;
    int velocidad = 150; 
    
    // Control de botón
    bool btnAnterior = false;
    
    // Variables para la animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    void generarComida() {
        foodX = random(0, 14);
        foodY = random(0, 14);
    }
    
    void reiniciarJuego() {
        snakeX[0] = 7; snakeY[0] = 7;
        longitud = 3;
        dirX = 1; dirY = 0;
        gameOver = false;
        velocidad = 150;
        generarComida();
    }

public:
    const char* obtenerNombre() override { return "SNAKE GAME"; }
    const uint8_t* obtenerEmoji() override { return emoji_snake; }
    
    void setup() override { randomSeed(esp_random()); }

    void alEntrar() override {
        reiniciarJuego();
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
    }
    
    void alSalir() override {}

    void loop(Arduino_Canvas* canvas) override {
        
        // --- MOTOR UNIFICADO DE LECTURA DE JUEGO ---
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);

        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool izq = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool der = joyInvHorizontal ? (valHoriz < 800) : (valHoriz > 3200);
        
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // --- ANIMACIÓN NEÓN GLOBAL ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
        }

        canvas->fillScreen(BLACK);

        // ==========================================
        // ESTADO 1: GAME OVER
        // ==========================================
        if (gameOver) {
            // Estela Roja Táctica
            canvas->drawCircle(120, 120, 118, IRON_RED);
            canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
            canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_RED); 
            canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE);

            Launcher::dibujarTextoCentrado(canvas, "GAME OVER", 95, 2, IRON_RED);
            char puntaje[16];
            sprintf(puntaje, "PUNTOS: %d", longitud * 10);
            Launcher::dibujarTextoCentrado(canvas, puntaje, 130, 1, WHITE);
            
            Launcher::dibujarTextoCentrado(canvas, "< Clic = Reintentar", 170, 1, IRON_CYAN);
            
            canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
            canvas->setCursor(120 - 25, 212);
            canvas->setTextColor(IRON_RED);
            canvas->print("DOBLE CLICK");
            
            canvas->flush();
            
            // Reiniciar partida con clic simple
            if (clicSimple) {
                reiniciarJuego();
            }
            
            btnAnterior = botonAhora;
            return;
        }

        // ==========================================
        // ESTADO 2: JUGANDO (Animación Cyan)
        // ==========================================
        canvas->drawCircle(120, 120, 118, IRON_CYAN);
        canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
        canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
        canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 

        // Lógica de Movimiento y Colisiones
        if (izq && dirX == 0) { dirX = -1; dirY = 0; } 
        if (der && dirX == 0) { dirX = 1; dirY = 0; }  
        if (arriba && dirY == 0) { dirX = 0; dirY = -1; } 
        if (abajo && dirY == 0) { dirX = 0; dirY = 1; }  

        if (millis() - ultimoMovimiento > velocidad) {
            ultimoMovimiento = millis();
            
            for (int i = longitud - 1; i > 0; i--) {
                snakeX[i] = snakeX[i - 1];
                snakeY[i] = snakeY[i - 1];
            }
            snakeX[0] += dirX;
            snakeY[0] += dirY;

            if (snakeX[0] < 0 || snakeX[0] >= 14 || snakeY[0] < 0 || snakeY[0] >= 14) gameOver = true;
            for (int i = 1; i < longitud; i++) {
                if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) gameOver = true;
            }

            if (snakeX[0] == foodX && snakeY[0] == foodY) {
                longitud++;
                if (velocidad > 60) velocidad -= 2; 
                generarComida();
            }
        }

        // Dibujo de la grilla de juego y la serpiente
        canvas->drawRect(49, 49, 142, 142, IRON_DARK);
        canvas->fillRect(50 + (foodX * 10), 50 + (foodY * 10), 10, 10, IRON_RED);

        for (int i = 0; i < longitud; i++) {
            uint16_t color = (i == 0) ? WHITE : IRON_CYAN; 
            canvas->fillRect(50 + (snakeX[i] * 10), 50 + (snakeY[i] * 10), 10, 10, color);
        }

        // Puntaje en tiempo real en la parte superior
        char pnt[16];
        sprintf(pnt, "PTS: %d", longitud * 10);
        Launcher::dibujarTextoCentrado(canvas, pnt, 30, 1, WHITE);

        canvas->flush();
        btnAnterior = botonAhora; // Guardar estado del botón
    }
};
#endif