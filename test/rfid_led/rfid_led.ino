#include <SPI.h>
#include <MFRC522.h>
#define PIN 6
#define NUM_LEDS 10
#include "Adafruit_NeoPixel.h"
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

#define RST_PIN 9  // Пин rfid модуля RST
#define SS_PIN 10  // Пин rfid модуля SS

String state = "none";
int led = 0;
uint32_t last_read = 0;
uint32_t last_change_led = 0;

MFRC522 rfid(SS_PIN, RST_PIN);  // Объект rfid модуля
MFRC522::MIFARE_Key key;        // Объект ключа
MFRC522::StatusCode status;     // Объект статуса

void setup() {
  strip.begin();
  strip.setBrightness(50);
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
  digitalWrite(4, LOW);
  // Занимаемся чем угодно
  if (state == "none") {
    strip.clear();
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(255,0,0));
    }
    strip.show();
  }

  static uint32_t rebootTimer = millis();  // Важный костыль против зависания модуля!
  if (millis() - rebootTimer >= 1000) {    // Таймер с периодом 1000 мс
    rebootTimer = millis();                // Обновляем таймер
    digitalWrite(RST_PIN, HIGH);           // Сбрасываем модуль
    delayMicroseconds(2);                  // Ждем 2 мкс
    digitalWrite(RST_PIN, LOW);            // Отпускаем сброс
    rfid.PCD_Init();                       // Инициализируем заного
  }

  if (!rfid.PICC_IsNewCardPresent()){
    
    if (millis() - last_read >= 4000){
      state = "none";
      last_read = millis();
    }
    Serial.println("n0000000000");
    return;  // Если новая метка не поднесена - вернуться в начало loop
  }
  if (!rfid.PICC_ReadCardSerial()){
    return;    // Если метка не читается - вернуться в начало loop
  } 
  state = "found";

  /*
  Serial.print("UID: ");
  for (uint8_t i = 0; i < 4; i++) {          // Цикл на 4 итерации
    Serial.print("0x");                      // В формате HEX
    Serial.print(rfid.uid.uidByte[i], HEX);  // Выводим UID по байтам
    Serial.print(", ");
  }
  Serial.println("");
  */
  
  Serial.println("found");
  digitalWrite(4, HIGH);

  if (state == "found"){
    strip.clear();
    for(int i = 0; i < led; i++){
      strip.setPixelColor(i, strip.Color(0,255,0));
    }
    delay(200);
    led += 1;
    if (led > NUM_LEDS){
      state = "finish";
      led = 0;
    }
    strip.show();
  }
  
}