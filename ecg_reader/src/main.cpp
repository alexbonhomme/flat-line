#include <Arduino.h>

#define ECG_PIN A0
#define LO_P 7
#define LO_N 6

void setup() {
  analogReadResolution(12);

  // initialize the serial communication:
  Serial.begin(115200);

  pinMode(LO_P, INPUT); // Setup for leads off detection LO +
  pinMode(LO_N, INPUT); // Setup for leads off detection LO -
}

void loop() {
  if((digitalRead(LO_P) == 1) || (digitalRead(LO_N) == 1)){
    Serial.println('!');
  } else{
    // send the value of analog input 0:
    Serial.println(analogRead(ECG_PIN));
  }

  //Wait for a bit to keep serial data from saturating
  delay(1);
}