#ifndef APPBLE_H
#define APPBLE_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <math.h> 

#define LED_PIN 48

// --- VARIABLES GLOBALES PARA ESCANEO ASÍNCRONO ---
static volatile bool escaneoBLECompletado = false;
static void callbackFinEscaneo(BLEScanResults results) {
    escaneoBLECompletado = true;
}

struct DispBLE { String mac; String nombre; int rssi; };
enum EstadoBLE { ESCANEANDO, LISTA, DETALLE };

class AppBLE : public App {
private:
    BLEScan* pBLEScan;
    DispBLE dispositivos[20]; 
    int numDispositivos = 0;
    
    EstadoBLE estado = ESCANEANDO;
    int indiceSeleccionado = 0;
    
    unsigned long ultimoParpadeo = 0;
    unsigned long ultimoMovimiento = 0; 
    
    // Variables para la animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    
    bool estadoLed = false;
    bool pantallaActualizada = false;
    bool btnAnterior = false; 

    // Temporizador visual de escaneo
    unsigned long inicioEscaneo = 0;
    int segundosRestantes = 4;

    float calcularDistancia(int rssi) {
        return pow(10.0, ((float)-69 - (float)rssi) / 20.0);
    }

public:
    const char* obtenerNombre() override { return "BLUE ESCANER"; }
    const uint8_t* obtenerEmoji() override { return emoji_ble; }
    
    void setup() override {
        BLEDevice::init("IronOS");
        pBLEScan = BLEDevice::getScan();
        pBLEScan->setActiveScan(true);
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99); 
    }

    void alEntrar() override {
        estado = ESCANEANDO;
        numDispositivos = 0;
        indiceSeleccionado = 0;
        ultimoMovimiento = millis(); 
        ultimoFrameNeon = millis();
        anguloNeon = 0;
        pantallaActualizada = false;
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        
        // --- INICIO DE ESCANEO ASÍNCRONO (4 Segundos) ---
        escaneoBLECompletado = false;
        inicioEscaneo = millis();
        segundosRestantes = 4;
        
        // Al pasar la función callback, el escaneo se va a segundo plano y NO congela el loop visual
        pBLEScan->start(4, callbackFinEscaneo, false); 
    }
    
    void alSalir() override {
        pBLEScan->stop();
        pBLEScan->clearResults();
        neopixelWrite(LED_PIN, 0, 0, 0); 
    }

    void loop(Arduino_Canvas* canvas) override {
        
        // --- MOTOR UNIFICADO DE LECTURA ---
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
            pantallaActualizada = false; 
        }

        // ==========================================
        // ESTADO 1: ESCANEANDO (AHORA FLUIDO)
        // ==========================================
        if (estado == ESCANEANDO) {
            
            if (millis() - ultimoParpadeo > 200) { 
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 0, 0, 255); 
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            // Cálculo del tiempo visual
            int transcurrido = (millis() - inicioEscaneo) / 1000;
            int nuevoRestante = 4 - transcurrido;
            if (nuevoRestante < 0) nuevoRestante = 0;
            
            if (nuevoRestante != segundosRestantes) {
                segundosRestantes = nuevoRestante;
                pantallaActualizada = false;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 

                Launcher::dibujarTextoCentrado(canvas, "ESCANEO DE BLUES !", 100, 2, IRON_CYAN);
                
                char tiempoStr[32];
                sprintf(tiempoStr, "Termina en: %d s", segundosRestantes);
                Launcher::dibujarTextoCentrado(canvas, tiempoStr, 130, 2, WHITE);

                canvas->flush();
                pantallaActualizada = true;
            }

            // --- REVISAMOS SI EL PROCESO EN SEGUNDO PLANO TERMINÓ ---
            if (escaneoBLECompletado) {
                BLEScanResults foundDevices = pBLEScan->getResults();
                
                for (int i = 0; i < foundDevices.getCount(); i++) {
                    BLEAdvertisedDevice d = foundDevices.getDevice(i);
                    String dMac = d.getAddress().toString().c_str();
                    dMac.toUpperCase();
                    
                    bool existe = false;
                    for (int j = 0; j < numDispositivos; j++) {
                        if (dispositivos[j].mac == dMac) { existe = true; break; }
                    }
                    
                    if (!existe && numDispositivos < 20) {
                        dispositivos[numDispositivos].mac = dMac;
                        dispositivos[numDispositivos].rssi = d.getRSSI();
                        String nom = d.getName().c_str();
                        dispositivos[numDispositivos].nombre = (nom.length() > 0) ? nom : "DESCONOCIDO";
                        numDispositivos++;
                    }
                }
                pBLEScan->clearResults();
                estado = LISTA;
                neopixelWrite(LED_PIN, 0, 0, 0); 
                pantallaActualizada = false;
            }
        }
        
        // ==========================================
        // ESTADO 2: LISTA
        // ==========================================
        else if (estado == LISTA) {
            
            if (millis() - ultimoMovimiento > 250) {
                if (arriba) { indiceSeleccionado--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo) { indiceSeleccionado++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                
                if (indiceSeleccionado < 0) indiceSeleccionado = (numDispositivos > 0) ? numDispositivos - 1 : 0;
                if (indiceSeleccionado >= numDispositivos) indiceSeleccionado = 0;
                
                if (der && numDispositivos > 0) { 
                    estado = DETALLE;
                    neopixelWrite(LED_PIN, 0, 0, 255); 
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }
            
            if (clicSimple && numDispositivos > 0) {
                estado = DETALLE;
                neopixelWrite(LED_PIN, 0, 0, 255); 
                pantallaActualizada = false;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 
                
                char titulo[32];
                sprintf(titulo, "REDES BLE (%d)", numDispositivos);
                Launcher::dibujarTextoCentrado(canvas, titulo, 30, 1, IRON_CYAN);
                canvas->drawLine(20, 40, 220, 40, IRON_DARK);

                if (numDispositivos == 0) {
                    Launcher::dibujarTextoCentrado(canvas, "AIRE LIMPIO", 110, 2, IRON_BLUE);
                } else {
                    int inicio = indiceSeleccionado - 1;
                    if (inicio < 0) inicio = 0;
                    if (numDispositivos > 3 && inicio > numDispositivos - 3) inicio = numDispositivos - 3;
                    
                    for (int i = 0; i < 3 && (inicio + i) < numDispositivos; i++) {
                        int idx = inicio + i;
                        int yPos = 65 + (i * 35); 
                        
                        if (idx == indiceSeleccionado) {
                            canvas->fillRoundRect(15, yPos - 10, 210, 25, 3, IRON_RED);
                            canvas->fillTriangle(20, yPos-2, 20, yPos+6, 25, yPos+2, IRON_GREEN); 
                            canvas->setTextColor(WHITE);
                        } else {
                            canvas->setTextColor(IRON_BLUE);
                        }
                        
                        canvas->setTextSize(1);
                        canvas->setCursor(30, yPos);
                        
                        String textoItem = String(idx + 1) + ". " + dispositivos[idx].nombre;
                        canvas->print(textoItem.substring(0, 18));
                    }
                    Launcher::dibujarTextoCentrado(canvas, "Clic/Der = Ver Datos", 175, 1, IRON_DARK);
                }
                
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("DOBLE CLICK");
                
                canvas->flush();
                pantallaActualizada = true;
            }
        }
        
        // ==========================================
        // ESTADO 3: DETALLE
        // ==========================================
        else if (estado == DETALLE) {
            
            if (millis() - ultimoMovimiento > 250) {
                if (izq) { 
                    estado = LISTA;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }

            if (clicSimple) {
                estado = LISTA;
                neopixelWrite(LED_PIN, 0, 0, 0); 
                pantallaActualizada = false;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 
                
                Launcher::dibujarTextoCentrado(canvas, "INFO OBJETIVO", 35, 1, IRON_CYAN);
                canvas->drawLine(30, 45, 210, 45, IRON_DARK);

                Launcher::dibujarTextoCentrado(canvas, dispositivos[indiceSeleccionado].nombre.substring(0, 18).c_str(), 75, 2, WHITE);
                Launcher::dibujarTextoCentrado(canvas, dispositivos[indiceSeleccionado].mac.c_str(), 105, 1, IRON_BLUE);
                
                char rssiStr[32];
                sprintf(rssiStr, "Senal: %d dBm", dispositivos[indiceSeleccionado].rssi);
                Launcher::dibujarTextoCentrado(canvas, rssiStr, 125, 1, IRON_CYAN);

                float distancia = calcularDistancia(dispositivos[indiceSeleccionado].rssi);
                char distStr[32];
                if (distancia < 1.0) sprintf(distStr, "Aprox: Menos de 1 metro");
                else sprintf(distStr, "Aprox: %.1f metros", distancia);
                
                Launcher::dibujarTextoCentrado(canvas, distStr, 145, 1, WHITE);
                Launcher::dibujarTextoCentrado(canvas, "< Volver = Clic/Izq", 175, 1, IRON_DARK);

                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("DOBLE CLICK");

                canvas->flush();
                pantallaActualizada = true;
            }
        }
        
        btnAnterior = botonAhora; 
        delay(10); 
    }
};
#endif