#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <WebSocketsServer.h>

#define PIN_LED 0

struct color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t w;
};

ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
File fsUploadFile;                                    // a File variable to temporarily store the received file

const char *deviceName = "lampe";           // A name and a password for the OTA service
const char *OTAPassword = "";
unsigned long delayTime = 0;
unsigned long prevMillis = millis();
bool initComplete = false;
uint8_t brightness = 0;
uint8_t pos = 0;
uint8_t functionSelector = 0;
uint16_t step = 0;
color color = {0, 0, 0, 255};

Adafruit_NeoPixel strip = Adafruit_NeoPixel(18, PIN_LED, NEO_GRBW + NEO_KHZ800);

/*__________________________________________________________SETUP__________________________________________________________*/

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("\r\r\n");
  initFunction();
}

/*__________________________________________________________LOOP__________________________________________________________*/



void loop() {
  wifiMulti.run();
  webSocket.loop();
  server.handleClient();
  ArduinoOTA.handle();
  doStep();
}


void dimUp(uint16_t ms) {
  if (prevMillis + ms <= millis() && brightness < 100) {
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color((color.r * brightness / 100), (color.g * brightness / 100), (color.b * brightness / 100), (color.w * brightness / 100)));
    }
    brightness++;
    prevMillis = millis();
    Serial.printf("brightness: %i ", brightness);
    Serial.printf("r:%i, g:%i, b:%i, w:%i\r\n", (color.r * brightness / 100), (color.g * brightness / 100), (color.b * brightness / 100), (color.w * brightness / 100));
    strip.show();
  }
}


void rotate(uint16_t ms, bool trail = false) {
  if (prevMillis + ms <= millis()) {
    pos = (pos + 1) % strip.numPixels();
    if (trail) {
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0);
      }
    }
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, 0);
    }
    strip.setPixelColor(pos, strip.Color(color.r, color.g, color.b, color.w));
    Serial.printf("Pos: %i\r\r\n", pos);
    strip.show();
    prevMillis = millis();
  }
}
void rainbowCycle(uint8_t ms) {
  if (prevMillis + ms <= millis()) {
    uint16_t i;
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, wheel(((i * 256 / strip.numPixels()) + step) & 255));
    }
    strip.show();
    step++;
    prevMillis = millis();
  }
}

void rainbow(uint8_t ms) {
  if (prevMillis + ms <= millis()) {
    uint16_t i;
  
    for (i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, wheel((i + step) & 255));
    }
    strip.show();
    step++;
    step %= 300;
    prevMillis = millis();
  }
}

uint32_t wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void doStep() {
  switch (functionSelector) {
    case 0:
      break;
    case 1:
      color = {255, 0, 0, 0};
      rotate(250);
      break;
    case 2:
      wheel(pos);
      rotate(50);
      break;
    case 3:
        rainbowCycle(30);
      break;
    case 4:
        rainbow(75);
      break;
  }
}


void initFunction() {
  Serial.println("init");
  strip.begin();
  wifiMulti.addAP("example WiFi", "Example Password123"); // add Wi-Fi networks you want to connect to
  initOTA();
  SPIFFS.begin();

  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);

  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  server.begin();

  MDNS.begin(deviceName);
  brightness = 0;
  while (brightness < 100) {
    dimUp(30);
  }
  while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(250);
    Serial.print('.');
  }
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());             // Tell us what network we're connected to
  Serial.print("IP address:\t");
  Serial.print(WiFi.localIP());
}

void initOTA() { // Start the OTA service
  ArduinoOTA.setHostname(deviceName);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
      Serial.println("sketch");
    }
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, 0);
    }
    strip.show();
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\r\nEnd");
    for (int rep = 0; rep < 3; rep++) {
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, 0);
      }
      strip.show();
      delay(300);
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 127, 0, 0));
      }
      strip.show();
      delay(300);
    }
    for (int i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, 0);
    }
    strip.show();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    uint8_t percent = (progress / (total / 100));
    Serial.printf("Progress: %u%%\r", percent);
    uint32_t color = strip.Color(255 - (percent * 2.55), 0 + (percent * 2.55), 0, 0);
    for (int i = 0; i < 3 * percent; i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}


/*__________________________________________________________SERVER_HANDLERS__________________________________________________________*/

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\r\n", num);
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        char response [10];
        snprintf(response, 10, "#%02X%02X%02X%02X", color.r, color.g, color.b, color.w);
        Serial.printf("r:%i, g:%i, b:%i, w:%i\r\n", color.r, color.g, color.b, color.w);
        webSocket.sendTXT(num, response);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
    case WStype_TEXT:                     // if new text data is received
      Serial.printf("[%u] get Text: %s\r\n", num, payload);
      if (payload[0] == '#') {            // we get RGB data
        uint32_t rgb = (uint32_t) strtoul((const char *) &payload[1], NULL, 16);   // decode rgb data
        color.r = (rgb >> 24) & 0xFF;
        color.g = (rgb >> 16) & 0xFF;
        color.b = (rgb >> 8) & 0xFF;
        color.w = rgb & 0xFF;
        functionSelector = 0;
        Serial.printf("r:%i, g:%i, b:%i, w:%i\r\n", color.r, color.g, color.b, color.w);
        setColor(strip.Color(color.r, color.g, color.b, color.w));
        strip.show();
        webSocket.broadcastTXT(payload, lenght);
      }
      else {
        switch(payload[0]) {
          case '1':
              functionSelector = 1;
            break;
          case '2':
              functionSelector = 2;
            break;
          case '3':
              functionSelector = 3;
            break;
          case '4':
              functionSelector = 4;
            break;
          default:
              functionSelector = 0;
        }
      }
      break;
    default:
      Serial.printf("Default: num=%u, type=%s, payload=%s, length=%i", num, type, payload, lenght);
      break;
  }
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

void setHue(int hue) { // Set the RGB LED to a given hue (color) (0° = Red, 120° = Green, 240° = Blue)
  hue %= 360;                   // hue is an angle between 0 and 359°
  float radH = hue * 3.142 / 180; // Convert degrees to radians
  float rf, gf, bf;

  if (hue >= 0 && hue < 120) {  // Convert from HSI color space to RGB
    rf = cos(radH * 3 / 4);
    gf = sin(radH * 3 / 4);
    bf = 0;
  } else if (hue >= 120 && hue < 240) {
    radH -= 2.09439;
    gf = cos(radH * 3 / 4);
    bf = sin(radH * 3 / 4);
    rf = 0;
  } else if (hue >= 240 && hue < 360) {
    radH -= 4.188787;
    bf = cos(radH * 3 / 4);
    rf = sin(radH * 3 / 4);
    gf = 0;
  }
  color.r = rf * rf * 255;
  color.g = gf * gf * 255;
  color.b = bf * bf * 255;
  color.w = 0;
  setColor(strip.Color(color.r, color.g, color.b, color.w));
  strip.show();
}

void setColor(uint32_t color) {
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
}
