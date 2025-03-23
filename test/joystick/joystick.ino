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

int x_min;
int x_max;
int y_min;
int y_max;
int y_center;
int x_center;

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
  y_center = analogRead(y_pin);
  x_center = analogRead(x_pin);
  x_min = x_center;
  x_max = x_center;
  y_min = y_center;
  y_max = y_center;
  setColor(0, 0, 255);
  int calib_time = millis();
  while(millis() - calib_time < 5000){
    int y_value = analogRead(y_pin);
    int x_value = analogRead(x_pin);
    if (y_value < y_min){
      y_min = y_value;
    }

    if (y_value > y_max){
      y_max = y_value;
    }

    if (x_value < x_min){
      x_min = x_value;
    }

    if (x_value > x_max){
      x_max = x_value;
    }
  }
  Serial.println(y_center);
  Serial.println((y_max + y_min)/2);

}

void loop() {
  //Serial.println(digitalRead(4));
  if (digitalRead(4)){
    data.weapon_active = 1;
  } else{
    data.weapon_active = 0;
  }
  // put your main code here, to run repeatedly:
  int x_value = analogRead(x_pin) - ((x_max + x_min)/2);
  int y_value = analogRead(y_pin) - ((y_max + y_min)/2);

  int left_motor_value = map((  y_value -  x_value), x_min, x_max , 1000, 2000);


  //сигнал управления правым мотором диапазон от 1000 до 2000
  int right_motor_value = map(( y_value +  x_value), x_min+y_min, x_max+y_max, 1000, 2000);

  data.left_motor_value = constrain(left_motor_value, 1000, 2000);
  data.right_motor_value = constrain(right_motor_value, 1000, 2000);

  Serial.print(y_min);
  Serial.print(" ");
  Serial.print(y_max);
  Serial.print(" ");
  Serial.print(x_min);
  Serial.print(" ");
  Serial.print(x_max);
  Serial.print(" ");
  Serial.print(y_center);
  Serial.print("\t");
  Serial.print(x_value);
  Serial.print("\t");
  Serial.print(y_value);
  Serial.print("\t");
  Serial.println(right_motor_value);

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
