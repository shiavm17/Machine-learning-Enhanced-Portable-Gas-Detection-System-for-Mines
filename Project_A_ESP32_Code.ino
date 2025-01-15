#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <BH1750.h>
#include <LoRa.h>
#include <SPI.h>
#include <SD.h>

// Pin Definitions
#define GREEN_LED 5
#define YELLOW_LED 18
#define RED_LED 19
#define BUZZER 23
#define FAN_RELAY 25
#define SD_CS_PIN 15

// Sensor Initialization
Adafruit_BME280 bme;         // BME280 Sensor
BH1750 lightMeter;           // Light Sensor
const int mq135Pin = 36, mq7Pin = 39, mq2Pin = 34, tgs2600Pin = 35, tgs822Pin = 32;

// Gas Thresholds
struct GasThreshold {
  int safe;
  int moderate;
  int hazardous;
};

GasThreshold mq135 = {100, 300, 800};
GasThreshold mq7 = {50, 150, 500};
GasThreshold mq2 = {100, 300, 700};
GasThreshold tgs2600 = {50, 200, 700};
GasThreshold tgs822 = {50, 200, 700};

// LoRa Configuration
#define LORA_CS 5
#define LORA_RST 14
#define LORA_IRQ 2

void setup() {
  // Serial Monitor
  Serial.begin(115200);

  // Pins Setup
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(FAN_RELAY, OUTPUT);

  // Sensor Initialization
  if (!bme.begin(0x76)) Serial.println("BME280 Initialization failed!");
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) Serial.println("BH1750 Initialization failed!");

  // SD Card Initialization
  if (!SD.begin(SD_CS_PIN)) Serial.println("SD Card Initialization failed!");

  // LoRa Initialization
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  if (!LoRa.begin(868E6)) Serial.println("LoRa Initialization failed!");

  // Start System with Green LED
  digitalWrite(GREEN_LED, HIGH);
}

void classifyGasLevels(int value, GasThreshold thresholds, String gasName) {
  if (value < thresholds.safe) {
    Serial.printf("%s: SAFE (%d)\n", gasName.c_str(), value);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, LOW);
  } else if (value >= thresholds.safe && value < thresholds.hazardous) {
    Serial.printf("%s: MODERATE (%d)\n", gasName.c_str(), value);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    digitalWrite(RED_LED, LOW);
  } else {
    Serial.printf("%s: HAZARDOUS (%d)\n", gasName.c_str(), value);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(FAN_RELAY, HIGH);

    // Send LoRa Alert
    LoRa.beginPacket();
    LoRa.printf("ALERT! %s Level: %d\n", gasName.c_str(), value);
    LoRa.endPacket();
  }
}

void loop() {
  // Sensor Readings
  float temp = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;
  uint16_t light = lightMeter.readLightLevel();
  int gasMQ135 = analogRead(mq135Pin);
  int gasMQ7 = analogRead(mq7Pin);
  int gasMQ2 = analogRead(mq2Pin);
  int gasTGS2600 = analogRead(tgs2600Pin);
  int gasTGS822 = analogRead(tgs822Pin);

  // Display Readings
  Serial.printf("Temp: %.2f°C, Humidity: %.2f%%, Pressure: %.2f hPa\n", temp, humidity, pressure);
  Serial.printf("Light: %d lx, MQ135: %d, MQ7: %d, MQ2: %d, TGS2600: %d, TGS822: %d\n",
                light, gasMQ135, gasMQ7, gasMQ2, gasTGS2600, gasTGS822);

  // Log Data to SD Card
  File dataFile = SD.open("data.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.printf("%.2f,%.2f,%.2f,%d,%d,%d,%d,%d,%d\n", temp, humidity, pressure, light, gasMQ135, gasMQ7, gasMQ2, gasTGS2600, gasTGS822);
    dataFile.close();
  }

  // Safety Alerts and Actions
  classifyGasLevels(gasMQ135, mq135, "MQ135");
  classifyGasLevels(gasMQ7, mq7, "MQ7");
  classifyGasLevels(gasMQ2, mq2, "MQ2");
  classifyGasLevels(gasTGS2600, tgs2600, "TGS2600");
  classifyGasLevels(gasTGS822, tgs822, "TGS822");

  // Reset Buzzer and Fan if Safe
  if (digitalRead(RED_LED) == LOW) {
    digitalWrite(BUZZER, LOW);
    digitalWrite(FAN_RELAY, LOW);
  }

  delay(2000); // 2-second delay
}
