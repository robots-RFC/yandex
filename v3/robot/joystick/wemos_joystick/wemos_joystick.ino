#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#define LED_PIN 6
#define NUM_LED 1
Adafruit_NeoPixel led = Adafruit_NeoPixel(NUM_LED, LED_PIN, NEO_GRB + NEO_KHZ800);

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const uint32_t COLOR_GREEN = led.Color(0, 255, 0);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    led.setPixelColor(0, led_color);
    led.show();
    delay(250);
    Serial.print(".");
    led.clear();
    led.show();
    delay(250);
  }
  Serial.println("\nConnected to WiFi!");
  led.setPixelColor(0, led_color);
  led.show();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {  // Check Wi-Fi connection
    HTTPClient http;

    // Example GET request to httpbin.org
    http.begin("http://httpbin.org/get");  // Specify request URL
    int httpCode = http.GET();             // Send the GET request

    if (httpCode > 0) {  // Check for a response
      String payload = http.getString();  // Get the response payload
      Serial.println("Response:");
      Serial.println(httpCode);  // HTTP status code (e.g., 200)
      Serial.println(payload);   // Response body
    } else {
      Serial.println("Error on HTTP request: " + String(httpCode));
    }

    http.end();  // Close connection
  }

  delay(500);  // Wait 5 seconds before next request
}