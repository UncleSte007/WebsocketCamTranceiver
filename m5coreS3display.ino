// M5stack v2.1.4/3.3.9
// M5Unified 0.2.20
// WebSocketGeneric 2.14.1

#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "FS.h"
#include "SPI.h"
#include "SD.h"
#include <M5Unified.h>

#define SD_SPI_SCK_PIN 36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN 4

const char* ssid = "atomcam";
const char* password = "12345678";
// const char* server_url = "ws://192.168.1.145:8080/";
// const char* ssid     = "TP-Link_CCC7";
// const char* password = "10596234";
const char* server_url = "ws://192.168.4.1:8080/";
using namespace websockets;
WebsocketsClient client;
FILE* fptr;
int fileCounter = 0;
WebsocketsMessage message;

void onMessageCallback(WebsocketsMessage message) {
  if (message.isBinary()) {
    // CoreS3 uses M5Unified to draw compressed JPEG directly from a byte buffer to the screen
    M5.Display.drawJpg((uint8_t*)message.c_str(), message.length(), 0, 0);
    

    // M5.delay(10);
    M5.update();
    auto count = M5.Touch.getCount();
    if (count > 0) {
    //   Serial.println(count);
      auto touch = M5.Touch.getDetail();
      if (touch.wasClicked()) {
        // Serial.println("touched!");
        printf("used size before:\t %llu B\n", SD.usedBytes());
        char filename[255];
        sprintf(filename, "/sd/WSfile%03d.jpg", fileCounter);
        fptr = fopen(filename, "w");
        if (!fptr) {
            Serial.println("file open failed");
            }
        fwrite(message.c_str(), sizeof(uint8_t), message.length(), fptr);
        printf("file %s written\n", filename);
        fclose(fptr);

        fileCounter++;
        printf("used size after:\t %llu B\n", SD.usedBytes());
      }
    }
  }
}
void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setTextSize(2);
  M5.Display.print("Connecting to WiFi...");
  M5.Touch.begin(&M5.Display);

  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
    M5.Display.println("Card failed, or not present");
    while (1)
      ;
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  printf("SD Card Size: %lluMB\n", cardSize);
  // SD.end();


  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    M5.Display.print(".");
  }

  M5.Display.fillScreen(BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.println("WiFi Connected!\nConnecting to Server...");

  // Setup network callbacks
  client.onMessage(onMessageCallback);

  // Connect to your WebSocket/Streaming server
  while (!client.connect(server_url)) {
    M5.Display.print(".");
    delay(1000);
  }

  M5.Display.fillScreen(BLACK);
}

void loop() {
  delay(2);


  if (client.available()) {
    client.poll();  // Keep processing incoming frames
    // Serial.println("client available!");
  } else {
    M5.Display.setCursor(0, 0);
    M5.Display.println("Connection Lost. Reconnecting...");
    client.connect(server_url);
    delay(2000);
  }
}
