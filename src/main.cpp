#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "ApiClient.h"

const char* ssid = "Home";
const char* password = "adm1806*";

AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 Image Upload</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial;text-align:center;margin:40px;}
img{max-width:90%;height:auto;border:1px solid #ccc;margin-top:20px;}
</style>
</head>
<body>

<h2>Upload PNG Image</h2>

<form method="POST" action="/upload" enctype="multipart/form-data">
<input type="file" name="image" accept="image/png">
<input type="submit" value="Upload">
</form>

<h2>Stored Image</h2>
<img src="/image.png">

</body>
</html>
)rawliteral";

ApiClient api("jsonplaceholder.typicode.com", 443, true);


void setup() {

    Serial.begin(9600);

    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    WiFi.begin(ssid, password);
    Serial.print("Connecting");

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.on("/image.png", HTTP_GET, [](AsyncWebServerRequest *request){
        if(SPIFFS.exists("/image.png"))
            request->send(SPIFFS, "/image.png", "image/png");
        else
            request->send(404, "text/plain", "No image uploaded");
    });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        request->redirect("/");
    }, [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final){

        static File file;

        if(!index){
            Serial.printf("UploadStart: %s\n", filename.c_str());
            file = SPIFFS.open("/image.png", FILE_WRITE);
        }

        if(file){
            file.write(data, len);
        }

        if(final){
            if(file) file.close();
            Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
        }
    });

    server.begin();

    ApiResponse r = api.get("/todos/1");

    Serial.println("Status:");
    Serial.println(r.status);

    Serial.println("Body:");
    Serial.println(r.body);
}

void loop() {}