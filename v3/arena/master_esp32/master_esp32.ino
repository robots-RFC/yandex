#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#define BUFFER_SIZE 5
struct Request {
  String url;
  StationState state;
};

Request requestBuffer[BUFFER_SIZE];
int bufferCount = 0;

// WiFi credentials
const char* ssid = "pozzzitron1";
const char* password = "12345678";

// Yandex Tracker API credentials
const char* token = "y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA";
const char* org_id = "bpfofdlbl4usqgkdim8s";

#define TCA9548A_ADDRESS 0x70  // Default I2C address of TCA9548A
#define RESTART_BTN_PIN 4
#define NUM_STATIONS 3
#define NUM_TASKS 2

char red_robot_id[6] = "12876";
char blue_robot_id[6] = "64780";

const char* base_url = "http://api.tracker.yandex.net/v2/issues/";
TaskHandle_t httpTaskHandle = NULL;

int center_station_addres = 1;

int red_task_count = 0;
int blue_task_count = 0;

unsigned long game_start_time;
const unsigned long duration = 100000;

enum GameState { NONE,
                 TASKS,
                 FLAG };
GameState game_state = NONE;

// Define station states
enum StationState : uint8_t { OFF,
                              COLLECT,
                              DROP,
                              ACTIVATOR,
                              KING,
                              WIN };

struct Tasks {
  String task_id;
  int station_address;
  char robot_color;
  bool activated;
  bool complete;
};

Tasks all_tasks[NUM_TASKS] = {
  { "679c8736ed61c22d6e28f9b2", 1, 'r', false, false },
  { "679c8736ed61c22d6e28f9b2", 2, 'b', false, false },
};

struct Station {
  int address;
  StationState state;
};

Station all_stations[NUM_STATIONS] = {
  { 1, COLLECT },
  { 2, COLLECT },
  { 3, DROP },
};

#pragma pack(1)
struct Datasend_data {
  StationState state;
  char robot_id[9];
  char robot_color;
};
#pragma pack()

Datasend_data send_data;

struct DataPacketActivation {
  bool station_activated;
  char who_activated;
};

DataPacketActivation activation_data;

void connect_wifi() {
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
  Wire.begin();
  pinMode(RESTART_BTN_PIN, INPUT_PULLUP);
  connect_wifi();

  xTaskCreatePinnedToCore(
    http_request,     // Task function
    "HTTP_Task",      // Task name
    16384,            // Stack size (in bytes)
    NULL,             // Task parameter
    1,                // Priority
    &httpTaskHandle,  // Task handle
    0                 // Core 0
  );
}

bool get_buffer(String& requestData) {
  if (bufferStart == bufferEnd && !bufferFull) {
    //Serial.println("Buffer is empty");
    return false;  // Buffer is empty
  }

  requestData = requestBuffer[bufferStart];
  bufferStart = (bufferStart + 1) % BUFFER_SIZE;
  bufferFull = false;
  return true;
}

void sendHttpRequest(String url, StationState state) {
  WiFiClient client;
  HTTPClient http;

  Serial.println("Sending request to: " + url);

  http.begin(client, url);
  http.addHeader("Authorization", String("OAuth ") + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Cloud-Org-Id", org_id);

  String payload;

  if (state == DROP){
    payload = "{\"resolution\":\"fixed\"}";
  } else {
    payload = "";
  }

  int httpResponseCode = http.POST(payload);  // Empty body

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());  // Print response
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();  // Close connection
}

void pop_buffer() {
  // Shift all remaining elements forward
  for (int i = 1; i < bufferCount; i++) {
    requestBuffer[i-1] = requestBuffer[i];
  }
  
  bufferCount--;
}

void http_request(void* parameter) {
  Serial.println("http");
  for (;;) {  // Infinite loop for the task
    if (bufferCount == 0){
      continue;
    }

    String url = requestBuffer.url;
    StationState state = requestBuffer.state;

    pop_buffer();

    sendHttpRequest(url, state);

    vTaskDelay(100 / portTICK_PERIOD_MS);  // Check buffer every second
  }
}

bool push_buffer(StationState state, String task_id, String place) {
  if (bufferCount == BUFFER_SIZE) {
    Serial.println("Buffer is full, dropping request!");
    return false;
  }

  Request newRequest;
  newRequest.url = String(base_url) + task_id + "/transitions/" + place + "/_execute";
  newRequest.state = state;

  requestBuffer[bufferCount] = newRequest;
  bufferCount++;
}


void push_buffer(String place, String task_id) {
  if (bufferFull) {
    Serial.println("Buffer is full, dropping request!");
    return;
  }

  String url = String(base_url) + task_id + "/transitions/" + place + "/_execute";

  requestBuffer[bufferEnd] = url;
  bufferEnd = (bufferEnd + 1) % BUFFER_SIZE;

  if (bufferEnd == bufferStart) {
    bufferFull = true;
  }
}

void send_station_state() {
  for (int i = 0; i < NUM_STATIONS; i++) {
    int address = all_stations[i].address;
    StationState state = all_stations[i].state;
    send_data.state = state;

    if (state == DROP) {
      strcpy(send_data.robot_id, red_robot_id);
      send_data.robot_color = 'r';
    } else if (state == COLLECT) {
      for (int j = 0; j < NUM_TASKS; j++) {
        if (all_tasks[j].station_address == address) {
          char robot_color = all_tasks[j].robot_color;
          strcpy(send_data.robot_id, (robot_color == 'r') ? red_robot_id : blue_robot_id);
          send_data.robot_color = robot_color;
          break;
        }
      }
    } else if (state == WIN) {
      strcpy(send_data.robot_id, "");

      if (red_task_count > blue_task_count)
        send_data.robot_color = 'r';
      else if (red_task_count < blue_task_count)
        send_data.robot_color = 'b';
      else
        send_data.robot_color = 'g';
    }

    Wire.beginTransmission(address);
    Wire.write((uint8_t*)&send_data, sizeof(send_data));  // Send struct as bytes
    Wire.endTransmission();
  }
}

void get_station_info() {
  for (int i = 0; i < NUM_STATIONS; i++) {
    StationState state = all_stations[i].state;
    int address = all_stations[i].address;

    Wire.requestFrom(address, sizeof(activation_data));

    if (Wire.available() == sizeof(activation_data)) {
      Wire.readBytes((byte*)&activation_data, sizeof(activation_data));

      Serial.print("station activated: ");
      Serial.println(activation_data.station_activated);
      Serial.print("who activated: ");
      Serial.println(activation_data.who_activated);
    } else {
      Serial.println("Error");
      continue;
    }

    if (!activation_data.station_activated) {
      continue;
    }

    if (state == COLLECT) {
      for (int j = 0; j < NUM_TASKS; j++) {
        if (all_tasks[j].station_address == address) {
          all_tasks[j].activated = true;
          String task_id = all_tasks[j].task_id;
          push_buffer("start_progress", task_id);
          while (true) {
          }
        }
      }
    } else if (state == DROP) {
      push_buffer("close", "679c8736ed61c22d6e28f9b2");
      // turn of station
      while (true) {
      }
    }
  }
}

void change_state_win() {
  for (int i = 0; i < NUM_STATIONS; i++) {
    all_stations[i].state = WIN;
  }
}

void loop() {
  if (!digitalRead(RESTART_BTN_PIN) && game_state == NONE) {
    Serial.println("Game Started!");
    send_station_state();
    game_state = TASKS;

    game_start_time = millis();
    red_task_count = 0;
    blue_task_count = 0;
  }

  if (game_state == TASKS) {
    if (millis() - game_start_time < duration) {
      get_station_info();
    } else {
      Serial.println("Game Over!");
      change_state_win();
      send_station_state();
      game_state = NONE;
    }
  }

  delay(500);  // Reduce CPU load
}
