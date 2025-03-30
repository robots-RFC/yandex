#include <Wire.h>
#include <esp_system.h>
//#include <WiFi.h>
//#include <HTTPClient.h>
//#include <WiFiClientSecure.h>

// Define station states
enum StationState : uint8_t { OFF,
                              COLLECT,
                              DROP,
                              ACTIVATOR,
                              KING,
                              WIN };

#define BUFFER_SIZE 5
struct Request {
  String url;
  StationState state;
};

Request requestBuffer[BUFFER_SIZE];
int bufferCount = 0;

// WiFi credentials
//const char* ssid = "pozzzitron1";
const char* ssid = "Mayya";
//const char* password = "12345678";
const char* password = "mayya_1234";

// Yandex Tracker API credentials
const char* token = "y0__xCGzMSsAxip6zQg-OyIkxILFCnLzmZ85p9Vo_ez69Ec0_XiqA";
const char* org_id = "bpfofdlbl4usqgkdim8s";

#define TCA9548A_ADDRESS 0x70  // Default I2C address of TCA9548A
#define RESTART_BTN_PIN 4

const int num_stations = 4;
const int num_tasks = num_stations - 1;

char red_robot_id[6] = "12876";
char blue_robot_id[6] = "64780";

const char* base_url = "http://api.tracker.yandex.net/v2/issues/";
TaskHandle_t httpTaskHandle = NULL;

int center_station_addres = 1;

int red_task_count = 0;
int blue_task_count = 0;

unsigned long game_start_time;
const unsigned long duration = 1000 * 60;
const unsigned long duration_2 = 1000 * 60;

enum GameState { NONE,
                 TASKS,
                 FLAG,
                 WIN };
GameState game_state = NONE;



struct Tasks {
  String task_id;
  int station_address;
  char robot_color;
  bool activated;
  bool completed;
};

Tasks all_tasks[num_tasks] = {
  { "679c8736ed61c22d6e28f9b2", 2, 'b', false, false },
  { "679c8736ed61c22d6e28f9b2", 3, 'b', false, false },
  { "679c8736ed61c22d6e28f9b2", 4, 'b', false, false },
};

struct Activators {
  Strign task_id;
  int station_address;
  char robot_color;
};

Activators all_activators[2] = {
  { "", -1, 'r' },
  { "", -1, 'b' }
}

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

/*
void connect_wifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
}
*/

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(RESTART_BTN_PIN, INPUT_PULLUP);
  /*connect_wifi();

  xTaskCreatePinnedToCore(
    http_request,     // Task function
    "HTTP_Task",      // Task name
    16384,            // Stack size (in bytes)
    NULL,             // Task parameter
    1,                // Priority
    &httpTaskHandle,  // Task handle
    0                 // Core 0
  );
  */
}

/*
void sendHttpRequest(String url, StationState state) {
  WiFiClient client;
  HTTPClient http;

  Serial.println("Sending request to: " + url);

  http.begin(client, url);
  http.addHeader("Authorization", String("OAuth ") + token);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Cloud-Org-Id", org_id);

  String payload;

  if (state == DROP) {
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
    requestBuffer[i - 1] = requestBuffer[i];
  }

  bufferCount--;
}

void http_request(void* parameter) {
  Serial.println("http");
  for (;;) {  // Infinite loop for the task
    if (bufferCount == 0) {
      continue;
    }

    String url = requestBuffer[0].url;
    StationState state = requestBuffer[0].state;

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
*/

void send_station_state(int address, StationState state) {
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
  }

  Wire.beginTransmission(address);
  Wire.write((uint8_t*)&send_data, sizeof(send_data));  // Send struct as bytes
  Wire.endTransmission();
}

void get_tasks_info() {
  for (int i = 0; i < num_stations; i++) {
    StationState state = all_stations[i].state;
    int address = all_stations[i].address;

    Serial.print(address);
    Serial.print(" ");
    Serial.println(state);

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
        if (all_tasks[j].station_address == address) {
          all_tasks[j].activated = true;
          String task_id = all_tasks[j].task_id;
          //push_buffer(state, "start_progress", task_id);
        }
      }
    } else if (state == DROP) {
      /*
      if (activation_data.who_activated == 'r'){
         if (red_task_count == num_tasks / 2) continue;
      } else {
        if (blue_task_count == num_tasks / 2) continue;
      }
      */
      //Serial.print("station activated: ");
      //Serial.print(activation_data.station_activated);
      //Serial.print(" ");
      //Serial.println(activation_data.who_activated);

      //push_buffer(state, "close", "679c8736ed61c22d6e28f9b2");
      for (int j = 0; j < num_tasks; j++) {
        if (all_tasks[j].robot_color == activation_data.who_activated && all_tasks[j].activated) {
          int station_address = all_tasks[j].station_address;
          Serial.print("sending station state off");
          Serial.println(station_address);
          send_station_state(station_address, OFF);
          all_tasks[j].activated = false;
          all_tasks[j].completed = true;

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
    }
  }
}

void get_flag_info() {
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

    if (state == ACTIVATOR){

    } else if (state == KING){
      
    }
  }
}

void change_state_win() {
  for (int i = 0; i < num_stations; i++) {
    all_stations[i].state = WIN;
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
    if (address == center_station_addres) {
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
    uint8_t randomStation = (esp_random() % 8) + 1;

    if (all_stations[randomStation].state != ACTIVATOR) {
      all_stations[randomStation].state = ACTIVATOR;
      all_activators[activator_count].station_address = randomStation;
      activator_count++;
    }
  }
}

void loop() {
  if (!digitalRead(RESTART_BTN_PIN) && game_state == NONE) {
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
      game_state = FLAG;
      set_flag_states();
      send_state_all_stations();
    }
  } else if (game_state == FLAG) {
    if (millis() - game_start_time < duration_2 + duration) {

    } else {
      game_state = WIN;
    }

  } else if (game_state == WIN) {
    Serial.println("Game Over!");
    change_state_win();
    send_state_all_stations();
  }

  delay(500);  // Reduce CPU load
}
