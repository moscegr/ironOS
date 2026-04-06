#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "core/Display.h"
#include "ui/Launcher.h"

// Inclusiones de las Apps 
#include "apps/AppSpamBLE.h"
#include "apps/AppSpamWifi.h"
#include "apps/AppWiFi.h"
#include "apps/AppInfo.h"
#include "apps/AppSettings.h"
#include "apps/AppStats.h"
#include "apps/AppSD.h"
#include "apps/AppBLE.h"
#include "apps/AppEvilTwin.h"
#include "apps/AppSniffer.h"
#include "apps/AppDucky.h"
#include "apps/AppArcade.h"

// =========================================================================
// 🛡️ MOTOR DE EVASIÓN DE FIREWALL DE ESPRESSIF (BYPASS 0xC0 DEAUTH)
// =========================================================================
extern "C" {
    int ieee80211_raw_frame_sanity_check(int32_t arg1, int32_t arg2, int32_t arg3) {
        return 0; // 0 = Paquete Legítimo. El firewall aprueba el envío.
    }
}

// Variables Globales de Calibración
bool joyEjeY_esVertical = true;
bool joyInvVertical = false;
bool joyInvHorizontal = false;
int rotacionPantalla = 0; 
Preferences preferencias;

// Instancias de Apps
AppWiFi       wifiApp;
AppSettings   settingsApp;
AppSD         sdApp;
AppStats      statsApp;
AppInfo       infoApp;
AppBLE        bleApp;     
AppSpamBLE    spamAppBLE; 
AppSpamWifi   spamAppWifi;     
AppEvilTwin   evilTwinApp; 
AppSniffer    snifferApp;
AppDucky      duckyApp;
AppArcade     arcadeApp;

// Catálogo maestro
App* catalogoApps[] = {&arcadeApp, &bleApp, &spamAppWifi,&snifferApp, &evilTwinApp, &wifiApp,&spamAppBLE, &duckyApp, &sdApp, &settingsApp, &statsApp, &infoApp }; 
int totalApps = 12;
int indiceActual = 0;
App* appActiva = nullptr;

// --- VARIABLES DE MOTOR UNIFICADO Y ANIMACIÓN ---
unsigned long ultimoMovimientoMain = 0;
int anguloNeonMain = 0;
unsigned long ultimoFrameNeonMain = 0;

// --- NUEVO MOTOR DE LECTURA DE BOTÓN ANTI-REBOTE ---
unsigned long tiempoPresionado = 0;
bool botonEstabaPresionado = false;
bool longPressDetectado = false; // Bandera para evitar disparos múltiples

enum EventosBoton { EVENTO_NADA, EVENTO_CLICK, EVENTO_MANTENER_PRESIONADO };

EventosBoton leerEventoBoton() {
    bool botonPresionadoAhora = (digitalRead(JOY_SW) == LOW);
    EventosBoton evento = EVENTO_NADA;

    if (botonPresionadoAhora) {
        if (!botonEstabaPresionado) {
            // El botón acaba de ser hundido
            botonEstabaPresionado = true;
            tiempoPresionado = millis();
            longPressDetectado = false;
        } else {
            // El botón se mantiene hundido, validamos el tiempo
            if (!longPressDetectado && (millis() - tiempoPresionado > TIEMPO_PRESION_LARGA)) {
                longPressDetectado = true; // Bloquea la acción hasta que se suelte
                evento = EVENTO_MANTENER_PRESIONADO;
            }
        }
    } 
    else {
        if (botonEstabaPresionado) {
            // El botón acaba de ser soltado
            botonEstabaPresionado = false;
            unsigned long duracion = millis() - tiempoPresionado;
            
            // Si el toque duró más de 25ms (Anti-rebote real) Y no fue un long press
            if (duracion > 25 && !longPressDetectado) { 
                evento = EVENTO_CLICK;
            }
        }
    }
    return evento;
}

void setup() {
    // --- LECTURA DE PREFERENCIAS DE HARDWARE ---
    preferencias.begin("ironos", false);
    joyEjeY_esVertical = preferencias.getBool("ejeYV", true); 
    joyInvVertical = preferencias.getBool("invV", false); 
    joyInvHorizontal = preferencias.getBool("invH", false); 
    rotacionPantalla = preferencias.getInt("rotPantalla", 0); 

    Display::init(); 
    Display::canvas->setRotation(rotacionPantalla); 

    // --- SPLASH SCREEN GEOMÉTRICO ---
    Display::canvas->fillScreen(BLACK);
    Display::canvas->fillCircle(120, 95, 38, 0xF800); 
    Display::canvas->fillCircle(120, 95, 28, 0xFFFF); 
    Display::canvas->fillTriangle(135, 45, 95, 100, 128, 100, 0xFFE0);
    Display::canvas->fillTriangle(112, 90, 145, 90, 105, 145, 0xFFE0);

    Display::canvas->setTextSize(3);
    Display::canvas->setTextColor(0xFFE0); 
    int16_t x1, y1; uint16_t w, h;
    Display::canvas->getTextBounds("IRONOS", 0, 0, &x1, &y1, &w, &h);
    Display::canvas->setCursor((240 - w) / 2, 160);
    Display::canvas->print("IRONOS");

    Display::refresh();
    delay(2000); 

    Serial.begin(115200);
    pinMode(JOY_SW, INPUT_PULLUP);
    
    for(int i = 0; i < totalApps; i++) {
        catalogoApps[i]->setup();
    }
}

void loop() {
    EventosBoton evento = leerEventoBoton();
    
    if (Display::canvas->getRotation() != rotacionPantalla) {
        Display::canvas->setRotation(rotacionPantalla);
    }

    if (millis() - ultimoFrameNeonMain > 30) {
        anguloNeonMain = (anguloNeonMain + 2) % 360; 
        ultimoFrameNeonMain = millis();
    }
    
    if (appActiva == nullptr) {
        // --- NAVEGACIÓN ---
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);
        bool izquierda = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool derecha = joyInvHorizontal ? (valHoriz < 800) : (valHoriz > 3200);
        
        if (millis() - ultimoMovimientoMain > 200) { 
            if (izquierda) { indiceActual--; ultimoMovimientoMain = millis(); }
            if (derecha)   { indiceActual++; ultimoMovimientoMain = millis(); }
            if (indiceActual >= totalApps) indiceActual = 0;
            if (indiceActual < 0) indiceActual = totalApps - 1;
        }

        Launcher::dibujar(Display::canvas, catalogoApps, totalApps, indiceActual);
        
        int anguloInverso = (360 - (anguloNeonMain * 2)) % 360; 
        Display::canvas->drawCircle(120, 120, 118, 0x10A2); 
        Display::canvas->drawArc(120, 120, 119, 114, anguloNeonMain, (anguloNeonMain + 100) % 360, 0xF81F); 
        Display::canvas->drawArc(120, 120, 119, 114, (anguloNeonMain + 180) % 360, (anguloNeonMain + 280) % 360, 0xF81F); 
        Display::canvas->drawCircle(120, 120, 108, 0x10A2); 
        Display::canvas->drawArc(120, 120, 109, 106, anguloInverso, (anguloInverso + 80) % 360, 0x07FF); 
        Display::canvas->drawArc(120, 120, 109, 106, (anguloInverso + 180) % 360, (anguloInverso + 260) % 360, 0x07FF); 

        Display::refresh();

        if (evento == EVENTO_CLICK) {
            appActiva = catalogoApps[indiceActual];
            appActiva->alEntrar(); 
        }
    } 
    else {
        appActiva->loop(Display::canvas);
        
        // --- AQUÍ ESTÁ LA MAGIA DEL CIERRE ---
        if (evento == EVENTO_MANTENER_PRESIONADO) {
            appActiva->alSalir(); 
            appActiva = nullptr; 
            
            Display::canvas->fillScreen(BLACK);
            Display::canvas->drawCircle(120, 120, 118, IRON_RED); 
            Display::refresh();
            delay(150); // Pequeño efecto visual de cierre
        }
    }
}