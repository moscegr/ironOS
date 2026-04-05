#ifndef APPDUCKY_H
#define APPDUCKY_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <SD_MMC.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

#define LED_PIN 48

class AppDucky : public App {
private:
    enum EstadoDucky { MENU_SCRIPTS, CONFIRMANDO, EJECUTANDO, FINALIZADO };
    EstadoDucky estado = MENU_SCRIPTS;

    bool sdOk = false;
    int numScripts = 0;
    String nombresScripts[30];
    int indiceSeleccionado = 0;
    String scriptActual = "";

    // Intérprete Ducky
    USBHIDKeyboard Keyboard;
    File duckyFile; // Archivo global para lectura asíncrona
    int lineasTotales = 0;
    int lineaActual = 0;
    bool usbIniciado = false;

    // UI
    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    void cargarScripts() {
        numScripts = 0;
        if (!sdOk) return;

        if (!SD_MMC.exists("/ducky")) {
            SD_MMC.mkdir("/ducky");
        }

        File dir = SD_MMC.open("/ducky");
        if (!dir) return;

        File file = dir.openNextFile();
        while (file && numScripts < 30) {
            String fileName = file.name();
            if (fileName.endsWith(".txt") || fileName.endsWith(".TXT")) {
                int index = fileName.lastIndexOf('/');
                nombresScripts[numScripts] = (index >= 0) ? fileName.substring(index + 1) : fileName;
                numScripts++;
            }
            file = dir.openNextFile();
        }
        dir.close();
    }

    int contarLineas(String ruta) {
        int conteo = 0;
        File f = SD_MMC.open(ruta, FILE_READ);
        if (!f) return 0;
        while (f.available()) {
            f.readStringUntil('\n');
            conteo++;
        }
        f.close();
        return conteo;
    }

    void procesarComandoDucky(String linea) {
        linea.trim();
        if (linea.length() == 0 || linea.startsWith("REM")) return;

        int espacio = linea.indexOf(' ');
        String cmd = (espacio > 0) ? linea.substring(0, espacio) : linea;
        String args = (espacio > 0) ? linea.substring(espacio + 1) : "";

        if (cmd == "STRING") { Keyboard.print(args); } 
        else if (cmd == "DELAY") { delay(args.toInt()); } 
        else if (cmd == "ENTER") { Keyboard.write(KEY_RETURN); } 
        else if (cmd == "GUI" || cmd == "WINDOWS") {
            if (args.length() > 0) {
                Keyboard.press(KEY_LEFT_GUI);
                Keyboard.press(args[0]);
                Keyboard.releaseAll();
            } else {
                Keyboard.write(KEY_LEFT_GUI);
            }
        }
        else if (cmd == "TAB") { Keyboard.write(KEY_TAB); }
        else if (cmd == "SPACE") { Keyboard.write(' '); }
        else if (cmd == "UP" || cmd == "UPARROW") { Keyboard.write(KEY_UP_ARROW); }
        else if (cmd == "DOWN" || cmd == "DOWNARROW") { Keyboard.write(KEY_DOWN_ARROW); }
        else if (cmd == "LEFT" || cmd == "LEFTARROW") { Keyboard.write(KEY_LEFT_ARROW); }
        else if (cmd == "RIGHT" || cmd == "RIGHTARROW") { Keyboard.write(KEY_RIGHT_ARROW); }
        else if (cmd == "ESCAPE") { Keyboard.write(KEY_ESC); }
        else if (cmd == "CTRL" || cmd == "CONTROL") {
            Keyboard.press(KEY_LEFT_CTRL);
            if (args.length() > 0) Keyboard.press(args[0]);
            Keyboard.releaseAll();
        }
        else if (cmd == "ALT") {
            Keyboard.press(KEY_LEFT_ALT);
            if (args.length() > 0) Keyboard.press(args[0]);
            Keyboard.releaseAll();
        }
        else if (cmd == "SHIFT") {
            Keyboard.press(KEY_LEFT_SHIFT);
            if (args.length() > 0) Keyboard.press(args[0]);
            Keyboard.releaseAll();
        }
        delay(10); // Estabilización USB
    }

public:
    const char* obtenerNombre() override { return "IRON DUCKY"; }
    const uint8_t* obtenerEmoji() override { return emoji_sd; } 
    
    void setup() override {}

    void alEntrar() override {
        estado = MENU_SCRIPTS;
        indiceSeleccionado = 0;
        pantallaActualizada = false;
        ultimoMovimiento = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        anguloNeon = 0;
        ultimoFrameNeon = millis();

        SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
        sdOk = SD_MMC.begin("/sdcard", true);
        if (sdOk) cargarScripts();

        if (!usbIniciado) {
            Keyboard.begin();
            USB.begin(); 
            usbIniciado = true;
        }
        
        neopixelWrite(LED_PIN, 0, 0, 0); 
    }
    
    void alSalir() override {
        if (duckyFile) duckyFile.close();
        if (sdOk) SD_MMC.end(); 
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

        // ==========================================
        // 1. LISTA DE SCRIPTS
        // ==========================================
        if (estado == MENU_SCRIPTS) {
            if (millis() - ultimoMovimiento > 200 && sdOk) {
                if (arriba) { indiceSeleccionado--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceSeleccionado++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceSeleccionado < 0) indiceSeleccionado = (numScripts > 0) ? numScripts - 1 : 0;
                if (indiceSeleccionado >= numScripts) indiceSeleccionado = 0;
            }

            if (clicSimple && numScripts > 0) {
                scriptActual = nombresScripts[indiceSeleccionado];
                lineasTotales = contarLineas("/ducky/" + scriptActual);
                lineaActual = 0;
                estado = CONFIRMANDO;
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                if (!sdOk) {
                    canvas->drawCircle(120, 120, 118, IRON_RED);
                    Launcher::dibujarTextoCentrado(canvas, "ERROR DE LECTURA SD", 100, 1, IRON_RED);
                } else {
                    canvas->drawCircle(120, 120, 118, IRON_CYAN);
                    canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_BLUE); 

                    Launcher::dibujarTextoCentrado(canvas, "PAYLOADS DUCKY", 35, 1, IRON_CYAN);
                    canvas->drawLine(20, 45, 220, 45, IRON_DARK);

                    if (numScripts == 0) {
                        Launcher::dibujarTextoCentrado(canvas, "CARPETA /ducky VACIA", 110, 1, IRON_BLUE);
                    } else {
                        int inicio = indiceSeleccionado - 1;
                        if (inicio < 0) inicio = 0;
                        if (numScripts > 3 && inicio > numScripts - 3) inicio = numScripts - 3;

                        for (int i = 0; i < 3 && (inicio + i) < numScripts; i++) {
                            int idx = inicio + i;
                            int yPos = 65 + (i * 35);

                            canvas->setTextSize(1);
                            canvas->setCursor(30, yPos);
                            
                            if (idx == indiceSeleccionado) {
                                canvas->fillRoundRect(15, yPos - 10, 210, 25, 3, IRON_RED);
                                canvas->fillTriangle(20, yPos-2, 20, yPos+6, 25, yPos+2, WHITE);
                                canvas->setTextColor(WHITE);
                            } else {
                                canvas->setTextColor(IRON_BLUE);
                            }
                            canvas->print(nombresScripts[idx].substring(0, 20));
                        }
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
        // 2. CONFIRMANDO ATAQUE
        // ==========================================
        else if (estado == CONFIRMANDO) {
            if (clicSimple) {
                // Abrir archivo y preparar estado (Se enciende el LED una sola vez)
                duckyFile = SD_MMC.open("/ducky/" + scriptActual, FILE_READ);
                estado = EJECUTANDO;
                pantallaActualizada = false;
                neopixelWrite(LED_PIN, 255, 0, 0); 
            } else if (izq && millis() - ultimoMovimiento > 200) {
                estado = MENU_SCRIPTS;
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "PREPARAR INYECCION", 45, 1, IRON_RED);
                canvas->drawLine(20, 55, 220, 55, IRON_DARK);
                
                Launcher::dibujarTextoCentrado(canvas, scriptActual.c_str(), 80, 1, WHITE);
                
                char linStr[32]; sprintf(linStr, "Lineas a inyectar: %d", lineasTotales);
                Launcher::dibujarTextoCentrado(canvas, linStr, 100, 1, IRON_CYAN);

                canvas->fillRoundRect(30, 130, 180, 30, 3, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "CLIC PARA ATACAR", 140, 1, WHITE);
                
                Launcher::dibujarTextoCentrado(canvas, "< Volver = Izq", 185, 1, IRON_DARK);

                canvas->flush();
                pantallaActualizada = true;
            }
        }
        // ==========================================
        // 3. EJECUTANDO DUCKYSCRIPT (ASÍNCRONO)
        // ==========================================
        else if (estado == EJECUTANDO) {
            
            if (duckyFile && duckyFile.available()) {
                String cmdLine = duckyFile.readStringUntil('\n');
                procesarComandoDucky(cmdLine);
                lineaActual++;
            } else {
                // Terminó el archivo
                if (duckyFile) duckyFile.close();
                estado = FINALIZADO;
                pantallaActualizada = false;
                neopixelWrite(LED_PIN, 0, 255, 0); // Led Verde éxito (Una sola vez)
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_RED);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, WHITE); 
                
                Launcher::dibujarTextoCentrado(canvas, "ATACANDO...", 60, 2, IRON_RED);
                
                int pct = (lineaActual * 100) / max(1, lineasTotales);
                canvas->drawRect(40, 110, 160, 20, WHITE);
                canvas->fillRect(42, 112, map(pct, 0, 100, 0, 156), 16, IRON_RED);
                
                char pctStr[16]; sprintf(pctStr, "%d%%", pct);
                Launcher::dibujarTextoCentrado(canvas, pctStr, 140, 1, WHITE);
                
                canvas->flush();
                pantallaActualizada = true;
            }
        }
        // ==========================================
        // 4. FINALIZADO
        // ==========================================
        else if (estado == FINALIZADO) {
            if (clicSimple || izq) {
                estado = MENU_SCRIPTS;
                neopixelWrite(LED_PIN, 0, 0, 0);
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }

            if (!pantallaActualizada) {
                canvas->fillScreen(BLACK);
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                Launcher::dibujarTextoCentrado(canvas, "INYECCION EXITOSA", 90, 1, IRON_GREEN);
                Launcher::dibujarTextoCentrado(canvas, "Payload entregado", 115, 1, WHITE);
                
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