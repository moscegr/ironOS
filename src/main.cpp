#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "core/Display.h"
#include "ui/Launcher.h"
#include "apps/AppWiFi.h"
#include "apps/AppInfo.h"
#include "apps/AppSettings.h"
#include "apps/AppStats.h"
#include "apps/AppSnake.h"
#include "apps/AppSD.h"
#include "apps/AppBLE.h"
#include "apps/AppSpammer.h"
#include "apps/AppEvilTwin.h"

// Variables Globales de Calibración
bool joyEjeY_esVertical = true;
bool joyInvVertical = false;
bool joyInvHorizontal = false;
Preferences preferencias;

AppWiFi wifiApp;
AppSettings settingsApp;
AppSD sdApp;
AppStats statsApp;
AppSnake snakeApp;
AppInfo infoApp;
AppBLE bleApp;           
AppSpammer spammerApp;   
AppEvilTwin evilTwinApp; 
App* catalogoApps[] = { &wifiApp, &bleApp, &spammerApp, &sdApp, &settingsApp, &statsApp, &snakeApp, &infoApp, &evilTwinApp }; 
int totalApps = 9;
int indiceActual = 0;
App* appActiva = nullptr;

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
    delay(2000); 
    Serial.begin(115200);
    
    preferencias.begin("ironos", false);
    joyEjeY_esVertical = preferencias.getBool("ejeYV", true); 
    joyInvVertical = preferencias.getBool("invV", false); 
    joyInvHorizontal = preferencias.getBool("invH", false); 
    
    Display::init();
    pinMode(JOY_SW, INPUT_PULLUP);
    
    for(int i = 0; i < totalApps; i++) {
        catalogoApps[i]->setup();
    }
}

void loop() {
    EventosBoton evento = leerEventoBoton();
    
    if (appActiva == nullptr) {
        
        // --- MOTOR UNIFICADO DE NAVEGACIÓN ---
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        
        if (arriba) { indiceActual--; delay(150); }
        if (abajo) { indiceActual++; delay(150); }
        
        if (indiceActual >= totalApps) indiceActual = 0;
        if (indiceActual < 0) indiceActual = totalApps - 1;

        Launcher::dibujar(Display::canvas, catalogoApps, totalApps, indiceActual);
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