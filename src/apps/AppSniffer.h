#ifndef APPSNIFFER_H
#define APPSNIFFER_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <WiFi.h>
#include "esp_wifi.h"
#include <SD_MMC.h>

#define LED_PIN 48
#define MAX_CHANNELS 14
#define PCAP_BUF_SIZE 16384 // 16 KB de Buffer Anti-Crash

// --- VARIABLES GLOBALES PARA EL CALLBACK PROMISCUO ---
static volatile long pkts_per_channel[MAX_CHANNELS + 1] = {0};
static volatile long total_pkts = 0;
static volatile int current_channel = 1;
static unsigned long last_channel_hop = 0;

static volatile bool is_pcap_active = false;
static uint8_t pcap_buf[PCAP_BUF_SIZE];
static volatile int buf_head = 0;
static volatile int buf_tail = 0;
static volatile int buf_len = 0;

// Callback de altísima velocidad (Ejecutado en la RAM interna)
void IRAM_ATTR sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    pkts_per_channel[current_channel]++;
    total_pkts++;

    // Lógica de Ring Buffer para PCAP
    if (is_pcap_active) {
        uint32_t len = pkt->rx_ctrl.sig_len;
        uint32_t timestamp = esp_timer_get_time();
        uint32_t ts_sec = timestamp / 1000000;
        uint32_t ts_usec = timestamp % 1000000;

        uint32_t header[4] = { ts_sec, ts_usec, len, len }; // PCAP Packet Header
        uint32_t total_len = 16 + len;

        // Si hay espacio en el buffer, lo guardamos rápido y volvemos al aire
        if (buf_len + total_len < PCAP_BUF_SIZE) {
            uint8_t* hdr_ptr = (uint8_t*)header;
            for(int i=0; i<16; i++) {
                pcap_buf[buf_head] = hdr_ptr[i];
                buf_head = (buf_head + 1) % PCAP_BUF_SIZE;
            }
            uint8_t* payload_ptr = (uint8_t*)pkt->payload;
            for(uint32_t i=0; i<len; i++) {
                pcap_buf[buf_head] = payload_ptr[i];
                buf_head = (buf_head + 1) % PCAP_BUF_SIZE;
            }
            buf_len += total_len;
        }
    }
}

class AppSniffer : public App {
private:
    enum EstadoSniff { MENU, GRAFICO_TRAFICO, CAPTURA_PCAP };
    EstadoSniff estado = MENU;

    int indiceMenu = 0;
    String opciones[2] = {"1. GRAFICO (RADAR)", "2. CAPTURA PCAP (SD)"};

    unsigned long ultimoMovimiento = 0;
    unsigned long ultimoParpadeo = 0;
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    bool estadoLed = false;
    
    bool snifferIniciado = false;
    bool sdOk = false;
    File pcapFile;

    void iniciarModoPromiscuo() {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_STA);
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
        esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
        snifferIniciado = true;
    }

    void detenerSniffer() {
        esp_wifi_set_promiscuous(false);
        esp_wifi_set_promiscuous_rx_cb(NULL);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        snifferIniciado = false;
        is_pcap_active = false;
        if (pcapFile) pcapFile.close();
    }

    void iniciarPCAP() {
        SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
        sdOk = SD_MMC.begin("/sdcard", true);
        if (sdOk) {
            String fileName = "/captura_" + String(millis()) + ".pcap";
            pcapFile = SD_MMC.open(fileName, FILE_WRITE);
            if (pcapFile) {
                // Escribir Cabecera Global PCAP
                uint32_t magic = 0xa1b2c3d4; pcapFile.write((uint8_t*)&magic, 4);
                uint16_t v_maj = 2; pcapFile.write((uint8_t*)&v_maj, 2);
                uint16_t v_min = 4; pcapFile.write((uint8_t*)&v_min, 2);
                int32_t zone = 0; pcapFile.write((uint8_t*)&zone, 4);
                uint32_t sig = 0; pcapFile.write((uint8_t*)&sig, 4);
                uint32_t snap = 65535; pcapFile.write((uint8_t*)&snap, 4);
                uint32_t net = 105; pcapFile.write((uint8_t*)&net, 4); // 802.11
                pcapFile.flush();
                is_pcap_active = true;
            } else {
                sdOk = false;
            }
        }
    }

    void procesarBufferSD() {
        if (is_pcap_active && pcapFile && buf_len > 0) {
            // Escribir a la SD en el loop principal, sin congelar el ESP32
            while(buf_len > 0) {
                pcapFile.write(pcap_buf[buf_tail]);
                buf_tail = (buf_tail + 1) % PCAP_BUF_SIZE;
                buf_len--;
            }
            pcapFile.flush();
        }
    }

public:
    const char* obtenerNombre() override { return "WIFI SNIFFER"; }
    const uint8_t* obtenerEmoji() override { return emoji_wifi; } 
    
    void setup() override {}

    void alEntrar() override {
        estado = MENU;
        indiceMenu = 0;
        pantallaActualizada = false;
        ultimoMovimiento = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        detenerSniffer();
    }
    
    void alSalir() override {
        detenerSniffer();
        neopixelWrite(LED_PIN, 0, 0, 0); 
    }

    void loop(Arduino_Canvas* canvas) override {
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool izq = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        // Salto de Canal Automático para "barrer" toda la banda
        if (snifferIniciado && millis() - last_channel_hop > 200) {
            last_channel_hop = millis();
            current_channel++;
            if(current_channel > MAX_CHANNELS) current_channel = 1;
            esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
        }

        // Guardado asíncrono
        procesarBufferSD();

        if (estado == MENU) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenu--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenu++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenu < 0) indiceMenu = 1;
                if (indiceMenu > 1) indiceMenu = 0;
            }

            if (clicSimple) {
                for(int i=0; i<=MAX_CHANNELS; i++) pkts_per_channel[i] = 0;
                total_pkts = 0;
                
                if (indiceMenu == 0) {
                    estado = GRAFICO_TRAFICO;
                    iniciarModoPromiscuo();
                } else {
                    estado = CAPTURA_PCAP;
                    iniciarPCAP();
                    if(sdOk) iniciarModoPromiscuo();
                }
                pantallaActualizada = false;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                
                Launcher::dibujarTextoCentrado(canvas, "WIFI SNIFFER", 40, 1, IRON_CYAN);
                canvas->drawLine(20, 50, 220, 50, IRON_DARK);

                for (int i = 0; i < 2; i++) {
                    int yPos = 90 + (i * 35);
                    if (i == indiceMenu) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_RED);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, WHITE);
                        Launcher::dibujarTextoCentrado(canvas, opciones[i].c_str(), yPos - 3, 1, WHITE);
                    } else {
                        Launcher::dibujarTextoCentrado(canvas, opciones[i].c_str(), yPos - 3, 1, IRON_BLUE);
                    }
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
        else if (estado == GRAFICO_TRAFICO || estado == CAPTURA_PCAP) {
            
            if (millis() - ultimoParpadeo > (estado == CAPTURA_PCAP ? 50 : 200)) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 0, 0, 255); // Azul en Gráfico, rápido en PCAP
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            if (clicSimple || izq) {
                detenerSniffer();
                estado = MENU;
                neopixelWrite(LED_PIN, 0, 0, 0); 
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                if(estado == CAPTURA_PCAP && !sdOk) {
                    canvas->drawCircle(120, 120, 118, IRON_RED);
                    Launcher::dibujarTextoCentrado(canvas, "ERROR SD CARD", 100, 2, IRON_RED);
                    Launcher::dibujarTextoCentrado(canvas, "< Volver", 185, 1, IRON_DARK);
                } else {
                    canvas->drawCircle(120, 120, 118, IRON_GREEN);
                    canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 

                    String titulo = (estado == GRAFICO_TRAFICO) ? "MONITOR TRAFICO" : "CAPTURANDO PCAP";
                    Launcher::dibujarTextoCentrado(canvas, titulo.c_str(), 35, 1, IRON_GREEN);
                    canvas->drawLine(20, 45, 220, 45, IRON_DARK);

                    // DIBUJAR HISTOGRAMA DE BARRAS EN TIEMPO REAL
                    long max_pkts = 1;
                    for(int i=1; i<=MAX_CHANNELS; i++) if(pkts_per_channel[i] > max_pkts) max_pkts = pkts_per_channel[i];
                    
                    int chart_x = 30;
                    int chart_y = 150;
                    int bar_w = 10;
                    for(int i=1; i<=MAX_CHANNELS; i++) {
                        int bar_h = map(pkts_per_channel[i], 0, max_pkts, 0, 60);
                        uint16_t colorBarra = (i == current_channel) ? IRON_RED : IRON_CYAN;
                        canvas->fillRect(chart_x + ((i-1)*(bar_w + 2)), chart_y - bar_h, bar_w, bar_h, colorBarra);
                    }
                    
                    char chStr[32]; sprintf(chStr, "Pkts: %ld", total_pkts);
                    Launcher::dibujarTextoCentrado(canvas, chStr, 60, 1, WHITE);

                    Launcher::dibujarTextoCentrado(canvas, "< Detener = Clic/Izq", 175, 1, IRON_DARK);
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

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif