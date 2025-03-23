#include <Wire.h>
#include <rdm6300.h>
#include "Adafruit_NeoPixel.h"

#define I2C_ADDRESS   1

#define LED_PIN       6
#define NUM_LED       10
#define RFID_PIN      4
#define BUFFER_SIZE   32

#define BLINK_DELAY   100
#define LOAD_DELAY    200
#define READ_DELAY    100

Adafruit_NeoPixel led = Adafruit_NeoPixel(NUM_LED, LED_PIN, NEO_GRB + NEO_KHZ800);

Rdm6300 rdm6300;

uint32_t led_color;
const uint32_t COLOR_RED = led.Color(255, 0, 0);
const uint32_t COLOR_GREEN  = led.Color(0, 255, 0);
const uint32_t COLOR_BLUE = led.Color(0, 0, 255);
const uint32_t COLOR_YELLOW = led.Color(255, 100, 0);

// Define station states
enum StationState { OFF, COLLECT, DROP, ACTIVATOR, KING, WIN};
StationState station_state = OFF;

#pragma pack(1)
struct DataPacket {
  StationState state;
  char robot_id[8];
  char robot_color;
};
#pragma pack()

DataPacket received_data;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_ADDRESS);       // Initialize I2C with the specified address
  Wire.onReceive(receiveEvent);  // Register event for receiving data
  Wire.onRequest(requestEvent);  // Register event for sending data

  led.begin();
  led.setBrightness(50);
  led.show();  // Инициализация светодиодов

  rdm6300.begin(RFID_PIN);
}

void set_led_color(int n = NUM_LED) {
  led.clear();
  for (int i = 0; i < n; i++) {
    led.setPixelColor(i, led_color);
  }
  led.show();
}

void set_led_off() {
  led.clear();
  led.show();
}

void weapon_activate(char robot_color) {
  led_color = (robot_color == 'r') ? COLOR_RED : COLOR_BLUE;
  set_led_color();
  bool card_present;

  while (station_state == ACTIVATOR) {
    card_present = rdm6300.get_new_tag_id();

    if (card_present) {
      // send data

      for (int i = 0; i < 2; i++) {
        set_led_off();
        delay(100);
        set_led_color();
        delay(100);
      }
    }
  }
}

void collect_task(uint16_t robot_id, char robot_color) {
  led_color = (robot_color == 'r') ? COLOR_RED : COLOR_BLUE;
  set_led_color();
  Serial.println("Station Activated");

  uint16_t card_id;
  int led_count = 10;            // Текущий светодиод
  uint32_t last_change_led = 0;  // Время последнего изменения светодиода
  uint32_t last_found_card = 0;

  while (station_state == COLLECT) {
    card_id = rdm6300.get_tag_id();
    if (card_id == robot_id) {
      last_found_card = millis();
      if (millis() - last_found_card < READ_DELAY) {
        if (millis() - last_change_led > LOAD_DELAY) {
          last_change_led = millis();

          set_led_color(led_count);
          led_count -= 1;

          if (led_count < 0) {
            Serial.println("Task Collected");
            while (station_state == COLLECT) {
              set_led_off();
              delay(BLINK_DELAY);
              set_led_color();
              delay(BLINK_DELAY);
            }
          }
        }
      }
    } else {
      if (millis() - last_found_card > READ_DELAY) {
        if (millis() - last_change_led > LOAD_DELAY && led_count < NUM_LED) {
          last_change_led = millis();
          set_led_color(led_count);
          led_count += 1;
        }
      }
    }
  }
}


void drop_task(uint16_t red_id) {
  led_color = COLOR_YELLOW;
  set_led_color();

  int led_count = 0;
  uint16_t card_present;
  uint32_t last_change_led = 0;  // Время последнего изменения светодиода
  uint32_t last_found_card = 0;

  while (station_state == DROP) {
    card_present = rdm6300.get_tag_id();

    if (card_present) {
      last_found_card = millis();
      led_color = (card_present == red_id) ? COLOR_RED : COLOR_BLUE;

      if (millis() - last_found_card < READ_DELAY) {
        if (millis() - last_change_led > LOAD_DELAY) {
          last_change_led = millis();
          set_led_color();
          led_count += 1;

          if (led_count > NUM_LED) {
            led_count = 0;
          }
        }
      }
    } else {
      if (millis() - last_found_card > READ_DELAY) {
        if (millis() - last_change_led > LOAD_DELAY && led_count > 0) {
          last_change_led = millis();
          set_led_color(led_count);
          led_count -= 1;

          if (led_count < 0) {
            led_color = COLOR_YELLOW;
            set_led_color();
          }
        }
      }
    }
  }
}

void win(char robot_color) {
  if (robot_color == 'r') {
    led_color = COLOR_RED;
  } else if (robot_color == 'b') {
    led_color = COLOR_BLUE;
  } else if (robot_color == 'g') {
    led_color = COLOR_GREEN;
  }
  set_led_color();
}


// Function to handle received data
void receiveEvent(int bytes) {
  if (bytes == sizeof(received_data)) {
    Wire.readBytes((char*)&received_data, sizeof(received_data));

    Serial.println("Received Data:");
    Serial.print("Status: "); Serial.println(received_data.state);
    Serial.print("ID: "); Serial.println(received_data.robot_id);
    Serial.print("Color: "); Serial.println(received_data.robot_color);

    station_state = received_data.state;

    switch (station_state) {
      case OFF:
        Serial.println("Turning OFF station...");
        set_led_off();
        break;
      case COLLECT:
        Serial.println("Starting COLLECT task...");
        collect_task(atoi(received_data.robot_id), received_data.robot_color);
        break;
      case DROP:
        Serial.println("Station - DROP...");
        drop_task(atoi(received_data.robot_id));
        break;
      case ACTIVATOR:
        Serial.println("Station - ACTIVATOR...");
        weapon_activate(received_data.robot_color);
        break;
      case KING:
        Serial.println("Station - KING...");
        break;
      case WIN:
        Serial.println("Station - WIN...");
        win(received_data.robot_color);
        break;
    }
  }
}

// Function to handle data requests
void requestEvent() {
  //Wire.write((uint8_t*)&station_data, sizeof(station_data));  // Convert struct to bytes and send
  //Wire.write("Slave ");           // Send a response back to the master
  //Wire.write(I2C_ADDRESS + '0');  // Send the slave's address as part of the response
}

void loop() {
}
