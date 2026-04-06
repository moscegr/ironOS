#ifndef APPSTATS_H
#define APPSTATS_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <esp_mac.h>
#include <SD_MMC.h> // Añadido para leer la SD

class AppStats : public App {
private:
    unsigned long ultimaActualizacionDatos = 0;
    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    
    // Variables para la animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    
    // Scroll de bloques (¡Ahora son 7 para incluir la SD!)
    int indiceScroll = 0;
    const int NUM_BLOQUES = 7;

    // --- VARIABLES DE TELEMETRÍA (Cache) ---
    char txtUp[32];
    int pctRam = 0; 
    uint32_t freeRamKB = 0;
    int pctPsram = 0; 
    float freePsramMB = 0; 
    uint32_t totalPsram = 0;
    uint32_t freePsram = 0; 
    int pctSketch = 0; 
    float freeSketchMB = 0;
    
    // Variables de la SD fusionadas
    bool sdActiva = false;
    int pctSd = 0;
    uint32_t freeSdMB = 0;

    int cpuFreq = 0; 
    int tempChip = 0;
    int flashMB = 0; 
    int flashFreq = 0;
    char txtMac[32]; 
    char txtSdk[32];

public:
    const char* obtenerNombre() override { return "TELEMETRIA"; }
    const uint8_t* obtenerEmoji() override { return emoji_stats; }
    
    void setup() override {}
    
    void alEntrar() override {
        ultimaActualizacionDatos = 0; 
        anguloNeon = 0;
        indiceScroll = 0;
        ultimoFrameNeon = millis();
        ultimoMovimiento = millis();
        pantallaActualizada = false;

        // --- CORRECCIÓN: INICIALIZAR LA MICROSD ---
        SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
        sdActiva = SD_MMC.begin("/sdcard", true);
    }
    
    void alSalir() override {
        // --- APAGAR LA MICROSD AL SALIR PARA AHORRAR ENERGÍA ---
        if (sdActiva) SD_MMC.end();
    }

    void loop(Arduino_Canvas* canvas) override {
        
        // --- MOTOR UNIFICADO DE LECTURA ---
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);

        // --- LÓGICA DE SCROLL ---
        if (millis() - ultimoMovimiento > 200) {
            if (arriba) { indiceScroll--; pantallaActualizada = false; ultimoMovimiento = millis(); }
            if (abajo)  { indiceScroll++; pantallaActualizada = false; ultimoMovimiento = millis(); }
            
            if (indiceScroll < 0) indiceScroll = NUM_BLOQUES - 1;
            if (indiceScroll >= NUM_BLOQUES) indiceScroll = 0;
        }

        // --- ANIMACIÓN NEÓN GLOBAL ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        // --- LECTURA DE SENSORES Y MEMORIA ---
        if (millis() - ultimaActualizacionDatos > 1000 || ultimaActualizacionDatos == 0) {
            ultimaActualizacionDatos = millis();
            
            // 1. UPTIME
            unsigned long seg = millis() / 1000;
            sprintf(txtUp, "UPTIME: %02dh %02dm %02ds", (seg / 3600), (seg / 60) % 60, seg % 60);
            
            // 2. RAM
            uint32_t freeRam = ESP.getFreeHeap();
            uint32_t totalRam = ESP.getHeapSize();
            pctRam = ((totalRam - freeRam) * 100) / totalRam;
            freeRamKB = freeRam / 1024;
            
            // 3. PSRAM
            freePsram = ESP.getFreePsram();
            totalPsram = ESP.getPsramSize();
            if (totalPsram > 0) {
                pctPsram = ((totalPsram - freePsram) * 100) / totalPsram;
                freePsramMB = freePsram / 1048576.0; 
            }

            // 4. SKETCH
            uint32_t usedSketch = ESP.getSketchSize();
            uint32_t totalSketch = ESP.getFreeSketchSpace() + usedSketch;
            pctSketch = (usedSketch * 100) / totalSketch;
            freeSketchMB = ESP.getFreeSketchSpace() / 1048576.0;

            // 5. MICRO SD (FUSIONADO)
            if (SD_MMC.cardSize() > 0) {
                sdActiva = true;
                uint32_t totalSd = SD_MMC.totalBytes() / 1048576;
                uint32_t usedSd = SD_MMC.usedBytes() / 1048576;
                if (totalSd > 0) {
                    pctSd = (usedSd * 100) / totalSd;
                    freeSdMB = totalSd - usedSd;
                }
            } else {
                sdActiva = false;
            }

            // 6. HARDWARE
            cpuFreq = ESP.getCpuFreqMHz();
            tempChip = (int)temperatureRead();
            flashMB = ESP.getFlashChipSize() / 1048576;
            flashFreq = ESP.getFlashChipSpeed() / 1000000;

            // 7. SISTEMA
            uint8_t mac[6];
            esp_read_mac(mac, ESP_MAC_WIFI_STA);
            sprintf(txtMac, "MAC: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            sprintf(txtSdk, "SDK: %s", ESP.getSdkVersion());
        }

        // --- RENDERIZADO GRÁFICO ---
        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);
            
            // Anillo de Neón
            canvas->drawCircle(120, 120, 118, IRON_CYAN);
            canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
            canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
            canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 
            
            Launcher::dibujarTextoCentrado(canvas, "TELEMETRIA DE SISTEMA", 35, 1, IRON_CYAN);
            canvas->drawLine(20, 45, 220, 45, IRON_RED);

            int inicio = indiceScroll - 1;
            if (inicio < 0) inicio = 0;
            if (inicio > NUM_BLOQUES - 3) inicio = NUM_BLOQUES - 3;

            for (int i = 0; i < 3; i++) {
                int idx = inicio + i;
                int yBase = 60 + (i * 45); 

                uint16_t colorTexto = (idx == indiceScroll) ? WHITE : IRON_BLUE;

                if (idx == indiceScroll) {
                    canvas->fillRoundRect(10, yBase - 8, 220, 40, 3, IRON_RED);
                    canvas->fillTriangle(15, yBase+6, 15, yBase+14, 20, yBase+10, IRON_CYAN);
                }

                canvas->setTextSize(1);
                canvas->setTextColor(colorTexto);
                char bufferLin[40];

                switch (idx) {
                    case 0:
                        canvas->setCursor(28, yBase); canvas->print(txtUp);
                        sprintf(bufferLin, "TEMP CHIP: %d C", tempChip);
                        canvas->setCursor(28, yBase + 16); canvas->print(bufferLin);
                        break;
                    case 1:
                        sprintf(bufferLin, "RAM  (U: %d%% L: %dkB)", pctRam, freeRamKB);
                        canvas->setCursor(28, yBase); canvas->print(bufferLin);
                        canvas->fillRect(30, yBase + 16, 160, 6, (idx == indiceScroll) ? BLACK : IRON_DARK); 
                        canvas->fillRect(30, yBase + 16, (160 * pctRam) / 100, 6, IRON_CYAN); 
                        break;
                    case 2:
                        if (totalPsram > 0) {
                            sprintf(bufferLin, "PSRAM(U: %d%% L: %.1fMB)", pctPsram, freePsramMB);
                            canvas->setCursor(28, yBase); canvas->print(bufferLin);
                            canvas->fillRect(30, yBase + 16, 160, 6, (idx == indiceScroll) ? BLACK : IRON_DARK);
                            int w = (160 * pctPsram) / 100; if(w==0 && pctPsram>0) w=2;
                            canvas->fillRect(30, yBase + 16, w, 6, IRON_CYAN);
                        } else {
                            canvas->setCursor(28, yBase + 8); canvas->setTextColor(IRON_RED);
                            canvas->print("PSRAM NO DETECTADA");
                        }
                        break;
                    case 3:
                        sprintf(bufferLin, "SKETCH (U: %d%% L: %.1fMB)", pctSketch, freeSketchMB);
                        canvas->setCursor(28, yBase); canvas->print(bufferLin);
                        canvas->fillRect(30, yBase + 16, 160, 6, (idx == indiceScroll) ? BLACK : IRON_DARK); 
                        canvas->fillRect(30, yBase + 16, (160 * pctSketch) / 100, 6, IRON_CYAN); 
                        break;
                    case 4: // NUEVO BLOQUE MICRO SD
                        if (sdActiva) {
                            sprintf(bufferLin, "SD   (U: %d%% L: %luMB)", pctSd, freeSdMB);
                            canvas->setCursor(28, yBase); canvas->print(bufferLin);
                            canvas->fillRect(30, yBase + 16, 160, 6, (idx == indiceScroll) ? BLACK : IRON_DARK); 
                            int w = (160 * pctSd) / 100; if(w==0 && pctSd>0) w=2;
                            canvas->fillRect(30, yBase + 16, w, 6, IRON_CYAN);
                        } else {
                            canvas->setCursor(28, yBase + 8); canvas->setTextColor(IRON_RED);
                            canvas->print("SD NO DETECTADA");
                        }
                        break;
                    case 5:
                        sprintf(bufferLin, "CPU: %d MHz", cpuFreq);
                        canvas->setCursor(28, yBase); canvas->print(bufferLin);
                        sprintf(bufferLin, "FLASH: %d MB @ %d MHz", flashMB, flashFreq);
                        canvas->setCursor(28, yBase + 16); canvas->print(bufferLin);
                        break;
                    case 6:
                        canvas->setCursor(28, yBase); canvas->print(txtMac);
                        canvas->setCursor(28, yBase + 16); 
                        String sdkCorto = String(txtSdk).substring(0, 20); 
                        canvas->print(sdkCorto);
                        break;
                }
            }

            canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
            canvas->setCursor(120 - 25, 212);
            canvas->setTextColor(IRON_RED);
            canvas->print("MANTENER CLICK");

            canvas->flush();
            pantallaActualizada = true;
        }
        delay(10); 
    }
};
#endif