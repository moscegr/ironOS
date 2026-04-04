#ifndef APPEVILTWIN_H
#define APPEVILTWIN_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

#define LED_PIN 48

// ==========================================
// 1. PÁGINA DE LOGIN FALSO (RESPONSIVA)
// ==========================================
const char login_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Iniciar sesión</title>

  <style>
    body {
      font-family: Arial, Helvetica, sans-serif;
      background-color: #ffffff;
      margin: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      color: #202124;
    }

    .container {
      width: 100%;
      max-width: 380px;
      padding: 40px 30px;
      border: 1px solid #dadce0;
      border-radius: 8px;
      box-sizing: border-box;
    }

    /* Logo estilo multicolor */
    .logo {
      text-align: center;
      font-size: 28px;
      font-weight: bold;
      margin-bottom: 25px;
      letter-spacing: 1px;
    }

    .g1 { color: #4285f4; }
    .g2 { color: #ea4335; }
    .g3 { color: #fbbc05; }
    .g4 { color: #34a853; }

    h2 {
      font-size: 24px;
      font-weight: normal;
      margin-bottom: 10px;
    }

    p {
      font-size: 14px;
      color: #5f6368;
      margin-bottom: 30px;
    }

    .input-group {
      margin-bottom: 20px;
    }

    input {
      width: 100%;
      padding: 14px 12px;
      border: 1px solid #dadce0;
      border-radius: 4px;
      font-size: 14px;
      outline: none;
      transition: 0.2s;
    }

    input:focus {
      border: 2px solid #1a73e8;
      padding: 13px 11px;
    }

    .actions {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-top: 20px;
    }

    .link {
      color: #1a73e8;
      font-size: 14px;
      text-decoration: none;
      cursor: pointer;
    }

    .link:hover {
      text-decoration: underline;
    }

    button {
      background-color: #1a73e8;
      color: white;
      border: none;
      padding: 10px 22px;
      border-radius: 4px;
      font-size: 14px;
      cursor: pointer;
      transition: background 0.3s;
    }

    button:hover {
      background-color: #1669c1;
    }

    .footer {
      text-align: center;
      font-size: 12px;
      color: #5f6368;
      margin-top: 25px;
    }

  </style>
</head>

<body>

  <div class="container">

    <!-- Logo multicolor -->
    <div class="logo">
      <span class="g1">G</span>
      <span class="g2">O</span>
      <span class="g3">O</span>
      <span class="g1">G</span>
      <span class="g4">L</span>
      <span class="g2">E</span>
    </div>

    <h2>Iniciar sesión</h2>
    <p>Usa tu cuenta para conectarte a la red</p>

    <form action="/auth" method="POST">

      <div class="input-group">
        <input type="text" name="user" placeholder="Correo electrónico o teléfono" required>
      </div>

      <div class="input-group">
        <input type="password" name="pass" placeholder="Contraseña" required>
      </div>

      <div class="actions">
        <a class="link">¿Olvidaste tu contraseña?</a>
        <button type="submit">Siguiente</button>
      </div>

    </form>

    <div class="footer">
      Acceso seguro a red pública
    </div>

  </div>

</body>
</html>
)rawliteral";

// ==========================================
// 2. PÁGINA DE ADVERTENCIA EDUCATIVA
// ==========================================
const char warning_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>¡Alerta de Seguridad!</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background-color: #2c3e50; margin: 0; display: flex; justify-content: center; align-items: center; height: 100vh; }
    .container { background-color: #ecf0f1; padding: 30px; border-radius: 8px; box-shadow: 0px 10px 20px rgba(0,0,0,0.3); text-align: center; max-width: 90%; width: 350px; }
    h1 { color: #e74c3c; margin-bottom: 10px; font-size: 22px; }
    .alert-box { background-color: #fce4e4; border-left: 5px solid #e74c3c; padding: 15px; margin-top: 20px; text-align: left; font-size: 14px; color: #333; }
  </style>
</head>
<body>
  <div class="container">
    <h1>¡SIMULACIÓN TERMINADA!</h1>
    <div class="alert-box">
      <p><b>Acabas de introducir tus datos en un Portal Cautivo falso (Evil Twin).</b></p>
      <p>Tus credenciales <b>NO</b> han sido guardadas. Esta es una demostración académica sobre los riesgos de conectarse a redes Wi-Fi públicas y abiertas.</p>
    </div>
    <p style="margin-top:20px; font-size: 14px; color: #7f8c8d;">Laboratorio de Ciberdefensa</p>
  </div>
</body>
</html>
)rawliteral";

class AppEvilTwin : public App {
private:
    DNSServer* dnsServer = nullptr;
    WebServer* webServer = nullptr;
    
    unsigned long ultimoParpadeo = 0;
    bool estadoLed = false;
    bool pantallaActualizada = false;
    int clientesConectados = 0;

    // Variables animación neón
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    const byte DNS_PORT = 53;
    IPAddress apIP;

public:
    const char* obtenerNombre() override { return "EVIL TWIN"; }
    const uint8_t* obtenerEmoji() override { return emoji_wifi; } 
    
    void setup() override {}

    void alEntrar() override {
        pantallaActualizada = false;
        clientesConectados = 0;
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        apIP = IPAddress(192, 168, 4, 1);

        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP("CFE_ABIERTA"); 

        dnsServer = new DNSServer();
        dnsServer->start(DNS_PORT, "*", apIP);

        webServer = new WebServer(80);
        
        // --- RUTAS DEL SERVIDOR WEB ---
        webServer->onNotFound([this]() {
            webServer->send(200, "text/html", login_html);
        });
        webServer->on("/", HTTP_GET, [this]() {
            webServer->send(200, "text/html", login_html);
        });
        
        webServer->on("/auth", HTTP_POST, [this]() {
            webServer->send(200, "text/html", warning_html);
        });

        webServer->begin();
    }
    
    void alSalir() override {
        if (dnsServer) { dnsServer->stop(); delete dnsServer; dnsServer = nullptr; }
        if (webServer) { webServer->stop(); delete webServer; webServer = nullptr; }
        
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA); 
        neopixelWrite(LED_PIN, 0, 0, 0); 
    }

    void loop(Arduino_Canvas* canvas) override {
        
        if (dnsServer) dnsServer->processNextRequest();
        if (webServer) webServer->handleClient();

        int clientesNuevos = WiFi.softAPgetStationNum();
        if (clientesNuevos != clientesConectados) {
            clientesConectados = clientesNuevos;
            pantallaActualizada = false; 
        }

        // --- ANIMACIÓN NEÓN ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        if (millis() - ultimoParpadeo > 600) {
            ultimoParpadeo = millis();
            estadoLed = !estadoLed;
            if (estadoLed) {
                if (clientesConectados > 0) {
                    neopixelWrite(LED_PIN, 0, 255, 0); // Verde (Atrapado)
                } else {
                    neopixelWrite(LED_PIN, 0, 0, 150); // Azul tenue (Esperando)
                }
            } else {
                neopixelWrite(LED_PIN, 0, 0, 0);
            }
        }

        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);
            
            uint16_t colorHUD = (clientesConectados > 0) ? IRON_GREEN : IRON_CYAN;
            
            canvas->drawCircle(120, 120, 118, colorHUD); 
            
            // Reacción del Neón según el estado de conexión
            if (clientesConectados > 0) {
                // Estela de Captura (Verde)
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_GREEN); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 
            } else {
                // Estela de Escucha (Cyan/Azul)
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_CYAN); 
                canvas->drawArc(120, 120, 116, 114, (anguloNeon + 40) % 360, (anguloNeon + 45) % 360, WHITE); 
            }

            Launcher::dibujarTextoCentrado(canvas, "PORTAL CAUTIVO", 35, 1, colorHUD);
            canvas->drawLine(30, 45, 210, 45, IRON_DARK);

            Launcher::dibujarTextoCentrado(canvas, "Transmitiendo:", 70, 1, IRON_BLUE);
            Launcher::dibujarTextoCentrado(canvas, "CFE_ABIERTA", 90, 2, WHITE);
            
            char clientesStr[32];
            sprintf(clientesStr, "Victimas: %d", clientesConectados);
            
            if (clientesConectados > 0) {
                Launcher::dibujarTextoCentrado(canvas, clientesStr, 130, 2, IRON_GREEN);
                Launcher::dibujarTextoCentrado(canvas, "!DISPOSITIVO ATRAPADO!", 160, 1, IRON_RED);
            } else {
                Launcher::dibujarTextoCentrado(canvas, clientesStr, 130, 1, IRON_CYAN);
                Launcher::dibujarTextoCentrado(canvas, "Esperando conexiones...", 160, 1, IRON_DARK);
            }

            canvas->drawBitmap(120 - 45, 200, emoji_back, 16, 16, colorHUD);
            canvas->setCursor(120 - 25, 200);
            canvas->setTextColor(IRON_RED);
            canvas->print("DOBLE CLICK");

            canvas->flush();
            pantallaActualizada = true;
        }
        
        delay(2); 
    }
};
#endif