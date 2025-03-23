#include <Wire.h>
#include <rdm6300.h>
#include "Adafruit_NeoPixel.h"

#define I2C_ADDRESS 1

#define LED_PIN 6
#define NUM_LEDS 10
#define RFID_PIN 4

Adafruit_NeoPixel led = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

Rdm6300 rdm6300;

String led_state = "none";     // Состояние: none, found, removing
int led_count = 0;             // Текущий светодиод
uint32_t last_change_led = 0;  // Время последнего изменения светодиода
uint32_t last_found_card = 0;
bool card_present = false;  // Флаг наличия карты
uint32_t color;

void setup() {
  Serial.begin(9600);
  Wire.begin(I2C_ADDRESS);       // Initialize I2C with the specified address
  Wire.onReceive(receiveEvent);  // Register event for receiving data
  Wire.onRequest(requestEvent);  // Register event for sending data

  led.begin();
  led.setBrightness(50);
  led.show();  // Инициализация светодиодов

  rdm6300.begin(RFID_PIN);
}

void all_led() {

  for (int i = 0; i < NUM_LEDS; i++) {
    led.setPixelColor(i, color);
  }
  led.show();
}

void war_activate(char robot) {
  if (robot == 'r') {
    color = led.Color(255, 0, 0);
  } else if (robot == 'b') {
    color = led.Color(0, 0, 255);
  }

  all_led();

  while (true) {
    card_present = rdm6300.get_new_tag_id();
    Serial.println(card_present);

    if (card_present) {
      // send data

      for (int i = 0; i < 2; i++) {
        led.clear();
        led.show();
        delay(100);
        all_led();
        delay(100);
      }
    }
  }
}

void collect_task(uint16_t robot_id, char robot_color) {
  uint16_t card_id = rdm6300.get_tag_id();
  Serial.println(card_id);
  if (robot_color == 'r') {
    color = led.Color(255, 0, 0);
  } else if (robot_color == 'b') {
    color = led.Color(0, 0, 255);
  }

  all_led();

  if (card_id == robot_id) {
    while (true) {
      // !!! send data
      for (int i = 0; i < 2; i++) {
        led.clear();
        led.show();
        delay(100);
        all_led();
        delay(100);
      }
    }
    // Карта обнаружена
    //Serial.println(millis() - last_found_card);
    /*
    if (millis() - last_found_card < 100) {
      if (millis() - last_change_led > 200) {
        led.clear();
        for (int i = 0; i < led_count; i++) {
          led.setPixelColor(i, led.Color(0, 255, 0));
        }
        led_count += 1;
        if (led_count > NUM_LEDS) {
          led_state = "finish";
          led_count = 0;
        }
        led.show();
        last_change_led = millis();
      }
    }
    last_found_card = millis();
  } else {
    // Карта отсутствует
    if (millis() - last_found_card > 100) {
      while (led_count >= 0) {
        led.clear();
        for (int j = 0; j < led_count; j++) {
          led.setPixelColor(j, led.Color(0, 255, 0));
        }
        led_count -= 1;
        led.show();
        delay(200);
      }
      */
    /*
        strip.clear();
        for (int j = 0; j < NUM_LEDS; j++) {
        strip.setPixelColor(j, strip.Color(255, 0, 0));
        }
        strip.show();
        delay(100);
      */
  }
}

void inactive() {
  led.clear();
  led.show();
}

void complet_task(uint16_t red_id, uint16_t blue_id) {
  color = led.Color(255, 200, 0);

  all_led();


  uint16_t card_present = rdm6300.get_tag_id();

  if (card_present) {
    if (millis() - last_found_card < 100) {
      if (millis() - last_change_led > 200) {
        led.clear();
        for (int i = 0; i < led_count; i++) {
          led.setPixelColor(i, led.Color(0, 255, 0));
        }
        led_count += 1;
        if (led_count > NUM_LEDS) {
          led_state = "finish";
          led_count = 0;
        }
        led.show();
        last_change_led = millis();
      }
    }
    last_found_card = millis();
  } else {
    // Карта отсутствует
    if (millis() - last_found_card > 100) {
      while (led_count >= 0) {
        led.clear();
        for (int j = 0; j < led_count; j++) {
          led.setPixelColor(j, led.Color(0, 255, 0));
        }
        led_count -= 1;
        led.show();
        delay(200);
      }
      
      /*
        strip.clear();
        for (int j = 0; j < NUM_LEDS; j++) {
        strip.setPixelColor(j, strip.Color(255, 0, 0));
        }
        strip.show();
        delay(100);
      */
    }
  }
}

void win(char robot) {
  if (robot == 'r') {
    color = led.Color(255, 0, 0);

  } else if (robot == 'b') {
    color = led.Color(0, 0, 255);

  } else if (robot == 'g') {
    color = led.Color(0, 255, 0);
  }

  all_led();
}

void loop() {
  //war_activate('b');
  //win('g');
  //war_center();
  //color = led.Color(255, 0, 0);
  //all_led();
  collect_task(14393707, 'r');
  //complet_task(14393707, 14393708);
}










// Function to handle received data
void receiveEvent(int bytes) {
  while (Wire.available()) {
    char c = Wire.read();
    Serial.print(c);
  }
  Serial.println();
}

// Function to handle data requests
void requestEvent() {
  Wire.write("Slave ");           // Send a response back to the master
  Wire.write(I2C_ADDRESS + '0');  // Send the slave's address as part of the response
}