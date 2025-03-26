#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// WiFi credentials
const char* ssid = "kp11.free.pass(12345678)";
const char* password = "12345678";

// Yandex Tracker API credentials
const char* token = "y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA";
const char* org_id = "bpfofdlbl4usqgkdim8s";

// Issue IDs
const char* id_task[] = { 
    "679c8736ed61c22d6e28f9b2",
    "67b2fbce626b4074db2b4ec6"
};

// API Endpoints
const char* base_url = "https://api.tracker.yandex.net/v2/issues/";

// Function to send HTTPS request
void sendHttpRequest(const char* issue_id) {
    WiFiClientSecure client;
    client.setInsecure();  // Use if you don't have SSL certificate validation

    HTTPClient http;
    String url = String(base_url) + issue_id + "/transitions/start_progress/_execute";

    Serial.println("Sending request to: " + url);

    http.begin(client, url);
    http.addHeader("Authorization", String("OAuth ") + token);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Cloud-Org-Id", org_id);

    int httpResponseCode = http.POST("");  // Empty body

    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        Serial.println(http.getString()); // Print response
    } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }

    http.end();  // Close connection
}

// Setup WiFi connection
void setupWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi!");
}

void setup() {
    Serial.begin(115200);
    setupWiFi();

    // Send request for each task
    for (int i = 0; i < sizeof(id_task) / sizeof(id_task[0]); i++) {
        sendHttpRequest(id_task[i]);
        delay(100);  // Avoid spamming requests
    }
}

void loop() {
    // Nothing needed here, request is sent in setup()
}
