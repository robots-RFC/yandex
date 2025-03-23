#include <Wire.h>

#define TCA9548A_ADDRESS  0x70  // Default I2C address of TCA9548A
#define RESTART_BTN_PIN   4
#define NUM_STATIONS      3
#define NUM_TASKS         2

char red_robot_id[8] = "14393707";
char blue_robot_id[8] = "123";

int center_station_addres = 1;

int red_task_count = 0;
int blue_task_count = 0;

unsigned long game_start_time;
const unsigned long duration = 5000;

enum GameState { NONE, TASKS, FLAG};
GameState game_state = NONE;

// Define station states
enum StationState : uint8_t { OFF, COLLECT, DROP, ACTIVATOR, KING, WIN};

struct Tasks {
  String task_id;
  int station_address;
  char robot_color;
  bool activated;
  bool complete;
};

Tasks all_tasks[NUM_TASKS] = {
  {"679c8736ed61c22d6e28f9b2", 1, 'r', false, false},
  {"679c8736ed61c22d6e28f9b2", 2, 'b', false, false},
};

struct Station {
  int address;
  StationState state;
};

Station all_stations[NUM_STATIONS] = {
  {1, COLLECT},
  {2, COLLECT},
  {3, DROP},
};

#pragma pack(1)
struct send_dataPacket {
  StationState state;
  char robot_id[8];
  char robot_color;
};
#pragma pack()

send_dataPacket send_data;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(RESTART_BTN_PIN, INPUT_PULLUP);
}

void sendMessage(uint8_t address, const char* message) {
  Wire.beginTransmission(address);
  Wire.write(message, strlen(message));
}

void start_tasks() {
  for (uint8_t i = 1; i <= NUM_STATIONS; i++) {
    sendMessage(i, "c_123_r");
  }
}

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

void send_off() {
  for (uint8_t i = 1; i <= NUM_STATIONS; i++) {
    sendMessage(i, "o");
  }
}

void send_station_state() {
  for (int i = 0; i = NUM_STATIONS; i++) {
    int address = all_stations[i].address;
    StationState state = all_stations[i].state;

    if (state == DROP) {
      send_data.state = state;
      strcpy(send_data.robot_id, red_robot_id);
      send_data.robot_color = 'r';
    } else {
      for (int j = 0; j < NUM_TASKS; j++) {
        if (all_tasks[j].station_address == address) {
          char robot_color = all_tasks[j].robot_color;
          uint16_t robot_id = (robot_color == 'r') ? red_robot_id : blue_robot_id;
          send_data.state = state;
          strcpy(send_data.robot_id, robot_id);
          send_data.robot_color = robot_color;
          break;
        }
      }
    }
    Wire.beginTransmission(address);
    Wire.write((uint8_t*)&send_data, sizeof(send_data));  // Convert struct to bytes and send
    Wire.endTransmission();
  }
}

void get_station_info() {

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
      //send_win();
      game_state = NONE;
    }
  }

  delay(500); // Reduce CPU load
}
