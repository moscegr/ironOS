#ifndef APPNAVES_H
#define APPNAVES_H
#include "App.h"
#include "ui/Launcher.h"

#define ARCADE_CYAN    0x07FF
#define ARCADE_RED     0xF800
#define ARCADE_MAGENTA 0xF81F
#define ARCADE_YELLOW  0xFFE0

#define MAX_BALAS 8
#define MAX_ENEMIGOS 10
#define MAX_ESTRELLAS 25

struct Bala { float x, y; bool activa; };
struct Enemigo { float x, y; float vel; bool activo; };
struct Estrella { float x, y; float vel; uint16_t color; };

class AppNaves : public App {
private:
    enum EstadoJuego { JUGANDO, GAMEOVER };
    EstadoJuego estado = JUGANDO;

    float naveX = 120;
    const float naveY = 200;
    int puntuacion = 0;
    int nivel = 1;
    unsigned long ultimoSpawn = 0;
    unsigned long spawnRate = 1000;
    unsigned long ultimoDisparo = 0; 

    Bala balas[MAX_BALAS];
    Enemigo enemigos[MAX_ENEMIGOS];
    Estrella estrellas[MAX_ESTRELLAS];

    bool btnAnterior = false;
    bool pantallaActualizada = false;
    
    // Variables Neón y Rebote
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    unsigned long ultimoMovimiento = 0; 

    void iniciarJuego() {
        naveX = 120;
        puntuacion = 0;
        nivel = 1;
        spawnRate = 1200;
        estado = JUGANDO;
        regresarArcade = false;
        pantallaActualizada = false;
        ultimoDisparo = millis();
        ultimoMovimiento = millis();
        
        for(int i=0; i<MAX_BALAS; i++) balas[i].activa = false;
        for(int i=0; i<MAX_ENEMIGOS; i++) enemigos[i].activo = false;
        for(int i=0; i<MAX_ESTRELLAS; i++) {
            estrellas[i].x = random(0, 240);
            estrellas[i].y = random(0, 240);
            estrellas[i].vel = random(1, 4) / 2.0;
            estrellas[i].color = (random(0,2) == 0) ? ARCADE_CYAN : WHITE;
        }
    }

public:
    // 🚩 BANDERA DE SALIDA
    bool regresarArcade = false; 

    const char* obtenerNombre() override { return "GALAXY WAR"; }
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
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);
        bool izq = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // Anillo Neón Rotatorio (Se actualiza en todos los estados)
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        if (estado == JUGANDO) {
            // Fondo Parallax
            for(int i=0; i<MAX_ESTRELLAS; i++) {
                estrellas[i].y += estrellas[i].vel;
                if(estrellas[i].y > 240) {
                    estrellas[i].y = 0;
                    estrellas[i].x = random(0, 240);
                }
            }

            // Movimiento Nave
            float velNave = 0;
            if (valHoriz > 2400) velNave = map(valHoriz, 2400, 4095, 0, 6);
            else if (valHoriz < 1600) velNave = map(valHoriz, 1600, 0, 0, -6);
            if (joyInvHorizontal) velNave = -velNave;
            
            naveX += velNave;
            if (naveX < 15) naveX = 15;
            if (naveX > 225) naveX = 225;

            // DISPARO AUTOMÁTICO
            int cadencia = max(150, 400 - (nivel * 20)); // Dispara más rápido al subir de nivel
            if (millis() - ultimoDisparo > cadencia) {
                ultimoDisparo = millis();
                for(int i=0; i<MAX_BALAS; i++) {
                    if(!balas[i].activa) {
                        balas[i].x = naveX; balas[i].y = naveY - 12; balas[i].activa = true; break; 
                    }
                }
            }

            // Mover Balas
            for(int i=0; i<MAX_BALAS; i++) {
                if(balas[i].activa) { balas[i].y -= 6; if(balas[i].y < 0) balas[i].activa = false; }
            }

            // Spawn y Movimiento Enemigos
            if (millis() - ultimoSpawn > spawnRate) {
                ultimoSpawn = millis();
                for(int i=0; i<MAX_ENEMIGOS; i++) {
                    if(!enemigos[i].activo) {
                        enemigos[i].x = random(20, 220); enemigos[i].y = -10;
                        enemigos[i].vel = random(15, 35) / 10.0 + (nivel * 0.3);
                        enemigos[i].activo = true; break;
                    }
                }
            }

            for(int i=0; i<MAX_ENEMIGOS; i++) {
                if(enemigos[i].activo) {
                    enemigos[i].y += enemigos[i].vel;
                    if(enemigos[i].y > 240) enemigos[i].activo = false; 

                    // Colisión Bala vs Enemigo
                    for(int j=0; j<MAX_BALAS; j++) {
                        if(balas[j].activa && abs(balas[j].x - enemigos[i].x) < 12 && abs(balas[j].y - enemigos[i].y) < 12) {
                            enemigos[i].activo = false; balas[j].activa = false; puntuacion += 10;
                            if (puntuacion % 100 == 0) { nivel++; spawnRate = max(300, (int)(spawnRate * 0.85)); }
                        }
                    }

                    // Colisión Nave vs Enemigo (GAME OVER)
                    if(abs(naveX - enemigos[i].x) < 14 && abs(naveY - enemigos[i].y) < 14) {
                        estado = GAMEOVER;
                        pantallaActualizada = false;
                        ultimoMovimiento = millis(); // Anti-rebote
                    }
                }
            }

            // --- RENDERIZADO ---
            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                // Anillo Neón Exterior
                canvas->drawCircle(120, 120, 118, 0x10A2); // Gris oscuro de fondo
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, ARCADE_CYAN); 

                for(int i=0; i<MAX_ESTRELLAS; i++) canvas->drawPixel(estrellas[i].x, estrellas[i].y, estrellas[i].color);
                
                canvas->fillTriangle(naveX, naveY-12, naveX-12, naveY+8, naveX+12, naveY+8, ARCADE_CYAN); 
                canvas->fillTriangle(naveX, naveY+8, naveX-6, naveY+16, naveX+6, naveY+16, ARCADE_RED); 
                
                for(int i=0; i<MAX_BALAS; i++) if(balas[i].activa) canvas->fillRect(balas[i].x-1, balas[i].y-6, 3, 10, ARCADE_YELLOW);
                
                for(int i=0; i<MAX_ENEMIGOS; i++) {
                    if(enemigos[i].activo) {
                        canvas->fillRoundRect(enemigos[i].x-8, enemigos[i].y-8, 16, 16, 4, ARCADE_MAGENTA);
                        canvas->fillRect(enemigos[i].x-3, enemigos[i].y-3, 6, 6, WHITE); 
                    }
                }

                // HUD Superior Centrado
                char hudStr[32];
                sprintf(hudStr, "SCORE: %d | LVL: %d", puntuacion, nivel);
                Launcher::dibujarTextoCentrado(canvas, hudStr, 22, 1, WHITE);
                
                canvas->flush();
                pantallaActualizada = true;
            }
        }
        else if (estado == GAMEOVER) {
            // Lógica de Revancha o Salida
            if (clicSimple && millis() - ultimoMovimiento > 300) {
                iniciarJuego(); // Reinicia la partida sin salir
            }
            if (izq && millis() - ultimoMovimiento > 300) {
                regresarArcade = true; // Sube bandera para volver al menú
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, ARCADE_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 

                Launcher::dibujarTextoCentrado(canvas, "NAVE DESTRUIDA", 55, 2, ARCADE_RED);
                
                char scStr[32]; sprintf(scStr, "SCORE FINAL: %d", puntuacion);
                Launcher::dibujarTextoCentrado(canvas, scStr, 95, 1, WHITE);
                
                char lvlStr[32]; sprintf(lvlStr, "NIVEL: %d", nivel);
                Launcher::dibujarTextoCentrado(canvas, lvlStr, 115, 1, ARCADE_CYAN);

                // Botones Virtuales
                canvas->fillRoundRect(25, 145, 190, 25, 3, ARCADE_RED);
                Launcher::dibujarTextoCentrado(canvas, "CLIC = REINTENTAR", 154, 1, WHITE);
                
                Launcher::dibujarTextoCentrado(canvas, "< IZQ = Menu Arcade", 190, 1, ARCADE_YELLOW);
                
                canvas->flush();
                pantallaActualizada = true;
            }
        }

        btnAnterior = botonAhora;
        delay(15); 
    }
};
#endif