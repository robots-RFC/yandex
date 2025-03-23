#include <rdm6300.h>
#include "Adafruit_NeoPixel.h"

#define PIN 6
#define NUM_LEDS 10
#define RST_PIN 9  // Пин rfid модуля RST
#define SS_PIN 5  // Пин rfid модуля SS

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

Rdm6300 rdm6300;

String state = "none";         // Состояние: none, found, removing
int led = 0;                   // Текущий светодиод
uint32_t last_change_led = 0;  // Время последнего изменения светодиода
uint32_t last_found_card = 0;
bool card_present = false;  // Флаг наличия карты

void setup() {
  strip.begin();
  strip.setBrightness(50);
  strip.show();  // Инициализация светодиодов
  pinMode(4, OUTPUT);

  Serial.begin(9600);                        // Инициализация Serial
  rdm6300.begin(4);
}

void loop() {
  // Проверка наличия карты
  bool new_card_present = rdm6300.get_tag_id();

  if (state == "finish"){
    return;
  }

  if (new_card_present) {
    state = "loading";
    // Карта обнаружена
    Serial.println(millis() - last_found_card);
    if (millis() - last_found_card < 100) {
      if (millis() - last_change_led > 200) {
        strip.clear();
        for (int i = 0; i < led; i++) {
          strip.setPixelColor(i, strip.Color(0, 255, 0));
        }
        led += 1;
        if (led > NUM_LEDS) {
          state = "finish";
          led = 0;
        }
        strip.show();
        last_change_led = millis();
      }
    }
    last_found_card = millis();
  } else {
    // Карта отсутствует
    if (millis() - last_found_card > 100) {
      while (led >= 0) {
        strip.clear(); 
        for (int j = 0; j < led; j++) {
          strip.setPixelColor(j, strip.Color(0, 255, 0));
        }
        led -= 1;
        strip.show();
        delay(200);
      }
      strip.clear();
      for (int j = 0; j < NUM_LEDS; j++) {
        strip.setPixelColor(j, strip.Color(255, 0, 0));
      }
      strip.show();
      delay(100);
    }
  }

}