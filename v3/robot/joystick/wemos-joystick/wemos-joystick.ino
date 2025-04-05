#include <WiFi.h>
#include <HTTPClient.h>
//#include <ESP8266WiFi.h>
//#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>  // Include the ArduinoJson library

#define LED_PIN 3   // Define the GPIO pin connected to the NeoPixel
#define NUM_LEDS 1  // Define the number of LEDs in the strip

#define WEAPON_PIN 2

// WiFi credentials
const char* ssid = "Mayya";           // Replace with your WiFi SSID
const char* password = "mayya_1234";  // Replace with your WiFi password

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 500  // Time (in milliseconds) to pause between pixels

String token = "y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA";
String org_id = "bpfofdlbl4usqgkdim8s";

bool red_robot = 1;
String task_id;

String red_task_id = "67b2fc17626b4074db2b4f4b";
String blue_task_id = "67e69ed22d2ae1712cc18426";

WiFiClientSecure client;  // Declare WiFiClient
HTTPClient http;          // Declare HTTPClient

uint32_t last_weapon_activated = 0;
bool weapon_active = 0;

const uint32_t COLOR_ORANGE = strip.Color(255, 125, 14);
const uint32_t COLOR_GREEN = strip.Color(0, 255, 0);

void connect_wifi() {
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");                             
    strip.setPixelColor(0, COLOR_ORANGE);  
    strip.show();                                  
    delay(DELAYVAL);

    strip.clear();
    strip.show();                                 
    delay(DELAYVAL);
  }

  client.setInsecure();
  Serial.println("\nWiFi connected"); 

  strip.setPixelColor(0, COLOR_GREEN);
  strip.show();
}

void setup() {
  pinMode(WEAPON_PIN, OUTPUT);
  Serial.begin(115200);  //Initialize serial communication for debugging

  strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.setBrightness(80);  // Set BRIGHTNESS to about 1/5 (max = 255)
  strip.show();

  connect_wifi();

  task_id = red_robot ? red_task_id : blue_task_id;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected - attempting to reconnect");
    connect_wifi();
  }

  if (millis() - last_weapon_activated > 5000) {
    digitalWrite(WEAPON_PIN, LOW);
    if (weapon_active) {
      Serial.println("Deactivating");
      weapon_active = false;
      change_state();
    }
    Serial.println("Checking status");
    checkStatus();
  } else if (weapon_active) {
    Serial.println("Weapon is active");
    digitalWrite(WEAPON_PIN, HIGH);
  }
  delay(500);
}

void change_state() {

  String url = "https://api.tracker.yandex.net/v2/issues/" + task_id + "/transitions/stop_progress/_execute";

  http.begin(client, url);
  http.addHeader("Authorization", "OAuth " + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Cloud-Org-Id", org_id);

  int httpCode = http.POST("");

  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(httpCode);
    //StaticJsonDocument<1024> doc;
    //DeserializationError error = deserializeJson(doc, payload);

    //if (!error) {
    //  const char* statusKey = doc["status"]["key"];
    //  if (strcmp(statusKey, "need_info") == 0){
    //    changeState();
    //    delay(3000);
    //  }
    //}
  }
  http.end();
}


void checkStatus() {
  String url = "https://api.tracker.yandex.net/v2/issues/" + task_id;

  http.begin(client, url);
  http.addHeader("Authorization", "OAuth " + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Cloud-Org-Id", org_id);

  int httpCode = http.GET();

  Serial.print("HTTP Code: ");  // Serial print for debugging
  Serial.println(httpCode);     // Serial print for debugging

  if (httpCode > 0) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    Serial.println(payload);

    if (!error) {
      const char* statusKey = doc["status"]["key"];
      if (strcmp(statusKey, "inProgress") == 0) {
        last_weapon_activated = millis();
        weapon_active = true;
      }
    }
  }
  http.end();
}
