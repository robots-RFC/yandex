#include <Wire.h>
#include <esp_system.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Define station states
enum StationState : uint8_t { OFF,
                              COLLECT,
                              DROP,
                              ACTIVATOR,
                              KING,
                              WIN,
                              SWITCH };

String text_station_state(StationState state) {
  if (state == OFF)
    return "OFF";
  else if (state == COLLECT)
    return "COLLECT";
  else if (state == ACTIVATOR)
    return "ACTIVATOR";
  else if (state == KING)
    return "KING";
  else if (state == WIN)
    return "WIN";
  else if (state == DROP)
    return "DROP";
  else if (state == SWITCH){
    return "SWITCH";
  }
}

#define BUFFER_SIZE 5
struct Request {
  String task_id;
  StationState state;
};

Request http_buffer[BUFFER_SIZE];
int http_head = -1;
int http_tail = -1;
int http_buffer_count = 0;



// WiFi credentials
//const char* ssid = "pozzzitron1";
const char* ssid = "Mayya";
//const char* password = "12345678";
const char* password = "mayya_1234";

// Yandex Tracker API credentials
String token = "y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA";
String org_id = "bpfofdlbl4usqgkdim8s";

WiFiClientSecure client;
HTTPClient http;

#define TCA9548A_ADDRESS 0x70  // Default I2C address of TCA9548A
#define RESTART_BTN_PIN 4

const int num_stations = 4;
const int num_tasks = num_stations - 1;

char red_robot_id[6] = "12876";
char blue_robot_id[6] = "64780";

const char* base_url = "http://api.tracker.yandex.net/v2/issues/";
TaskHandle_t httpTaskHandle = NULL;

int center_station_address = 1;

int red_task_count = 0;
int blue_task_count = 0;

unsigned long game_start_time;
const unsigned long duration = 5000;
const unsigned long duration_2 = 5000;

enum GameState { NONE,
                 TASKS,
                 FLAG,
                 FINISH,
                 SWAP };
GameState game_state = NONE;



struct Tasks {
  String task_id;
  int station_address;
  char robot_color;
  bool activated;
  bool completed;
};

Tasks all_tasks[num_tasks] = {
  { "679c8736ed61c22d6e28f9b2", 2, 'r', false, false },
  { "67b2fbce626b4074db2b4ec6", 3, 'b', false, false },
  { "67b2fd1f95b6044ad3a4ddcc", 4, 'b', false, false },
};

struct Activators {
  String task_id;
  int station_address;
  char robot_color;
};

Activators all_activators[2] = {
  { "67b2fc17626b4074db2b4f4b", -1, 'r' },
  { "67e69ed22d2ae1712cc18426", -1, 'b' }
};

struct Station {
  int address;
  StationState state;
};

Station all_stations[num_stations];

#pragma pack(1)
struct Datasend_data {
  StationState state;
  char robot_id[9];
  char robot_color;
};
#pragma pack()

struct DataPacketActivation {
  bool station_activated;
  char who_activated;
};

DataPacketActivation activation_data;



void connect_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  client.setInsecure();
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

int buffer_ring(int ptr) {
  ptr++;
  if (ptr > (BUFFER_SIZE - 1))
    ptr = 0;
  return ptr;
}

void buffer_clear() {
  http_head = -1;
  http_tail = -1;
  http_buffer_count = 0;
}

void push_http_buffer(StationState state, String task_id) {
  Serial.print("Pushing to buffer - ");
  Serial.println(text_station_state(state));
  if (http_buffer_count < BUFFER_SIZE) {
    Request new_request;

    new_request.state = state;
    new_request.task_id = task_id;

    http_buffer_count++;
    if (http_buffer_count == 1) {
      http_head = 0;
    }
    http_tail = buffer_ring(http_tail);
    http_buffer[http_tail] = new_request;

    Serial.print("test ");
    Serial.print("head ");
    Serial.print(http_head);
    Serial.print(" tail ");
    Serial.print(http_tail);
    Serial.println(text_station_state(http_buffer[http_head].state));

  } else {
    Serial.println("Queue overflow");
  }
}

void pop_http_buffer() {
  if (http_buffer_count > 0) {
    http_buffer_count--;
    if (http_buffer_count > 0) {
      http_head = buffer_ring(http_head);
    } else {
      http_head = -1;
      http_tail = -1;
    }
  } else {
    Serial.println("Queue is empty");
  }
}

void sendHttpRequest(StationState state, String task_id) {
  String url = "https://api.tracker.yandex.net/v2/issues/" + task_id;

  switch (state) {
    case COLLECT:
      url += "/transitions/start_progress/_execute";
      break;
    case DROP:
      url += "/transitions/close/_execute";
      break;
  }

  Serial.println("Sending request to: " + url);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }

  http.begin(client, url);
  http.addHeader("Authorization", "OAuth " + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Cloud-Org-Id", org_id);

  int httpResponseCode = -1;

  switch (state) {
    case COLLECT:
      Serial.println("Sending COLLECT request...");
      httpResponseCode = http.POST("");
      break;
    case DROP:
      Serial.println("Sending DROP request...");
      httpResponseCode = http.POST("{\"resolution\":\"fixed\"}");
      break;
    case ACTIVATOR:
      Serial.println("Sending ACTIVATOR request...");
      httpResponseCode = http.PATCH("{\"tags\": {\"remove\" : [\"hide-task\"] }");
      break;
  }

  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    Serial.println("Server Response:");
    Serial.println(http.getString());
  } else {
    Serial.println("Failed to send request! Check URL, WiFi, and API keys.");
  }

  http.end();
}

/*
void sendHttpRequest(StationState state, String task_id) {
  //String url = String(base_url) + task_id;

  String url = "https://api.tracker.yandex.net/v2/issues/" + task_id + "/transitions/start_progress/_execute";


  switch (state) {
    case COLLECT:
      url += "/transitions/start_progress/_execute";
      break;
    case DROP:
      url += "/transitions/close/_execute";
      break;
  }

  Serial.println("Sending request to: " + url);

  http.begin(client, url);
  http.addHeader("Authorization", "OAuth " + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Cloud-Org-Id", org_id);

  int httpResponseCode;

  switch (state) {
    case COLLECT:
      httpResponseCode = http.POST("");
      break;
    case DROP:
      httpResponseCode = http.POST("{\"resolution\":\"fixed\"}");
      break;
    case ACTIVATOR:
      httpResponseCode = http.PATCH("{\"tags\": {\"remove\" : [\"hide-task\"] }");
      break;
  }

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

*/


void http_request(void* parameter) {
  while (true) {                           // Infinite loop for the task
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Check buffer every second

    if (http_buffer_count == 0) {
      continue;
    }

    String task_id = http_buffer[http_head].task_id;
    StationState state = http_buffer[http_head].state;

    Serial.print("got for request -");
    Serial.println(text_station_state(state));
    pop_http_buffer();

    sendHttpRequest(state, task_id);
  }
}


void send_station_state(int address, StationState state) {
  Serial.print("sending state");
  Serial.print(address);
  Serial.print(" ");
  Serial.println(text_station_state(state));
  Datasend_data send_data;
  send_data.state = state;

  if (state == DROP) {
    strcpy(send_data.robot_id, red_robot_id);
    send_data.robot_color = 'r';
  } else if (state == COLLECT) {
    for (int j = 0; j < num_tasks; j++) {
      if (all_tasks[j].station_address == address) {
        char robot_color = all_tasks[j].robot_color;
        strcpy(send_data.robot_id, (robot_color == 'r') ? red_robot_id : blue_robot_id);
        send_data.robot_color = robot_color;
        break;
      }
    }
  } else if (state == WIN) {
    if (red_task_count > blue_task_count)
      send_data.robot_color = 'r';
    else if (red_task_count < blue_task_count)
      send_data.robot_color = 'b';
    else
      send_data.robot_color = 'g';
  } else if (state == ACTIVATOR) {
    for (int i = 0; i < 2; i++) {
      if (all_activators[i].station_address == address) {
        char robot_color = all_activators[i].robot_color;
        send_data.robot_color = robot_color;
      }
    }

  } else if (state == KING) {
    strcpy(send_data.robot_id, red_robot_id);
    send_data.robot_color = 'r';
  }

  Wire.beginTransmission(address);
  Wire.write((uint8_t*)&send_data, sizeof(send_data));  // Send struct as bytes
  Wire.endTransmission();
}

void get_tasks_info() {
  for (int i = 0; i < num_stations; i++) {
    StationState state = all_stations[i].state;
    int address = all_stations[i].address;

    if (state == OFF) {
      continue;
    }

    Wire.requestFrom(address, sizeof(activation_data));

    if (Wire.available() == sizeof(activation_data)) {
      Wire.readBytes((byte*)&activation_data, sizeof(activation_data));


    } else {
      Serial.println("Error");
      continue;
    }

    if (!activation_data.station_activated) {
      continue;
    }

    if (state == COLLECT) {
      for (int j = 0; j < num_tasks; j++) {
        if (all_tasks[j].station_address == address && !all_tasks[j].activated) {
          all_tasks[j].activated = true;
          String task_id = all_tasks[j].task_id;
          Serial.println("In COLLECT");
          push_http_buffer(state, task_id);
        }
      }
    } else if (state == DROP) {
      for (int j = 0; j < num_tasks; j++) {
        if (all_tasks[j].robot_color == activation_data.who_activated && all_tasks[j].activated && !all_tasks[j].completed) {
          int station_address = all_tasks[j].station_address;

          Serial.print("sending station state off - ");
          Serial.println(station_address);
          send_station_state(station_address, OFF);

          all_tasks[j].completed = true;
          String task_id = all_tasks[j].task_id;
          Serial.println("In DROP");
          push_http_buffer(state, task_id);

          for (int b = 0; b < num_stations; b++) {
            if (all_stations[b].address == station_address) {
              all_stations[b].state = OFF;
            }
          }

          if (all_tasks[j].robot_color == 'r') {
            red_task_count++;
          } else {
            blue_task_count++;
          }
          break;
        }
      }
    } else if (state == ACTIVATOR) {
      Serial.print("activator color - ");
      Serial.println(activation_data.who_activated);

      for (int j = 0; j < 2; j++) {
        if (all_activators[j].robot_color == activation_data.who_activated) {
          String task_id = all_activators[j].task_id;
          push_http_buffer(state, task_id);
          break;
        }
      }

    } else if (state == KING) {
      if (activation_data.who_activated == 'r') {
        red_task_count *= 2;
      } else {
        blue_task_count *= 2;
      }
    }
  }
}

void change_state_win() {
  for (int i = 0; i < num_stations; i++) {
    all_stations[i].state = WIN;
  }
}

void change_state_switch() {
  for (int i = 0; i < num_stations; i++) {
    all_stations[i].state = SWITCH;
  }
}

void send_state_all_stations() {
  for (int i = 0; i < num_stations; i++) {
    int address = all_stations[i].address;
    StationState state = all_stations[i].state;
    send_station_state(address, state);
  }
}

void set_task_states() {
  for (int i = 0; i < num_stations; i++) {
    int address = i + 1;
    StationState state;
    if (address == center_station_address) {
      state = DROP;
    } else {
      state = COLLECT;
    }

    all_stations[i] = { address, state };
  }
}

void set_flag_states() {
  for (int i = 0; i < num_stations; i++) {
    int address = i + 1;
    all_stations[i] = { address, OFF };
  }

  all_stations[0] = { center_station_address, KING };

  uint8_t activator_count = 0;
  while (activator_count < 2) {
    uint8_t randomStation = (esp_random() % (num_stations - 1)) + 1;

    if (all_stations[randomStation].state != ACTIVATOR) {
      all_stations[randomStation].state = ACTIVATOR;
      int address = all_stations[randomStation].address;
      all_activators[activator_count].station_address = address;
      activator_count++;
    }
  }
}

void loop() {
  if (!digitalRead(RESTART_BTN_PIN)) {
    set_task_states();
    send_state_all_stations();
    Serial.println("Game Started!");
    game_state = TASKS;

    game_start_time = millis();
    red_task_count = 0;
    blue_task_count = 0;
  }

  if (game_state == TASKS) {
    if (millis() - game_start_time < duration) {
      get_tasks_info();
    } else {
      Serial.println("Game Mode - SWAP");
      game_state = SWAP;
      change_state_switch();
      send_state_all_stations();
      game_start_time = millis();
    }
  } else if (game_state == SWAP) {
    if (millis() - game_start_time > 3000) {
      Serial.println("Game Mode - FLAG");
      game_state = FLAG;
      set_flag_states();
      send_state_all_stations();
      game_start_time = millis();
    }
  } else if (game_state == FLAG) {
    if (millis() - game_start_time < duration_2) {
      //get_tasks_info();
    } else {
      game_state = FINISH;
    }

  } else if (game_state == FINISH) {
    Serial.println("Game Over!");
    change_state_win();
    send_state_all_stations();
    game_state = NONE;
  }

  delay(500);  // Reduce CPU load
}