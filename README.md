# LightController

This project manages Philips Hue lights through an ESP32, integrating with a local SQLite database, Hue Bridge, and Netatmo sensors. It includes a local web server for management and data visualization.

## 1. Project Configuration

### ESP32 Firmware Configuration (`include/configuration.h`)

The ESP32 firmware uses `include/configuration.h` for its credentials. Ensure this file exists and is correctly configured:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// WiFi Credentials
#define WIFI_SSID  "Your_SSID"
#define PASSWORD  "Your_WiFi_Password"

// Hue Bridge Configuration
#define BRIDGE_IP "192.168.1.xxx"
#define CREDENTIALS_FILE  "/hue_creds.json"

// Database 
#define DB_PATH "lights.db"

// Open Weather Map
#define OWM_LON "12345" 
#define OWM_LAT "12345"
#define OWM_API_KEY "12345"

// Netatmo
#define NETATMO_MAC "XX:XX:XX:XX" // MAC of the station
#define NETATMO_MODULE_ID "XX:XX:XX:XX" // MAC of the module
#define NETATMO_TOKEN_FILE "token.json" // The location, in LittleFS of the OAuth Token
#define NETATMO_CLIENT_SECRET "XXXX"
#define NETATMO_CLIENT_ID "XXXX"
#endif // CONFIG_H
```

*Note: `include/configuration.h` is excluded from version control by `.gitignore`.*

### PlatformIO Project Setup

The project uses PlatformIO for ESP32-S3 development. The `platformio.ini` is configured for the **Freenove ESP32-S3-WROOM** board.

#### Controller Specifications
- **Board:** Freenove ESP32-S3-WROOM
- **Chip:** ESP32-S3 (Dual-core, WiFi + Bluetooth)
- **Memory:** Enabled PSRAM support (8MB)
- **Flash Memory Mode:** QIO OPI (High performance)
- **Filesystem:** LittleFS (Used for storing credentials and local data)

#### Build Flags
Key build flags used in the project:
- `-D BOARD_HAS_PSRAM`; Enables PSRAM support.
- `-mfix-esp32-psram-cache-issue`
- `-DFIRWARE_VERSION="0.0.1-ota03"`
- `-DCONFIG_SPIRAM_SUPPORT`
- `-DELEGANTOTA_USE_ASYNC_WEBSERVER=1` ; OTA Web site 
- `-D CONFIG_ESP_COREDUMP_ENABLE=1` ; Core dump handling
- `-D CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=1`
- `-D CONFIG_ESP_COREDUMP_CHECKSUM=1`
- `-D CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=1`
- `-D ENABLE_NETATMO` ; If defined, the rederer task will call Netatmo APIs
- `-D ENABLE_OWM` ; If defined, the rederer task will call Open Weather Map APIs


#### Dependencies
The project relies on several external libraries:
- `ArduinoJson`: For parsing API responses.
- `ESPAsyncWebServer` & `AsyncTCP`: For the local web server.

## 2. Web API

The ESP32 runs an asynchronous web server for management and data access.

### Endpoints

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| **GET** | `/status` | Returns system status, memory usage, and Netatmo token state. |
| **GET** | `/logs` | Returns recent system logs in JSON format. |
| **GET** | `/groups` | Returns all light groups from the database. |
| **POST** | `/groups` | Create or update a light group in the local database. |
| **GET** | `/lights?groupId={id}` | Returns lights associated with the specified group ID. |
| **POST** | `/lights` | Create or update a light and its group association. |
| **GET** | `/switches?groupId={id}` | Returns switches associated with the specified group ID. |
| **POST** | `/switches` | Create or update a physical switch mapping. |
| **GET** | `/switches/check?uid={uid}` | Checks if a switch with the given UID already exists. |
| **POST** | `/switch_attribution/mode` | Enables or disables switch attribution mode. |
| **GET** | `/switch_attribution/data` | Returns the last received 433MHz switch data in attribution mode. |
| **GET** | `/render` | Triggers a new image rendering for the display. |
| **GET** | `/export_db` | Downloads the local SQLite database file. |
| **POST** | `/import_db` | Uploads and replaces the local SQLite database. |
| **GET** | `/crashes` | Downloads the core dump if a crash occurred. |

### Switch Attribution Mode
When **Switch Attribution Mode** is enabled via `POST /switch_attribution/mode`, the ESP32 will stop processing 433MHz signals normally (i.e., it won't trigger light events). Instead, it will store the UID of the last received signal. This UID can then be retrieved using `GET /switch_attribution/data` to facilitate mapping physical switches to light groups in a management interface.

Detailed API documentation is available in `openapi.yaml`.

## 3. Hardware Configuration (Pinout)

The project uses an Freenove ESP32-S3 WROOM with a 8MB of PSRAM with the following peripheral pinout:

| Peripheral | Component | Pin (ESP32-S3) | Notes |
| :--- | :--- | :--- | :--- |
| **Buzzer** | Passive Buzzer | `GPIO 1` | Uses `ezBuzzer` library. |
| **433MHz Receiver** | RX433 (Data Pin) | `GPIO 2` | Uses `RCSwitch` library. |

### 433MHz Receiver Connection
- **VCC**: 5V (or 3.3V depending on the module).
- **GND**: Ground.
- **Data**: Connected to `GPIO 2`.

### Buzzer Connection (3-Pin Module)
- **VCC**: 3.3V or 5V (from ESP32 or external source).
- **GND**: Ground.
- **I/O (Signal)**: Connected to `GPIO 1`.
- **Type**: Passive Buzzer (allows melody playback via `ezBuzzer`).

## Core dump handling
python .\espcoredump.py info_corefile -m <firmware.elf> --core <core_dump.bin>
or
```powershell
.\coredump.ps1 -IP 192.168.1.48 -FirmwareElf .\.pio\build\freenove_esp32_s3_wroom\firmware.elf > coredump.txt
```