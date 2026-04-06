#ifndef APPSD_H
#define APPSD_H
#include "App.h"
#include "Emojis.h"
#include "ui/Launcher.h"
#include "config.h" 
#include <SD_MMC.h>

#define LED_PIN 48

// Estructura para almacenar información del explorador
struct ArchivoSD {
    String nombre;
    bool esDirectorio;
};

class AppSD : public App {
private:
    enum EstadoSD { EXPLORANDO, OPCIONES_ARCHIVO, LEYENDO_TXT, CONFIRMANDO_BORRADO };
    EstadoSD estado = EXPLORANDO;

    bool sdOk = false;
    
    // Explorador de Archivos
    String rutaActual = "/";
    ArchivoSD archivos[30]; // Máximo 30 elementos por carpeta para no saturar RAM
    int numArchivos = 0;
    int indiceSeleccionado = 0;

    // Archivo Objetivo
    String archivoSeleccionado = "";
    bool esCarpetaSel = false;
    
    // Menú de Opciones
    int indiceOpcion = 0;
    int maxOpciones = 0;
    String opcionesMenu[3];

    // Lector de Texto
    String lineasTxt[30];
    int totalLineasTxt = 0;
    int offsetTxt = 0; // Desplazamiento vertical

    // Variables UI y Animación
    unsigned long ultimoMovimiento = 0;
    bool pantallaActualizada = false;
    bool btnAnterior = false;
    int anguloNeon = 0;
    unsigned long ultimoFrameNeon = 0;

    // ==========================================
    // LÓGICA DEL SISTEMA DE ARCHIVOS
    // ==========================================
    String extraerNombre(String ruta) {
        int index = ruta.lastIndexOf('/');
        if(index >= 0) return ruta.substring(index + 1);
        return ruta;
    }

    String rutaPadre(String ruta) {
        if(ruta == "/") return "/";
        int index = ruta.lastIndexOf('/');
        if(index <= 0) return "/";
        return ruta.substring(0, index);
    }

    void cargarArchivos() {
        numArchivos = 0;
        if (!sdOk) return;

        // Si no estamos en la raíz, agregar opción para regresar ".."
        if (rutaActual != "/") {
            archivos[numArchivos].nombre = "..";
            archivos[numArchivos].esDirectorio = true;
            numArchivos++;
        }

        File dir = SD_MMC.open(rutaActual);
        if (!dir || !dir.isDirectory()) return;

        File file = dir.openNextFile();
        while (file && numArchivos < 30) {
            archivos[numArchivos].nombre = extraerNombre(String(file.name()));
            archivos[numArchivos].esDirectorio = file.isDirectory();
            numArchivos++;
            file = dir.openNextFile();
        }
        dir.close();
    }

    void cargarTexto() {
        totalLineasTxt = 0;
        offsetTxt = 0;
        String rutaCompleta = rutaActual;
        if (rutaCompleta != "/") rutaCompleta += "/";
        rutaCompleta += archivoSeleccionado;

        File f = SD_MMC.open(rutaCompleta, FILE_READ);
        if(!f) {
            lineasTxt[0] = "Error al abrir archivo.";
            totalLineasTxt = 1;
            return;
        }
        
        while(f.available() && totalLineasTxt < 30) {
            String l = f.readStringUntil('\n');
            l.trim(); // Limpiar retornos de carro
            // Si la línea es muy larga, cortarla para evitar fallos de memoria
            if(l.length() > 30) l = l.substring(0, 30) + "...";
            lineasTxt[totalLineasTxt++] = l;
        }
        f.close();

        if(totalLineasTxt == 0) {
            lineasTxt[0] = "[ Archivo Vacio ]";
            totalLineasTxt = 1;
        }
    }

    void ejecutarBorrado() {
        String rutaCompleta = rutaActual;
        if (rutaCompleta != "/") rutaCompleta += "/";
        rutaCompleta += archivoSeleccionado;

        if (esCarpetaSel) {
            SD_MMC.rmdir(rutaCompleta); // Solo borra si está vacía
        } else {
            SD_MMC.remove(rutaCompleta);
        }
    }

    void abrirOpciones(String nombre, bool isDir) {
        archivoSeleccionado = nombre;
        esCarpetaSel = isDir;
        indiceOpcion = 0;
        estado = OPCIONES_ARCHIVO;

        if (isDir) {
            opcionesMenu[0] = "1. BORRAR CARPETA";
            opcionesMenu[1] = "2. CANCELAR";
            maxOpciones = 2;
        } else {
            opcionesMenu[0] = "1. LEER TEXTO";
            opcionesMenu[1] = "2. BORRAR ARCHIVO";
            opcionesMenu[2] = "3. CANCELAR";
            maxOpciones = 3;
        }
    }

public:
    const char* obtenerNombre() override { return "EXPLORADOR SD"; }
    const uint8_t* obtenerEmoji() override { return emoji_sd; }
    
    void setup() override {}

    void alEntrar() override {
        estado = EXPLORANDO;
        rutaActual = "/";
        indiceSeleccionado = 0;
        pantallaActualizada = false;
        ultimoMovimiento = millis();
        btnAnterior = (digitalRead(JOY_SW) == LOW);
        anguloNeon = 0;
        ultimoFrameNeon = millis();

        // Levantar tarjeta en Modo 1-Bit nativo
        SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
        sdOk = SD_MMC.begin("/sdcard", true);
        
        if (sdOk) {
            cargarArchivos();
            neopixelWrite(LED_PIN, 0, 255, 0); // Led Verde: Éxito
        } else {
            neopixelWrite(LED_PIN, 255, 0, 0); // Led Rojo: Falla
        }
    }
    
    void alSalir() override {
        if (sdOk) SD_MMC.end(); 
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

        // --- ANIMACIÓN NEÓN ---
        if (millis() - ultimoFrameNeon > 30) {
            anguloNeon = (anguloNeon + 8) % 360; 
            ultimoFrameNeon = millis();
            pantallaActualizada = false; 
        }

        // ==========================================
        // MÁQUINA DE ESTADOS
        // ==========================================

        // 1. EXPLORADOR PRINCIPAL
        if (estado == EXPLORANDO && sdOk) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceSeleccionado--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceSeleccionado++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceSeleccionado < 0) indiceSeleccionado = (numArchivos > 0) ? numArchivos - 1 : 0;
                if (indiceSeleccionado >= numArchivos) indiceSeleccionado = 0;
            }

            if (clicSimple && numArchivos > 0) {
                ArchivoSD sel = archivos[indiceSeleccionado];
                if (sel.nombre == "..") {
                    rutaActual = rutaPadre(rutaActual);
                    indiceSeleccionado = 0;
                    cargarArchivos();
                } 
                else if (sel.esDirectorio) {
                    // Entrar a directorio
                    if (rutaActual != "/") rutaActual += "/";
                    rutaActual += sel.nombre;
                    indiceSeleccionado = 0;
                    cargarArchivos();
                } 
                else {
                    abrirOpciones(sel.nombre, false);
                }
                pantallaActualizada = false;
            }
            
            // Atajo con Joystick Derecho en Carpetas para borrarla
            if (der && numArchivos > 0 && archivos[indiceSeleccionado].esDirectorio && archivos[indiceSeleccionado].nombre != "..") {
                abrirOpciones(archivos[indiceSeleccionado].nombre, true);
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }
        }
        
        // 2. MENÚ DE OPCIONES DE ARCHIVO/CARPETA
        else if (estado == OPCIONES_ARCHIVO) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba) { indiceOpcion--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo)  { indiceOpcion++; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (indiceOpcion < 0) indiceOpcion = maxOpciones - 1;
                if (indiceOpcion >= maxOpciones) indiceOpcion = 0;
            }

            if (clicSimple || izq) {
                if (izq || opcionesMenu[indiceOpcion].indexOf("CANCELAR") >= 0) {
                    estado = EXPLORANDO;
                } 
                else if (opcionesMenu[indiceOpcion].indexOf("LEER TEXTO") >= 0) {
                    cargarTexto();
                    estado = LEYENDO_TXT;
                }
                else if (opcionesMenu[indiceOpcion].indexOf("BORRAR") >= 0) {
                    indiceOpcion = 0; // Reusar para confirmación
                    estado = CONFIRMANDO_BORRADO;
                }
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }
        }

        // 3. CONFIRMACIÓN DE BORRADO
        else if (estado == CONFIRMANDO_BORRADO) {
            if (millis() - ultimoMovimiento > 200) {
                if (arriba || abajo) { indiceOpcion = !indiceOpcion; pantallaActualizada = false; ultimoMovimiento = millis(); }
            }

            if (clicSimple) {
                if (indiceOpcion == 0) { // SI
                    ejecutarBorrado();
                    indiceSeleccionado = 0;
                    cargarArchivos();
                }
                estado = EXPLORANDO;
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }
        }

        // 4. LECTOR DE TEXTO
        else if (estado == LEYENDO_TXT) {
            if (millis() - ultimoMovimiento > 150) {
                if (arriba && offsetTxt > 0) { offsetTxt--; pantallaActualizada = false; ultimoMovimiento = millis(); }
                if (abajo && offsetTxt < totalLineasTxt - 4) { offsetTxt++; pantallaActualizada = false; ultimoMovimiento = millis(); }
            }

            if (clicSimple || izq) {
                estado = EXPLORANDO;
                pantallaActualizada = false;
                ultimoMovimiento = millis();
            }
        }


        // ==========================================
        // RENDERIZADO VISUAL TÁCTICO
        // ==========================================
        if (!pantallaActualizada) {
            canvas->fillScreen(BLACK);
            
            // FALLO SD
            if (!sdOk) {
                canvas->drawCircle(120, 120, 118, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "ERROR DE LECTURA", 85, 1, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "Verifica la MicroSD", 115, 1, WHITE);
            } 
            
            // MODO 1: EXPLORADOR
            else if (estado == EXPLORANDO) {
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                canvas->drawArc(120, 120, 118, 112, anguloNeon, (anguloNeon + 60) % 360, IRON_DARK); 
                canvas->drawArc(120, 120, 117, 113, (anguloNeon + 20) % 360, (anguloNeon + 50) % 360, IRON_GREEN); 

                String rutaCorta = rutaActual;
                if(rutaCorta.length() > 16) rutaCorta = ".." + rutaCorta.substring(rutaCorta.length() - 14);
                Launcher::dibujarTextoCentrado(canvas, rutaCorta.c_str(), 35, 1, IRON_GREEN);
                canvas->drawLine(20, 45, 220, 45, IRON_DARK);

                if (numArchivos == 0) {
                    Launcher::dibujarTextoCentrado(canvas, "CARPETA VACIA", 110, 2, IRON_BLUE);
                } else {
                    int inicio = indiceSeleccionado - 1;
                    if (inicio < 0) inicio = 0;
                    if (numArchivos > 3 && inicio > numArchivos - 3) inicio = numArchivos - 3;

                    for (int i = 0; i < 3 && (inicio + i) < numArchivos; i++) {
                        int idx = inicio + i;
                        int yPos = 65 + (i * 35);

                        canvas->setTextSize(1);
                        canvas->setCursor(30, yPos);
                        
                        String textoItem = archivos[idx].nombre;
                        if (archivos[idx].esDirectorio && textoItem != "..") textoItem = "[DIR] " + textoItem;

                        if (idx == indiceSeleccionado) {
                            // Selección en ROJO
                            canvas->fillRoundRect(15, yPos - 10, 210, 25, 3, IRON_RED);
                            canvas->fillTriangle(20, yPos-2, 20, yPos+6, 25, yPos+2, WHITE);
                            canvas->setTextColor(WHITE);
                        } else {
                            canvas->setTextColor(IRON_BLUE);
                        }
                        canvas->print(textoItem.substring(0, 20));
                    }
                }
            }
            
            // MODO 2: OPCIONES DE ARCHIVO
            else if (estado == OPCIONES_ARCHIVO) {
                canvas->drawCircle(120, 120, 118, IRON_CYAN);
                Launcher::dibujarTextoCentrado(canvas, "ACCIONES", 35, 1, IRON_CYAN);
                canvas->drawLine(20, 45, 220, 45, IRON_DARK);
                Launcher::dibujarTextoCentrado(canvas, archivoSeleccionado.substring(0,18).c_str(), 55, 1, WHITE);

                for (int i = 0; i < maxOpciones; i++) {
                    int yPos = 85 + (i * 35);
                    if (i == indiceOpcion) {
                        canvas->fillRoundRect(15, yPos - 12, 210, 24, 3, IRON_RED);
                        canvas->fillTriangle(20, yPos-4, 20, yPos+4, 25, yPos, WHITE);
                        Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, WHITE);
                    } else {
                        Launcher::dibujarTextoCentrado(canvas, opcionesMenu[i].c_str(), yPos - 3, 1, IRON_BLUE);
                    }
                }
            }

            // MODO 3: CONFIRMACIÓN BORRADO
            else if (estado == CONFIRMANDO_BORRADO) {
                canvas->drawCircle(120, 120, 118, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "! ADVERTENCIA !", 50, 1, IRON_RED);
                Launcher::dibujarTextoCentrado(canvas, "Borrar definitivamente:", 70, 1, WHITE);
                Launcher::dibujarTextoCentrado(canvas, archivoSeleccionado.substring(0,20).c_str(), 90, 1, IRON_BLUE);

                String opts[2] = {"1. SI, BORRAR", "2. NO, CANCELAR"};
                for (int i = 0; i < 2; i++) {
                    int yPos = 130 + (i * 35);
                    if (i == indiceOpcion) {
                        canvas->fillRoundRect(30, yPos - 12, 180, 24, 3, IRON_RED);
                        Launcher::dibujarTextoCentrado(canvas, opts[i].c_str(), yPos - 3, 1, WHITE);
                    } else {
                        Launcher::dibujarTextoCentrado(canvas, opts[i].c_str(), yPos - 3, 1, IRON_CYAN);
                    }
                }
            }

            // MODO 4: LEYENDO TEXTO (LOGS/CREDENCIALES)
            else if (estado == LEYENDO_TXT) {
                canvas->drawCircle(120, 120, 118, IRON_GREEN);
                Launcher::dibujarTextoCentrado(canvas, archivoSeleccionado.substring(0,18).c_str(), 25, 1, IRON_GREEN);
                canvas->drawLine(20, 35, 220, 35, IRON_DARK);

                canvas->setTextSize(1);
                canvas->setTextColor(WHITE);
                for(int i=0; i<5; i++) { // Mostrar 5 líneas simultáneas
                    if(offsetTxt + i < totalLineasTxt) {
                        canvas->setCursor(20, 50 + (i * 25));
                        canvas->print(lineasTxt[offsetTxt + i]);
                    }
                }
                
                // Barra de scroll simulada
                int scrollY = map(offsetTxt, 0, max(1, totalLineasTxt - 5), 45, 160);
                canvas->fillRect(215, scrollY, 3, 20, IRON_RED);
                
                Launcher::dibujarTextoCentrado(canvas, "< Clic = Salir", 185, 1, IRON_DARK);
            }

            // Instrucción de salida global en la base
            if (estado != LEYENDO_TXT) {
                canvas->drawBitmap(120 - 45, 200, emoji_back, 32, 32, IRON_CYAN);
                canvas->setTextSize(1);
                canvas->setCursor(120 - 25, 212);
                canvas->setTextColor(IRON_RED);
                canvas->print("MANTENER CLICK");
            }

            canvas->flush();
            pantallaActualizada = true;
        }

        btnAnterior = botonAhora;
        delay(10);
    }
};
#endif