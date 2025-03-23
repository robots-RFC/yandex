#include <Wire.h>

#define I2C_ADDRESS 1  // Change this to 2, 3, 4 for other slaves

void setup() {
  Serial.begin(9600);
  Wire.begin(I2C_ADDRESS);  // Initialize I2C with the specified address
  Wire.onReceive(receiveEvent);  // Register event for receiving data
  Wire.onRequest(requestEvent);  // Register event for sending data
}

void loop() {
  // Do nothing in the loop
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
  Wire.write("Slave ");  // Send a response back to the master
  Wire.write(I2C_ADDRESS + '0');  // Send the slave's address as part of the response
}