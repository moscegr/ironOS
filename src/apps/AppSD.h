#ifndef APPSD_H
#define APPSD_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <SPI.h>
#include <SD.h>

#define LED_PIN 48

class AppSD : public App {
private:
    bool sdOk = false;
    int numArchivos = 0;
    String nombresArchivos[15];
    int indiceSeleccionado = 0;
    
    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    
    // Variables para la animación Neón táctica
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    
    SPIClass* spiSD = nullptr;

    void cargarArchivos() {
        numArchivos = 0;
        File root = SD.open("/");
        if (!root) return;
        
        File file = root.openNextFile();
        while (file && numArchivos < 15) {
            nombresArchivos[numArchivos] = file.name();
            numArchivos++;
            file = root.openNextFile();
        }
        root.close();
    }

public:
    const char* obtenerNombre() override { return "ARCHIVOS SD"; }
    const uint8_t* obtenerEmoji() override { return emoji_sd; }
    
    void setup() override {
        // Asignamos memoria dinámica para el bus SPI secundario
        spiSD = new SPIClass(FSPI);
    }

    void alEntrar() override {
        pantallaActualizada = false;
        numArchivos = 0;
        indiceSeleccionado = 0;
        ultimoMovimiento = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        
        anguloNeon = 0;
        ultimoFrameNeon = millis();

        // 1. Pre-estabilizar el pin CS para evitar el error 0x107 de Reddit
        pinMode(SD_CS, OUTPUT);
        digitalWrite(SD_CS, HIGH);

        // 2. Inicializar bus SPI pasando -1 en el pin CS para delegar control a SD.h
        spiSD->begin(SD_SCK, SD_MISO, SD_MOSI, -1);
        
        // 3. Levantar la tarjeta a 4MHz (Velocidad segura antibrownout)
        sdOk = SD.begin(SD_CS, *spiSD, 4000000);
        
        if (sdOk) {
            cargarArchivos();
            neopixelWrite(LED_PIN, 0, 255, 0); // Led Verde: Éxito
        } else {
            neopixelWrite(LED_PIN, 255, 0, 0); // Led Rojo: Falla
        }
    }
    
    void alSalir() override {
        SD.end(); 
        neopixelWrite(LED_PIN, 0, 0, 0); 
    }

    void loop(Arduino_Canvas* canvas) override {
        
        // --- MOTOR UNIFICADO DE LECTURA ---
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // --- ANIMACIÓN NEÓN GLOBAL ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; // Fuerza el redibujo ininterrumpido
        }

        // --- NAVEGACIÓN DEL EXPLORADOR ---
        if (sdOk && millis() - ultimoMovimiento > 250) {
            if (arriba) { indiceSeleccionado--; pantallaActualizada = false; ultimoMovimiento = millis(); }
            if (abajo) { indiceSeleccionado++; pantallaActualizada = false; ultimoMovimiento = millis(); }
            
            if (indiceSeleccionado < 0) indiceSeleccionado = (numArchivos > 0) ? numArchivos - 1 : 0;
            if (indiceSeleccionado >= numArchivos) indiceSeleccionado = 0;
            
            if (clicSimple && numArchivos > 0) {
                neopixelWrite(LED_PIN, 0, 0, 255); 
                delay(100);
                neopixelWrite(LED_PIN, 0, 255, 0);
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }
        }

        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);
            
            if (!sdOk) {
                // Interfaz de Error (Animación en ROJO)
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_RED); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE);
                
                Launcher::dibujarTextoCentrado(canvas, "ERROR DE LECTURA", 85, 1, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "Voltaje Inestable", 115, 1, WHITE);
                Launcher::dibujarTextoCentrado(canvas, "Usa modulo externo SPI", 140, 1, IRON_BLUE);
            } else {
                // Interfaz de Éxito (Animación en VERDE TÁCTICO)
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_GREEN); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE);

                Launcher::dibujarTextoCentrado(canvas, "ALMACENAMIENTO SD", 35, 1, IRON_GREEN);
                canvas->drawLine(20, 45, 220, 45, IRON_DARK);

                if (numArchivos == 0) {
                    Launcher::dibujarTextoCentrado(canvas, "CARPETA VACIA", 110, 2, IRON_BLUE);
                } else {
                    int inicio = indiceSeleccionado - 1;
                    if (inicio < 0) inicio = 0;
                    if (numArchivos > 3 && inicio > numArchivos - 3) inicio = numArchivos - 3;

                    for (int i = 0; i < 3 && (inicio + i) < numArchivos; i++) {
                        int idx = inicio + i;
                        int yPos = 65 + (i * 35);

                        if (idx == indiceSeleccionado) {
                            canvas->fillRoundRect(15, yPos - 10, 210, 25, 3, IRON_DARK);
                            canvas->fillTriangle(20, yPos-2, 20, yPos+6, 25, yPos+2, IRON_GREEN);
                            canvas->setTextColor(WHITE);
                        } else {
                            canvas->setTextColor(IRON_BLUE);
                        }

                        canvas->setTextSize(1);
                        canvas->setCursor(30, yPos);
                        
                        String textoItem = "> " + nombresArchivos[idx];
                        canvas->print(textoItem.substring(0, 18));
                    }
                }
            }

            canvas->drawBitmap(120 - 45, 200, emoji_back, 16, 16, sdOk ? IRON_GREEN : IRON_RED);
            canvas->setCursor(120 - 25, 200);
            canvas->setTextColor(IRON_RED);
            canvas->print("DOBLE CLICK");

            canvas->flush();
            pantallaActualizada = true;
        }

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif