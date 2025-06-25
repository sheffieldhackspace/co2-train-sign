/**
 * @file monitor-and-logger.cpp
 * @author alifeee (alifeee@alifeee.net)
 * @brief gets environment data, sends it to bus sign as well as InfluxDB
 * database
 * @version 0.1
 * @date 2025-06-25
 *
 * @copyright Copyright (c) 2025
 *
 */
// arduino
#include "Arduino.h"
// SCD40 library
#include "SensirionI2CScd4x.h"
// blink digits for flashing LED
#include "BlinkDigits.h"
// I2C
#include "Wire.h"
// for WiFi
// wifi
#include <ESP8266WiFiMulti.h>
// for connection to influxdb
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
// contains WiFi and Influx details
#include <secrets.h>

ESP8266WiFiMulti wifiMulti;

// Declare InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET,
                      INFLUXDB_TOKEN, InfluxDbCloud2CACert);
// Declare Data point
Point influxsensor("environment");

#define DO_SERIAL
#define DEVICE "ESP8266-BUSSIGN"
#define TZ_INFO "UTC0"

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

// error marks
int wifiError = 0;
int sensorError = 0;
int influxError = 0;

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
    sensorError = 1;
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
    sensorError = 1;
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

  // SETUP wifi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to wifi");
  link.print("Connecting to wifi");
  link.print('\0');
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  // SETUP INFLUXDB
  // Accurate time is necessary for certificate validation and writing in
  // batches We use the NTP servers in your area as provided by:
  // https://www.pool.ntp.org/zone/ Syncing progress and the time will be
  // printed to Serial.
  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");
  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
    link.print("Connecting to wifi");
    link.println(client.getServerUrl());
    link.print('\0');
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
    link.print("InfluxDB connection failed: ");
    link.println(client.getLastErrorMessage());
    link.print('\0');
    influxError = 1;
  }
  // Add tags to the data point
  influxsensor.addTag("device", DEVICE);
  influxsensor.addTag("SSID", WiFi.SSID());

  link.print("Waiting for first measurement... (5 sec)");
  link.print('\0');
}

void loop() {
  if (wifiError) {
    Serial.println("WiFi connect error");
    delay(1000);
  } else if (sensorError) {
    Serial.println("Sensor Error");
    delay(1000);
  } else if (influxError) {
    Serial.print("InfluxDB error");
    delay(1000);
  }

  uint16_t error;
  char errorMessage[256];

  // Clear fields for reusing the point. Tags will remain the same as set above.
  influxsensor.clearFields();

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
    sensorError = 1;
    return;
  } else {
    sensorError = 0;
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
    sensorError = 1;
  } else if (co2 == 0) {
    Serial.println("Invalid sample detected, skipping.");
    link.print("Invalid sample detected, skipping.");
    link.print('\0');
  } else {
    sensorError = 0;
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
    if (sensorError || wifiError || influxError) {
      link.print("  E");
    }
    link.print("\n");
    link.print("temp ");
    link.print(temperature);
    link.print(" deg C");
    link.print("\n");
    link.print("humidity ");
    link.print(humidity);
    link.print(" %");
    link.print('\0');

    // Store measured value into point
    influxsensor.addField("co2", co2);
    influxsensor.addField("temperature", temperature);
    influxsensor.addField("humidity", humidity);

    // Print what are we exactly writing
    Serial.print("Writing: ");
    Serial.println(influxsensor.toLineProtocol());

    // Write point
    if (!client.writePoint(influxsensor)) {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
      influxError = 1;
    } else {
      Serial.println("InfluxDB write success");
      influxError = 0;
    }

    // wait 30s
    Serial.println("waiting 30s for next measurement...");
    delay(30000);
  }

  // Check WiFi connection and reconnect if needed
  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
    wifiError = 1;
  } else {
    wifiError = 0;
  }
}
