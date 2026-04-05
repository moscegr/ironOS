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
// Inyectamos una función fantasma. Gracias a la bandera -Wl,-zmuldefs,
// el procesador ejecutará esto en lugar de la función original en libnet80211.a
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

unsigned long tiempoPresionado = 0;
unsigned long ultimoTiempoClick = 0;
bool botonEstabaPresionado = false;
enum EventosBoton { EVENTO_NADA, EVENTO_CLICK, EVENTO_DOBLE_CLICK };

EventosBoton leerEventoBoton() {
    bool botonPresionadoAhora = (digitalRead(JOY_SW) == LOW);
    EventosBoton evento = EVENTO_NADA;

    if (botonPresionadoAhora && !botonEstabaPresionado) {
        botonEstabaPresionado = true;
        tiempoPresionado = millis();
    } 
    else if (!botonPresionadoAhora && botonEstabaPresionado) {
        botonEstabaPresionado = false;
        unsigned long duracion = millis() - tiempoPresionado;
        if (duracion > 25) { 
            if (millis() - ultimoTiempoClick < TIEMPO_DOBLE_CLICK) {
                evento = EVENTO_DOBLE_CLICK;
                ultimoTiempoClick = 0; 
            } else {
                evento = EVENTO_CLICK;
                ultimoTiempoClick = millis(); 
            }
        }
    }
    return evento;
}

void setup() {
    Display::init(); // Inicializar pantalla primero para el splash

    // --- SPLASH SCREEN: EMBLEMA DE "THE FLASH" (Renderizado Geométrico) ---
    Display::canvas->fillScreen(BLACK);

    // 1. Círculo Exterior (Rojo Flash puro)
    Display::canvas->fillCircle(120, 95, 38, 0xF800); 
    
    // 2. Círculo Interior (Blanco puro)
    Display::canvas->fillCircle(120, 95, 28, 0xFFFF); 

    // 3. El Rayo (Amarillo Eléctrico)
    // Calculado matemáticamente con triángulos para sobresalir del círculo (Forma de Z)
    
    // Mitad Superior del rayo
    Display::canvas->fillTriangle(135, 45, 95, 100, 128, 100, 0xFFE0);
    // Mitad Inferior del rayo (Se superpone perfectamente en el centro)
    Display::canvas->fillTriangle(112, 90, 145, 90, 105, 145, 0xFFE0);

    // Texto de la plataforma
    Display::canvas->setTextSize(3);
    Display::canvas->setTextColor(0xFFE0); // Letras Amarillas para combinar
    
    // Centrado perfecto de la palabra "IRONOS"
    int16_t x1, y1; uint16_t w, h;
    Display::canvas->getTextBounds("IRONOS", 0, 0, &x1, &y1, &w, &h);
    Display::canvas->setCursor((240 - w) / 2, 160);
    Display::canvas->print("IRONOS");

    Display::refresh();
    delay(2000); // 2 Segundos de gloria

    Serial.begin(115200);
    
    preferencias.begin("ironos", false);
    joyEjeY_esVertical = preferencias.getBool("ejeYV", true); 
    joyInvVertical = preferencias.getBool("invV", false); 
    joyInvHorizontal = preferencias.getBool("invH", false); 
    
    pinMode(JOY_SW, INPUT_PULLUP);
    
    // Configuración inicial de las apps
    for(int i = 0; i < totalApps; i++) {
        catalogoApps[i]->setup();
    }
}

void loop() {
    EventosBoton evento = leerEventoBoton();
    
    // --- ANIMACIÓN GLOBAL DEL MENÚ PRINCIPAL ---
    if (millis() - ultimoFrameNeonMain > 30) {
        anguloNeonMain = (anguloNeonMain + 2) % 360; // Giro lento y elegante
        ultimoFrameNeonMain = millis();
    }
    
    if (appActiva == nullptr) {
        
        // --- MOTOR UNIFICADO DE NAVEGACIÓN (Horizontal: Izquierda/Derecha) ---
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);
        bool izquierda = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool derecha = joyInvHorizontal ? (valHoriz < 800) : (valHoriz > 3200);
        
        if (millis() - ultimoMovimientoMain > 200) { 
            if (izquierda) { indiceActual--; ultimoMovimientoMain = millis(); }
            if (derecha)   { indiceActual++; ultimoMovimientoMain = millis(); }
            
            if (indiceActual >= totalApps) indiceActual = 0;
            if (indiceActual < 0) indiceActual = totalApps - 1;
        }

        // Dibuja el HUD Táctico y el ícono
        Launcher::dibujar(Display::canvas, catalogoApps, totalApps, indiceActual);
        
        // --- MOTOR DE DOBLE ANILLO CYBERPUNK ---
        int anguloInverso = (360 - (anguloNeonMain * 2)) % 360; 

        // 1. Anillo Exterior (Gira a la Derecha | Magenta)
        Display::canvas->drawCircle(120, 120, 118, 0x10A2); 
        Display::canvas->drawArc(120, 120, 119, 114, anguloNeonMain, (anguloNeonMain + 100) % 360, 0xF81F); 
        Display::canvas->drawArc(120, 120, 119, 114, (anguloNeonMain + 180) % 360, (anguloNeonMain + 280) % 360, 0xF81F); 

        // 2. Anillo Interior (Gira a la Izquierda | Cyan)
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
        
        if (evento == EVENTO_DOBLE_CLICK) {
            appActiva->alSalir(); 
            appActiva = nullptr; 
            
            Display::canvas->fillScreen(BLACK);
            Display::canvas->drawCircle(120, 120, 118, IRON_RED); 
            Display::refresh();
            delay(150);
        }
    }
}