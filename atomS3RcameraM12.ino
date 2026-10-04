// M5stack v2.1.4/3.3.9

#include "camera_pins.h"
#include <WiFi.h>
#include "esp_camera.h"

#include <WebSocketsClient_Generic.h>
#include <WebSocketsServer_Generic.h>
#include <SocketIOclient_Generic.h>

// #define USE_ATOMS3R_CAM
#define USE_ATOMS3R_M12

// #define STA_MODE
#define AP_MODE

const char* ssid     = "atomcam";
const char* password = "12345678";
// const char* ssid     = "TP-Link_CCC7";
// const char* password = "10596234";
WebSocketsServer webSocket = WebSocketsServer(8080);
int activeClientNum = -1; // Keep track of the active client ID

void webSocketEvent(uint8_t num, int type, uint8_t * payload, size_t length) {
    if(type == WStype_CONNECTED) {
        activeClientNum = num;
        Serial.printf("[%d] M5Core3 display client joined.\n", num);
    } else if(type == WStype_DISCONNECTED) {
        activeClientNum = -1;
        Serial.printf("[%d] Client left.\n", num);
    }
}

static camera_config_t camera_config = {
    .pin_pwdn     = PWDN_GPIO_NUM,
    .pin_reset    = RESET_GPIO_NUM,
    .pin_xclk     = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7       = Y9_GPIO_NUM,
    .pin_d6       = Y8_GPIO_NUM,
    .pin_d5       = Y7_GPIO_NUM,
    .pin_d4       = Y6_GPIO_NUM,
    .pin_d3       = Y5_GPIO_NUM,
    .pin_d2       = Y4_GPIO_NUM,
    .pin_d1       = Y3_GPIO_NUM,
    .pin_d0       = Y2_GPIO_NUM,

    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href  = HREF_GPIO_NUM,
    .pin_pclk  = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

#ifdef USE_ATOMS3R_CAM
    .pixel_format = PIXFORMAT_RGB565,
    .frame_size   = FRAMESIZE_QVGA,
#endif

#ifdef USE_ATOMS3R_M12
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size   = FRAMESIZE_QVGA,
#endif

    .jpeg_quality  = 12,
    .fb_count      = 2,
    .fb_location   = CAMERA_FB_IN_DRAM,
    .grab_mode     = CAMERA_GRAB_WHEN_EMPTY,
    .sccb_i2c_port = 0,
};

void setup()
{
    Serial.begin(115200);
    pinMode(POWER_GPIO_NUM, OUTPUT);
    digitalWrite(POWER_GPIO_NUM, LOW);
    delay(500);
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        Serial.println("Camera Init Fail");
        delay(1000);
        esp_restart();
    } else {
        Serial.println("Camera Init Success");
    }
    delay(100);
    sensor_t *s = esp_camera_sensor_get();
    s->set_hmirror(s, 1);

#ifdef STA_MODE

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    WiFi.setSleep(false);
    Serial.println("");

    Serial.print("Connecting to ");
    Serial.println(ssid);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#endif

#ifdef AP_MODE
    if (!WiFi.softAP(ssid, password)) {
        log_e("Soft AP creation failed.");
        while (1);
    }

    Serial.println("AP SSID:");
    Serial.println(ssid);
    Serial.println("AP PASSWORD:");
    Serial.println(password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
#endif

    // server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
}

void loop()
{
  
    webSocket.loop();

    // If an M5Core3 is bound, capture a hardware frame and deploy it immediately
    if (activeClientNum != -1) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Frame ingestion failed");
            return;
        }

        // Broadcast raw binary data to the active channel
        webSocket.sendBIN(activeClientNum, fb->buf, fb->len);
        
        esp_camera_fb_return(fb); // Instantly cycle memory allocations back to pool
        delay(40);                // Throttle down to a clean ~25FPS to maintain Wi-Fi stability
    }
}
