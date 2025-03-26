#include <Wire.h>
#include <HTTPClient.h>

#define TCA9548A_ADDRESS 0x70  // Default I2C address of TCA9548A
#define RESTART_BTN_PIN 4
#define NUM_STATIONS 3
#define NUM_TASKS 2

char red_robot_id[6] = "12876";
char blue_robot_id[6] = "64780";

int center_station_addres = 1;

int red_task_count = 0;
int blue_task_count = 0;

unsigned long game_start_time;
const unsigned long duration = 100000;

enum GameState { NONE,TASKS,FLAG};
GameState game_state = NONE;

// Define station states
enum StationState : uint8_t { OFF, COLLECT, DROP, ACTIVATOR, KING, WIN };

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
  char robot_id[6];
  char robot_color;
};
#pragma pack()

Datasend_data send_data;

struct DataPacketActivation {
  bool station_activated;
  char who_activated;
};

DataPacketActivation activation_data;

const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";
const char* server_url = "http://yourserver.com/api/data";

String token = "y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA";
String org_id = "bpfofdlbl4usqgkdim8s";

TaskHandle_t httpTaskHandle = NULL;

#define BUFFER_SIZE 5
String requestBuffer[BUFFER_SIZE];
int bufferStart = 0;
int bufferEnd = 0;
bool bufferFull = false;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(RESTART_BTN_PIN, INPUT_PULLUP);
  connect_wifi();
  
  xTaskCreatePinnedToCore(
    http_request,  // Task function
    "HTTP_Task",      // Task name
    8192,             // Stack size (in bytes)
    NULL,             // Task parameter
    1,                // Priority
    &httpTaskHandle,  // Task handle
    0                 // Core 0
  );
}

void connect_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
}

void push_buffer(String requestData) {
  if (bufferFull) {
    Serial.println("Buffer is full, dropping request!");
    return;
  }

  requestBuffer[bufferEnd] = requestData;
  bufferEnd = (bufferEnd + 1) % BUFFER_SIZE;

  if (bufferEnd == bufferStart) {
    bufferFull = true;
  }
}

bool get_buffer(String &requestData) {
  if (bufferStart == bufferEnd && !bufferFull) {
    return false;  // Buffer is empty
  }

  requestData = requestBuffer[bufferStart];
  bufferStart = (bufferStart + 1) % BUFFER_SIZE;
  bufferFull = false;
  return true;
}

void http_request(void* parameter) {
  for (;;) {  // Infinite loop for the task
    String requestData;
    
    if (getFromBuffer(requestData)) {
      Serial.println("Sending HTTP request: " + requestData);

      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(server_url);
        http.addHeader("Content-Type", "application/json");
        
        int httpResponseCode = http.POST(requestData);
        if (httpResponseCode > 0) {
          Serial.println("HTTP Response code: " + String(httpResponseCode));
        } else {
          Serial.println("HTTP Request failed: " + http.errorToString(httpResponseCode));
        }
        http.end();
      } else {
        Serial.println("WiFi not connected, cannot send request.");
      }
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Check buffer every second
  }
}

/*
  void get_slave_send_data() {
  for (uint8_t i = 1; i <= NUM_STATIONS; i++) {
    uint8_t bytes_requested = 8;
    Wire.requestFrom(i, bytes_requested);
    if (Wire.available()) {
      Serial.println("send_data from Slave ");
      while (Wire.available()) {
        Serial.print((char)Wire.read());
      }
      Serial.println();
    } else {
      Serial.println("No send_data from Slave ");
    }
  }
  }
*/

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

    Wire.requestFrom(2, sizeof(activation_data));

    if (Wire.available() == sizeof(activation_data)) {
      Wire.readBytes((byte*)&activation_data, sizeof(activation_data));

      Serial.print("station activated: ");
      Serial.println(activation_data.station_activated);
      Serial.print("who activated: ");
      Serial.println(activation_data.who_activated);
    } else {
      Serial.println("Error");
      return;
    }

    if (!activation_data.station_activated) {
      continue;
    }

    if (state == COLLECT) {
      for (int j = 0; j < NUM_TASKS; j++) {
        if (all_tasks[j].station_address == address) {
          all_tasks[j].activated = true;
          // !!! send request
        }
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
