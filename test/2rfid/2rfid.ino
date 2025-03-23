#include <SPI.h>
#include <MFRC522.h>
#include "Adafruit_NeoPixel.h"

#define PIN 6
#define PIN2 5
#define NUM_LEDS 10
#define RST_PIN 9  // Пин rfid модуля RST
#define SS_PIN 10  // Пин rfid модуля SS
#define SS_PIN2 8  // Пин rfid модуля SS

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(NUM_LEDS, PIN2, NEO_GRB + NEO_KHZ800);

MFRC522 rfid(SS_PIN, RST_PIN);  // Объект rfid модуля
MFRC522 rfid2(SS_PIN2, RST_PIN);  // Объект rfid модуля
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

  strip2.begin();
  strip2.setBrightness(50);
  strip2.show();  // Инициализация светодиодов
  pinMode(4, OUTPUT);

  Serial.begin(9600);                        // Инициализация Serial
  SPI.begin();                               // Инициализация SPI

  // Инициализация RFID 1
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  rfid.PCD_Init();                           // Инициализация модуля
  rfid.PCD_SetAntennaGain(rfid.RxGain_max);  // Установка усиления антенны
  rfid.PCD_AntennaOff();                     // Перезагружаем антенну
  rfid.PCD_AntennaOn();                      // Включаем антенну

  // Инициализация RFID 2
  pinMode(SS_PIN2, OUTPUT);
  digitalWrite(SS_PIN2, HIGH);
  rfid2.PCD_Init();                           // Инициализация модуля
  rfid2.PCD_SetAntennaGain(rfid2.RxGain_max);  // Установка усиления антенны
  rfid2.PCD_AntennaOff();                     // Перезагружаем антенну
  rfid2.PCD_AntennaOn();                      // Включаем антенну

  for (byte i = 0; i < 6; i++) {  // Наполняем ключ
    key.keyByte[i] = 0xFF;        // Ключ по умолчанию 0xFFFFFFFFFFFF
  }
}

void selectRFID(MFRC522 &rfid, int ssPin) {
  digitalWrite(SS_PIN, HIGH);   // Disable RFID 1
  digitalWrite(SS_PIN2, HIGH);  // Disable RFID 2
  digitalWrite(ssPin, LOW);     // Enable the selected RFID
  rfid.PCD_Init();              // Reinitialize the selected RFID
}

void loop() {
  // Проверка наличия карты на RFID 1
  selectRFID(rfid, SS_PIN);
  bool new_card_present = rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();

  // Проверка наличия карты на RFID 2
  selectRFID(rfid2, SS_PIN2);
  bool new_card_present2 = rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial();

  Serial.print(new_card_present);
  Serial.print(" ");
  Serial.println(new_card_present2);

  if (new_card_present) {
    state = "loading";
    // Карта обнаружена
    Serial.println(millis() - last_found_card);
    if (millis() - last_found_card < 300) {
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
    if (millis() - last_found_card > 700) {
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

  if (new_card_present2) {
    state = "loading";
    // Карта обнаружена
    Serial.println(millis() - last_found_card);
    if (millis() - last_found_card < 300) {
      if (millis() - last_change_led > 200) {
        strip2.clear();
        for (int i = 0; i < led; i++) {
          strip2.setPixelColor(i, strip2.Color(0, 255, 0));
        }
        led += 1;
        if (led > NUM_LEDS) {
          state = "finish";
          led = 0;
        }
        strip2.show();
        last_change_led = millis();
      }
    }
    last_found_card = millis();
  } else {
    // Карта отсутствует
    if (millis() - last_found_card > 700) {
      while (led >= 0) {
        strip2.clear();
        for (int j = 0; j < led; j++) {
          strip2.setPixelColor(j, strip2.Color(0, 255, 0));
        }
        led -= 1;
        strip2.show();
        delay(200);
      }
      strip2.clear();
      for (int j = 0; j < NUM_LEDS; j++) {
        strip2.setPixelColor(j, strip2.Color(255, 0, 0));
      }
      strip2.show();
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
    rfid2.PCD_Init();
  }
}