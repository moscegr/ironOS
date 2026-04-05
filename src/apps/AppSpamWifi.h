#ifndef APPSPAMWIFI_H
#define APPSPAMWIFI_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" // Incluimos la paleta de colores
#include <WiFi.h>
#include "esp_wifi.h"

// El LED RGB interno en las placas ESP32-S3
#define LED_PIN 48

class AppSpamWifi : public App {
private:
    String redesFalsas[20];
    uint8_t macsFalsas[20][6];
    
    // Plantilla de un paquete Beacon 802.11 estándar
    uint8_t beacon_packet[128] = {
        0x80, 0x00, 0x00, 0x00, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 4-9: Destino (Broadcast)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // 10-15: MAC Origen (Se sobreescribe)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // 16-21: BSSID (Se sobreescribe)
        0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x64, 0x00, 0x11, 0x04, 
        0x00 // Byte 36: Tag SSID
        // Byte 37: Longitud del SSID
        // Byte 38+: SSID...
    };

    unsigned long ultimoSpam = 0;
    unsigned long ultimoParpadeo = 0;
    bool estadoLed = false;
    bool pantallaActualizada = false;

    // Variables para la animación Neón táctica
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    // Función para generar un caracter hexadecimal aleatorio
    char randomHex() {
        int r = random(0, 16);
        return (r < 10) ? ('0' + r) : ('A' + (r - 10));
    }

    void generarRedesAleatorias() {
        const char* bases[] = {"izzi-", "INFINITUM", "WIFI_GRATIS_"};
        
        for (int i = 0; i < 20; i++) {
            // 1. Generar SSID Aleatorio
            int tipo = random(0, 3);
            String nombre = bases[tipo];
            
            // Añadir 4 caracteres aleatorios al final para simular routers reales
            for(int j=0; j<4; j++) {
                nombre += randomHex();
            }
            redesFalsas[i] = nombre;

            // 2. Generar MAC Aleatoria (BSSID)
            macsFalsas[i][0] = 0x02; // MAC administrada localmente
            for (int j = 1; j < 6; j++) {
                macsFalsas[i][j] = random(0, 256);
            }
        }
    }

    void enviarBeaconFalso(int index) {
        String ssid = redesFalsas[index];
        int ssid_len = ssid.length();
        
        // Copiar la MAC aleatoria al paquete
        for(int i=0; i<6; i++) {
            beacon_packet[10 + i] = macsFalsas[index][i];
            beacon_packet[16 + i] = macsFalsas[index][i];
        }
        
        beacon_packet[37] = ssid_len; // Longitud del SSID
        for(int i = 0; i < ssid_len; i++) {
            beacon_packet[38 + i] = ssid[i];
        }
        
        // Finalizar paquete con los tags obligatorios (Velocidades soportadas, canal, etc)
        beacon_packet[38 + ssid_len] = 0x01;
        beacon_packet[39 + ssid_len] = 0x08;
        beacon_packet[40 + ssid_len] = 0x82;
        beacon_packet[41 + ssid_len] = 0x84;
        beacon_packet[42 + ssid_len] = 0x8b;
        beacon_packet[43 + ssid_len] = 0x96;
        beacon_packet[44 + ssid_len] = 0x24;
        beacon_packet[45 + ssid_len] = 0x30;
        beacon_packet[46 + ssid_len] = 0x48;
        beacon_packet[47 + ssid_len] = 0x6c;
        beacon_packet[48 + ssid_len] = 0x03;
        beacon_packet[49 + ssid_len] = 0x01;
        beacon_packet[50 + ssid_len] = 0x01; // Canal 1 por defecto

        // Inyección cruda de radiofrecuencia
        esp_wifi_80211_tx(WIFI_IF_AP, beacon_packet, 51 + ssid_len, false);
    }

public:
    const char* obtenerNombre() override { return "WIFI SPAM"; }
    const uint8_t* obtenerEmoji() override { return emoji_spammer; }
    
    void setup() override {}

    void alEntrar() override {
        WiFi.mode(WIFI_AP_STA); // Necesario para inyectar paquetes
        generarRedesAleatorias(); // Pre-calculamos las 20 redes
        
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        pantallaActualizada = false;
    }
    
    void alSalir() override {
        WiFi.mode(WIFI_STA); // Regresar a modo normal
        neopixelWrite(LED_PIN, 0, 0, 0); // Apagar LED
    }

    void loop(Arduino_Canvas* canvas) override {
        // --- 1. Lógica del LED Rojo Parpadeante ---
        if (millis() - ultimoParpadeo > 150) { // Parpadeo rápido (peligro)
            ultimoParpadeo = millis();
            estadoLed = !estadoLed;
            if (estadoLed) {
                neopixelWrite(LED_PIN, 255, 0, 0); // ROJO puro
            } else {
                neopixelWrite(LED_PIN, 0, 0, 0);   // Apagado
            }
        }

        // --- ANIMACIÓN NEÓN GLOBAL ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; // Fuerza el redibujo continuo
        }

        // --- 2. Inyección de Paquetes ---
        // Se ejecuta muy rápido (cada 50ms) pero no bloquea el procesador
        if (millis() - ultimoSpam > 50) {
            ultimoSpam = millis();
            // Disparamos las 20 redes de golpe
            for(int i = 0; i < 20; i++) {
                enviarBeaconFalso(i);
            }
        }

        // --- 3. Interfaz Gráfica ---
        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);
            
            // Efecto Neón Rojo de Alerta/Ataque
            canvas->drawCircle(120, 120, 118, IRON_RED);
            canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
            canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_RED); 
            canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 
            
            Launcher::dibujarTextoCentrado(canvas, "INYECCION WIFI ACTIVA", 40, 1, IRON_RED);
            canvas->drawLine(20, 50, 220, 50, IRON_DARK);
            
            char txtCount[32];
            sprintf(txtCount, "Generando %d SSID Falsos", 20);
            Launcher::dibujarTextoCentrado(canvas, txtCount, 75, 1, WHITE);
            
            // Mostramos una pequeña muestra de lo que se está inyectando (3 redes)
            for(int i = 0; i < 5; i++) {
                canvas->setCursor(40, 105 + (i * 15));
                canvas->setTextSize(1);
                canvas->setTextColor(IRON_CYAN);
                canvas->print("> ");
                canvas->print(redesFalsas[i]);
            }

            Launcher::dibujarTextoCentrado(canvas, "...y 15 mas", 180, 1, IRON_DARK);

            // Instrucción de Salida
            canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
            canvas->setCursor(120 - 25, 212);
            canvas->setTextColor(IRON_RED);
            canvas->print("DOBLE CLICK");

            canvas->flush();
            pantallaActualizada = true;
        }

        // Mini pausa para asegurar que el Doble Click se lea perfectamente
        delay(10);
    }
};
#endif