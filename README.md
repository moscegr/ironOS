# ironOS
IronOS es un ecosistema táctico de código abierto para ESP32-S3 con pantalla circular GC9A01. Diseñado para pruebas de penetración Wi-Fi/BLE, monitoreo de telemetría y gestión de archivos, todo bajo una interfaz de usuario fluida con animaciones de neón a 30 FPS y un motor de navegación unificado mediante joystick.

# 🛡️ IronOS v1.0 - Tactical ESP32-S3 Dashboard

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32--S3-orange.svg)
![Framework](https://img.shields.io/badge/framework-Arduino/PlatformIO-green.svg)

**IronOS** es un sistema operativo minimalista y potente diseñado para dispositivos "wearable" o terminales portátiles basados en el chip **ESP32-S3 (N16R8)**. Este proyecto combina una interfaz gráfica de alto rendimiento con herramientas avanzadas de ciberseguridad y diagnóstico de hardware.



## 🚀 Características Principales
* **Interfaz Fluida:** Motor gráfico optimizado para la pantalla circular GC9A01 con estelas de neón animadas a 30 FPS.
* **Navegación Unificada:** Control total mediante joystick con lógica de calibración persistente (EEPROM/Preferences).
* **Arquitectura Modular:** Sistema basado en aplicaciones independientes fáciles de extender.
* **Gestión Energética:** Monitoreo en tiempo real de RAM, PSRAM (8MB) y temperatura del SoC.

---

## 🛠️ Hardware y Conexiones

### Componentes Utilizados
* **MCU:** ESP32-S3 DevKitC-1 (N16R8V)
* **Display:** LCD Circular 1.28" (GC9A01) SPI
* **Input:** Joystick Analógico de 2 ejes + Botón central
* **Storage:** Módulo MicroSD Externo (Protocolo SPI)
* **LED:** Neopixel RGB integrado (GPIO 48)

### Diagrama de Conexión (Pinout)

| Componente | Pin ESP32-S3 | Función |
| :--- | :--- | :--- |
| **TFT LCD** | GPIO 7 / 6 / 1 / 4 / 5 | SCK / MOSI / CS / DC / RST |
| **Joystick** | GPIO 2 / 3 / 8 | Eje X / Eje Y / SW (Click) |
| **MicroSD** | GPIO 12 / 13 / 11 / 10 | SCK / MISO / MOSI / CS |
| **RGB LED** | GPIO 48 | Status Feedback |

---

## 📱 Aplicaciones Incluidas

| App | Función |
| :--- | :--- |
| **📡 WIFI SCAN** | Escáner asíncrono de redes 2.4GHz con visualización de RSSI y Canales. |
| **☠️ DEAUTH** | Herramienta de auditoría para pruebas de desautenticación (802.11 injection). |
| **🔗 BLE SCAN** | Rastreador de dispositivos Bluetooth Low Energy con cálculo de distancia aproximada. |
| **🌐 EVIL TWIN** | Simulación de Portal Cautivo (DNS Spoofing + Web Server) para fines educativos. |
| **🛰️ TELEMETRÍA** | Monitor de sistema: Uptime, uso de PSRAM (8MB), RAM interna y temperatura. |
| **📂 ARCHIVOS** | Explorador de archivos en tarjeta SD mediante bus SPI dedicado. |
| **⚙️ AJUSTES** | Calibración de ejes e inversión de controles del Joystick con guardado permanente. |
| **🐍 SNAKE** | El clásico juego adaptado a pantalla circular con dificultad progresiva. |
| **ℹ️ INFO** | Créditos del proyecto y especificaciones técnicas del hardware (Ing. CEGR). |

---

## 📸 Galería de Funcionamiento
*(Aquí puedes insertar las imágenes que planeas subir)*

---

## 💻 Instalación y Compilación

Este proyecto está desarrollado bajo **PlatformIO**. Para desplegarlo:

1. Instala VS Code y la extensión PlatformIO.
2. Clona este repositorio: `git clone https://github.com/tu-usuario/IronOS.git`
3. Abre la carpeta del proyecto.
4. En `platformio.ini`, asegúrate de tener configurada la PSRAM:
   ```ini
   
   [env:esp32-s3-ironos]
   platform = espressif32@5.3.0
   board = esp32-s3-devkitc-1
   framework = arduino
   monitor_speed = 115200
   monitor_filters = esp32_exception_decoder
   
   ; --- CONFIGURACIÓN N16R8: 16MB Flash QIO + 8MB PSRAM OPI ---
   board_build.arduino.memory_type = qio_opi
   board_build.flash_mode = qio
   board_build.flash_size = 16MB
   board_build.partitions = default_16MB.csv
   board_upload.flash_size = 16MB
   board_upload.maximum_size = 16777216
   
   lib_deps = 
       moononournation/GFX Library for Arduino @ 1.4.7
   
   build_flags = 
       -DBOARD_HAS_PSRAM
       -DARDUINO_USB_MODE=1
       -DARDUINO_USB_CDC_ON_BOOT=1
       -DCORE_DEBUG_LEVEL=0
