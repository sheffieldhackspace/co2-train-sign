// arduino
#include "Arduino.h"
// SCD40 library
#include "SensirionI2CScd4x.h"
// I2C
#include "BlinkDigits.h"
#include "Wire.h"

SensirionI2CScd4x scd4x;
BlinkDigits flasher1;
int ledPin = LED_BUILTIN;
uint16_t flash_co2 = 0;
bool flashing_finished = false;

// serial to sign
#include <SoftwareSerial.h>
#define RX D5                // 14
#define TX D6                // 12
SoftwareSerial link(RX, TX); // Rx, Tx

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  flasher1.config(200, 800, 200, 1500); // slower timings in milliseconds.

  Serial.begin(9600);
  while (!Serial) {
    delay(100);
  }

  // to sign
  link.begin(9600);
  pinMode(RX, INPUT);
  pinMode(TX, OUTPUT);

  Wire.begin(4, 5);

  uint16_t error;
  char errorMessage[256];

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    link.print("Error trying to execute stopPeriodicMeasurement(): ");
    link.print(errorMessage);
    link.print('\0');
  }

  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;
  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    link.print("Error trying to execute getSerialNumber(): ");
    link.print(errorMessage);
    link.print('\0');
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    link.print("Error trying to execute startPeriodicMeasurement(): ");
    link.print(errorMessage);
    link.print('\0');
  }

  Serial.println("Waiting for first measurement... (5 sec)");
  link.print("Waiting for first measurement... (5 sec)");
  link.print('\0');
}

void loop() {
  uint16_t error;
  char errorMessage[256];

  // bool fin = flasher1.blink(ledPin, LOW, flash_co2);
  if (flasher1.blink(ledPin, LOW, flash_co2)) {
    flashing_finished = true;
  }

  // Read Measurement
  uint16_t co2 = 0;
  float temperature = 0.0f;
  float humidity = 0.0f;
  bool isDataReady = false;
  error = scd4x.getDataReadyFlag(isDataReady);
  if (error) {
    Serial.print("Error trying to execute getDataReadyFlag(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    link.print("Error trying to execute getDataReadyFlag(): ");
    link.print(errorMessage);
    link.print('\0');
    return;
  }
  if (!isDataReady) {
    return;
  }
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if (error) {
    Serial.print("Error trying to execute readMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    link.print("Error trying to execute readMeasurement(): ");
    link.print(errorMessage);
    link.print('\0');
  } else if (co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
    link.print("Invalid sample detected, skipping.");
    link.print('\0');
  } else {
    // Serial.print("fin is ");
    // Serial.println(fin);
    // Serial.print("flash_co2 is ");
    // Serial.println(flash_co2);

    if (flashing_finished && (co2 != flash_co2)) {
      flash_co2 = co2;
      flashing_finished = false;
    }
    // flasher does not return true properly so just set it here
    // flash_co2 = co2;

    Serial.print("Co2\t");
    Serial.print(co2);
    Serial.print("\t");
    Serial.print("Temperature\t");
    Serial.print(temperature);
    Serial.print("\t");
    Serial.print("Humidity\t");
    Serial.println(humidity);

    // send to sign
    link.print("CO2: ");
    link.print(co2);
    link.print(" ppm");
    link.print("\n");
    link.print("temp ");
    link.print(temperature);
    link.print(" deg C");
    link.print("\n");
    link.print("humidity ");
    link.print(humidity);
    link.print(" %");
    link.print('\0');
  }
}
