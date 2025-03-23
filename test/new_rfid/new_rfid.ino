#include <SPI.h>
#include <MFRC522.h>
#include "Adafruit_NeoPixel.h"

#define PIN 6
#define NUM_LEDS 10
#define RST_PIN 9  // Пин rfid модуля RST
#define SS_PIN 5  // Пин rfid модуля SS

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

MFRC522 rfid(SS_PIN, RST_PIN);  // Объект rfid модуля
MFRC522::MIFARE_Key key;        // Объект ключа

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
  SPI.begin();                               // Инициализация SPI
  rfid.PCD_Init();                           // Инициализация модуля
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);  // Установка усиления антенны
  rfid.PCD_AntennaOff();                     // Перезагружаем антенну
  rfid.PCD_AntennaOn();                      // Включаем антенну

  for (byte i = 0; i < 6; i++) {  // Наполняем ключ
    key.keyByte[i] = 0xFF;        // Ключ по умолчанию 0xFFFFFFFFFFFF
  }
}

void loop() {
  // Проверка наличия карты
  bool new_card_present = rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();

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

  // Перезагрузка модуля каждую секунду для предотвращения зависания
  static uint32_t rebootTimer = millis();
  if (millis() - rebootTimer >= 1000) {
    rebootTimer = millis();
    digitalWrite(RST_PIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(RST_PIN, LOW);
    rfid.PCD_Init();
  }
}