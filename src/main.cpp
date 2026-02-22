#include <Arduino.h>
#include <DHT.h>
#include <MHZ19C_Active.h>
#include <PMS.h>
#include <SoftwareSerial.h>

#define DEBUG true

// PINS
const int PMS_RX;
const int PMS_TX;
const int CO2_RX;
const int CO2_TX;

const int DHT_PIN;
#define DHT_TYPE DHT22

// PMS
SoftwareSerial PMS_SERIAL(PMS_RX, PMS_TX);
PMS pms(PMS_SERIAL);
PMS::DATA pmsRawData;

struct SENSORS_DATA {
  uint16_t pm1_0;
  uint16_t pm2_5;
  uint16_t pm10_0;
  int co_2;
  float temperature;
  float humidity;
};
SENSORS_DATA sensorsData;

// CO2
SoftwareSerial CO2_SERIAL(CO2_RX, CO2_TX);
MHZ19C_Active mhz(CO2_SERIAL);

// TEMPERATURE and HUMIDITY sensor
DHT dht(DHT_PIN, DHT_TYPE);

// TIMER

// LCD - uses `Serial` to send data

// others
unsigned long pollingTime = 1000;

// function definitions
void calculate_PMS(SENSORS_DATA&, unsigned long, unsigned long, bool);
void calculate_CO2(SENSORS_DATA&, unsigned long, unsigned long, bool);
void calculate_DHT(SENSORS_DATA&);
void draw_LCD(SENSORS_DATA&);
void endNextionCommand();
uint32_t getNextionValue(String component);

// SETUP
void setup() {
  Serial.begin(9600);
  PMS_SERIAL.begin(9600);
  CO2_SERIAL.begin(9600);
  dht.begin();

  Serial.write("Starting...\n");
}

// MAIN
void loop() {
  // polling PMS
  PMS_SERIAL.listen();
  unsigned long startWait = millis();
  calculate_PMS(sensorsData, startWait, pollingTime, DEBUG);

  // polling CO2
  CO2_SERIAL.listen();
  startWait = millis();
  calculate_CO2(sensorsData, startWait, pollingTime, DEBUG);

  // DHT
  calculate_DHT(sensorsData, DEBUG);

  // Timer

  // LCD - Display
}

void calculate_PMS(SENSORS_DATA& sensorsData, unsigned long startTime,
                   unsigned long waitTime, bool debug = false) {
  while (millis() - startTime < waitTime) {
    if (pms.read(pmsRawData)) {
      sensorsData.pm1_0 = pmsRawData.PM_AE_UG_1_0;
      sensorsData.pm2_5 = pmsRawData.PM_AE_UG_2_5;
      sensorsData.pm10_0 = pmsRawData.PM_AE_UG_10_0;

      if (debug) {
        Serial.println("PMS Result:");
        Serial.print("PM 1.0 (ug/m3): ");
        Serial.println(pmsRawData.PM_AE_UG_1_0);

        Serial.print("PM 2.5 (ug/m3): ");
        Serial.println(pmsRawData.PM_AE_UG_2_5);

        Serial.print("PM 10.0 (ug/m3): ");
        Serial.println(pmsRawData.PM_AE_UG_10_0);
        Serial.println();
      }
      break;
    }
  }
}

void calculate_CO2(SENSORS_DATA& sensorsData, unsigned long startTime,
                   unsigned long waitTime, bool debug = false) {
  while (millis() - startTime < waitTime) {
    MHZ19C_Data co2_results = mhz.read();
    if (co2_results.success) {
      sensorsData.co_2 = co2_results.co2;

      if (debug) {
        Serial.print("CO2 Result: ");
        Serial.print(co2_results.co2);
        Serial.println(" ppm");
      }
      break;
    }
  }
}

void calculate_DHT(SENSORS_DATA& sensorsData, bool debug = false) {
  sensorsData.temperature = dht.readTemperature();
  sensorsData.humidity = dht.readHumidity();

  if (debug) {
    Serial.println("DHT Result:");

    Serial.print("temperature: ");
    Serial.print(sensorsData.temperature);
    Serial.print("\n");

    Serial.print("humidity: ");
    Serial.print(sensorsData.humidity);
    Serial.print("\n");
  }
}

// LCD related things

void draw_LCD(SENSORS_DATA& sensorsData) {
  // TODO
}

void endNextionCommand() {
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

uint32_t getNextionValue(String component) {
  // 1. Clear the buffer to make sure we don't read old data
  while (Serial.available()) {
    Serial.read();
  }

  // 2. Ask the Nextion
  Serial.print("get ");
  Serial.print(component);
  endNextionCommand();

  // 3. Wait for the reply (Nextion takes ~5-20ms to reply)
  unsigned long start = millis();
  while (Serial.available() < 8 && (millis() - start < 150)) {
    // Timeout safety
  }

  // 4. Parse the 8-byte response
  if (Serial.available() >= 8) {
    if (Serial.read() == 0x71) {  // Numeric data header
      uint32_t val = 0;
      val = Serial.read();
      val |= (uint32_t)Serial.read() << 8;
      val |= (uint32_t)Serial.read() << 16;
      val |= (uint32_t)Serial.read() << 24;

      // Toss the three 0xFF end bytes
      for (int i = 0; i < 3; i++) Serial.read();
      return val;
    }
  }

  return 0;  // Return 0 if failed or no flag set
}
