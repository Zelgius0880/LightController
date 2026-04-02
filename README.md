# LightController

This project manages Philips Hue lights through an ESP32, integrating with Firebase Firestore for data synchronization and a Hue Bridge for device control. It includes a Python migration script to synchronize and transform data between Firestore databases.

## 1. Project Configuration

### .env Configuration (Migration Script)

The `.env` file is used by the `migration.py` script to authenticate with your source and destination Firebase projects and the Hue Bridge. Create a `.env` file in the root directory with the following variables:

```env
# SOURCE DATABASE (Old)
SOURCE_FIREBASE_EMAIL=your_email@example.com
SOURCE_FIREBASE_PASSWORD=your_password
SOURCE_FIREBASE_API_KEY=your_source_api_key
SOURCE_FIREBASE_PROJECT_ID=your_source_project_id

# DESTINATION DATABASE (New)
DEST_FIREBASE_EMAIL=your_email@example.com
DEST_FIREBASE_PASSWORD=your_password
DEST_FIREBASE_API_KEY=your_dest_api_key
DEST_FIREBASE_PROJECT_ID=your_dest_project_id

# HUE BRIDGE CONFIGURATION
HUE_BRIDGE_IP=192.168.1.xxx
HUE_BRIDGE_USERNAME=your_hue_username
HUE_BRIDGE_CLIENT_KEY=your_hue_client_key
```

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

## 2. Data Migration

The `migration.py` script is used to migrate data from one Firestore project to another, specifically handling the transformation of Philips Hue light IDs from API v1 to v2.

### Prerequisites
- Python 3.x
- Install dependencies: `pip install requests python-dotenv`

### Running the Migration
To migrate a specific collection (e.g., `groups` and its `items` subcollections):

```bash
py migration.py --collection groups
```

The script will:
1. Fetch current light data directly from the Hue Bridge API v2.
2. Recursively traverse the source collection.
3. Transform HUE LIGHT documents (mapping v1 IDs to v2 UUIDs, syncing names, and removing obsolete fields).
4. Write transformed documents to the destination.
5. Verify the migration and log results to `migration_verification.log`.

### Output Logs
- `id_mapping.log`: Tracks v1 ID to v2 UUID mappings.
- `missing_ids.log`: Logs light IDs found in Firestore that don't match any light on the Hue Bridge.
- `migration_verification.log`: Reports the success of document creation in the destination.

## 3. Hardware Configuration (Pinout)

The project uses an Freenove ESP32-S3 WROOM with a 8mb of PSRAM with the following peripheral pinout:

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

## 4 Proxying Firebase Client
You'll need to update a source file of the FirebaseClient library to run a proxy.
Why a proxy? I encounter some 400 issues during running the requests. As I haven't more info of what's was wrong, I needed to see how the requests are being made.

### How to proceed
- Edit the file `FirebaseClient/src/core/AsyncClient/AsyncClient.h`
- On line ~`935:20` rplace the following code:
```aiignore
sData->request.addRequestHeader(method, path, extras);
        sData->request.addHostHeader(sData->request.getHost(true, &sData->response.val[resns::location]).c_str());
```
with
```C++
#ifdef  PROXY_ADDRESSS
        const String fullUrl = url + path;
        sData->request.val[reqns::url] = PROXY_ADDRESSS;
        sData->request.addRequestHeader(method, path, extras);
        sData->request.addHostHeader(url);
#ifdef PROXY_PORT
        sData->request.port = PROXY_PORT;
#endif

#else
        sData->request.addRequestHeader(method, path, extras);
        sData->request.addHostHeader(sData->request.getHost(true, &sData->response.val[resns::location]).c_str());
#endif
```

- Now you jsut need to define the `PROXY_ADDRESSS` and `PROXY_PORT` macros .