#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Servo.h>
#include <Adafruit_NeoPixel.h>

#define MOTOR_MIDDLE_VALUE 1500
#define NO_SIGNAL_TIMEOUT 1000
#define STEP_SIZE 10

#define LED_PIN 2
#define NUM_LED 10
Adafruit_NeoPixel led = Adafruit_NeoPixel(NUM_LED, LED_PIN, NEO_GRB + NEO_KHZ800);

#define MOTORX_PIN 5
#define MOTORY_PIN 4
#define MOTOR_WEAPON_PIN 0

RF24 radio(8, 9);  // CE, CSN

const byte address[6] = "00001";

struct DataStruct {
  int left_motor_value = 1500;
  int right_motor_value = 1500;
  int weapon_active = 0;
};

DataStruct data;

Servo motorX;
Servo motorY;
Servo motorWeapon;

unsigned long lastSignalTime = 0;

int currentPosX = MOTOR_MIDDLE_VALUE;
int currentPosY = MOTOR_MIDDLE_VALUE;
int currentWeapon = MOTOR_MIDDLE_VALUE;
int currentPosZ = MOTOR_MIDDLE_VALUE;
int currentPosHead = MOTOR_MIDDLE_VALUE;
int currentPosEyes = MOTOR_MIDDLE_VALUE;

uint32_t weapon_active = 0;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setPALevel(RF24_PA_MIN);
  radio.startListening();

  motorX.attach(MOTORX_PIN);
  motorY.attach(MOTORY_PIN);
  motorWeapon.attach(MOTOR_WEAPON_PIN);

  motorX.writeMicroseconds(MOTOR_MIDDLE_VALUE);
  motorY.writeMicroseconds(MOTOR_MIDDLE_VALUE);
  motorWeapon.writeMicroseconds(1000);

  led.begin();
  led.setBrightness(80);
  led.show();

  for (int i = 0; i < NUM_LED; i++) {
    led.setPixelColor(i, led.Color(255, 0, 0));
  }
  led.show();
  Serial.println("ready");
}

void smoothMove(Servo& motor, int& currentPos, int targetPos) {
  if (currentPos != targetPos) {
    if (currentPos < targetPos) {
      currentPos += STEP_SIZE;
      if (currentPos > targetPos) currentPos = targetPos;
    } else {
      currentPos -= STEP_SIZE;
      if (currentPos < targetPos) currentPos = targetPos;
    }
    motor.writeMicroseconds(currentPos);
  }
}

void loop() {
  if (radio.available()) {
    radio.read(&data, sizeof(data));

    smoothMove(motorX, currentPosX, data.left_motor_value);
    smoothMove(motorY, currentPosY, data.right_motor_value);

    Serial.println(data.weapon_active);

    if (data.weapon_active) {
      motorWeapon.writeMicroseconds(1800);
    } else {
      motorWeapon.writeMicroseconds(1000);
    }

    /*
    if (data.weapon_active){
      Serial.println("active");
      smoothMove(motorWeapon, currentWeapon, 2000);
      weapon_active = millis();
    }

    if (millis() - weapon_active > 5000){
      smoothMove(motorWeapon, currentWeapon, 1500);
    }
    */

    led.clear();
    for (int i = 0; i < NUM_LED; i++) {
      led.setPixelColor(i, led.Color(0, 255, 0));
    }
    led.show();

    lastSignalTime = millis();
  } else {
    if (millis() - lastSignalTime >= NO_SIGNAL_TIMEOUT) {
      led.clear();
      for (int i = 0; i < NUM_LED; i++) {
        led.setPixelColor(i, led.Color(255, 0, 0));
      }
      led.show();
      Serial.println("no signal");

      smoothMove(motorX, currentPosX, MOTOR_MIDDLE_VALUE);
      smoothMove(motorY, currentPosY, MOTOR_MIDDLE_VALUE);
    }
  }
}