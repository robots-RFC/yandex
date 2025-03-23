#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include "I2Cdev.h"

// Constants
const byte address[6] = "00001";

#define CALIB_DURATION 2000
#define NO_SIGNAL_TIMEOUT 2000

#define x_pin A1
#define y_pin A0

int x_min = -500;
int x_max = 500;
int y_min = 90;
int y_max = 900;

#define RED_PIN A3
#define GREEN_PIN 3
#define BLUE_PIN A2

#define REVERSE_LEFT_FB 1   //Реверс переднего/заднего хода левого мотора
#define REVERSE_LEFT_LR 1   //Реверс левого/правого поворота левого мотора
#define REVERSE_RIGHT_FB 1  //Реверс переднего/заднего хода правого мотора
#define REVERSE_RIGHT_LR -1 //Реверс левого/правого поворота правого мотора

struct DataStruct {
  int left_motor_value = 1500;
  int right_motor_value = 1500;
  int weapon_active = 0;
};

RF24 radio(7, 8);  // CE, CSN

DataStruct data;

void setup() {
  // put your setup code here, to run once:
  pinMode(x_pin, INPUT);
  pinMode(y_pin, INPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(4, INPUT);
  Serial.begin(9600);

  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.stopListening();

  Wire.begin();


}

void loop() {
  Serial.println(digitalRead(4));
  if (digitalRead(4)){
    data.weapon_active = 1;
  } else{
    data.weapon_active = 0;
  }
  // put your main code here, to run repeatedly:
  int x_value = analogRead(x_pin) - 400;
  int y_value = analogRead(y_pin) - 500;

  int left_motor_value = map((  y_value -  x_value), x_min, x_max , 1000, 2000);


  //сигнал управления правым мотором диапазон от 1000 до 2000
  int right_motor_value = map(( y_value +  x_value), x_min, x_max, 1000, 2000);

  data.left_motor_value = constrain(left_motor_value, 1000, 2000);
  data.right_motor_value = constrain(right_motor_value, 1000, 2000);

  //Serial.print(x_value);
  //Serial.print(" ");
  //Serial.println(right_motor_value);

  if (radio.write(&data, sizeof(data))) {
        setColor(0, 255, 0);
      } else {
        setColor(255, 0, 0);
        //Serial.println("Transmission failed!");
      }


}

void setColor(int redValue, int greenValue, int blueValue) {
  analogWrite(RED_PIN, redValue);
  digitalWrite(GREEN_PIN, greenValue);
  analogWrite(BLUE_PIN, blueValue);
}
