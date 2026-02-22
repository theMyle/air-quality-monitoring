#include <Arduino.h>
#include <DHT.h>
#include <MHZ19C_Active.h>
#include <PMS.h>
#include <RTClib.h>
#include <SoftwareSerial.h>

#define DEBUG true

// PINS
const int PMS_RX = 2;
const int PMS_TX = 3;
const int CO2_RX = 4;
const int CO2_TX = 5;

const int DHT_PIN = 6;
#define DHT_TYPE DHT22

// A4 - RTC SDA
// A5 - RTC SCL

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
  DateTime dateTime;
};
SENSORS_DATA sensorsData;

// CO2
SoftwareSerial CO2_SERIAL(CO2_RX, CO2_TX);
MHZ19C_Active mhz(CO2_SERIAL);

// TEMPERATURE and HUMIDITY sensor
DHT dht(DHT_PIN, DHT_TYPE);

// TIMER
#define RTC_VERSION RTC_DS3231
RTC_VERSION RTC;

// LCD - uses `Serial` to send data

// others
unsigned long pollingTime = 1000;

// function definitions
void calculate_PMS(SENSORS_DATA&, unsigned long, unsigned long, bool);
void calculate_CO2(SENSORS_DATA&, unsigned long, unsigned long, bool);
void calculate_DHT(SENSORS_DATA&, bool);
void calculate_RTC(SENSORS_DATA&, RTC_VERSION&, bool);
void draw_LCD(SENSORS_DATA&);
void endNextionCommand();
uint32_t getNextionValue(String component);
void debug_co2(unsigned long, unsigned long);

// SETUP
void setup() {
  Serial.begin(9600);
  PMS_SERIAL.begin(9600);
  CO2_SERIAL.begin(9600);
  dht.begin();

  RTC.begin();
  if (RTC.lostPower()) RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));

  Serial.write("Starting...\n");
}

// MAIN
void loop() {
  Serial.println("\n__________START_READING__________");

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
  calculate_RTC(sensorsData, RTC, DEBUG);

  // LCD - Display
  draw_LCD(sensorsData);
}

void calculate_RTC(SENSORS_DATA& sensorsData, RTC_VERSION& rtc,
                   bool debug = false) {
  DateTime now = rtc.now();
  sensorsData.dateTime = now;

  if (debug) {
    Serial.println("\nRTC Result:");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" - ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
  }
}

void calculate_PMS(SENSORS_DATA& sensorsData, unsigned long startTime,
                   unsigned long waitTime, bool debug = false) {
  while (millis() - startTime < waitTime) {
    if (pms.read(pmsRawData)) {
      sensorsData.pm1_0 = pmsRawData.PM_AE_UG_1_0;
      sensorsData.pm2_5 = pmsRawData.PM_AE_UG_2_5;
      sensorsData.pm10_0 = pmsRawData.PM_AE_UG_10_0;

      if (debug) {
        Serial.println("\nPMS Result:");
        Serial.print("PM 1.0 (ug/m3): ");
        Serial.println(pmsRawData.PM_AE_UG_1_0);

        Serial.print("PM 2.5 (ug/m3): ");
        Serial.println(pmsRawData.PM_AE_UG_2_5);

        Serial.print("PM 10.0 (ug/m3): ");
        Serial.println(pmsRawData.PM_AE_UG_10_0);
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
        Serial.print("\nCO2 Result: ");
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
    Serial.println("\nDHT Result:");

    Serial.print("temperature: ");
    Serial.println(sensorsData.temperature);

    Serial.print("humidity: ");
    Serial.println(sensorsData.humidity);
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

void debug_co2(unsigned long startTime, unsigned long waitTime) {
  while (CO2_SERIAL.available()) CO2_SERIAL.read();

  while (millis() - startTime < waitTime) {
    // 2. Print every byte we get in a continuous stream
    if (CO2_SERIAL.available() > 0) {
      Serial.print("STREAM: ");
      while (CO2_SERIAL.available() > 0) {
        byte b = CO2_SERIAL.read();
        if (b < 0x10) Serial.print("0");
        Serial.print(b, HEX);
        Serial.print(" ");

        // If we see the header again, start a new line
        if (b == 0x42 && CO2_SERIAL.peek() == 0x4D) {
          Serial.println();
          Serial.print("STREAM: ");
        }
      }
      Serial.println();
    }
  }
}