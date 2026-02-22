#ifndef MHZ19C_ACTIVE_H
#define MHZ19C_ACTIVE_H

#include <Arduino.h>
#include <SoftwareSerial.h>

struct MHZ19C_Data {
  int co2 = 0;
  int temp = 0;
  bool success = false;
  bool checksum = false;
};

class MHZ19C_Active {
 private:
  Stream* _serial;
  byte _buffer[14];

  uint16_t calculateCRC(uint8_t* data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
      crc ^= (uint16_t)data[i];
      for (uint8_t j = 8; j != 0; j--) {
        if ((crc & 0x0001) != 0) {
          crc >>= 1;
          crc ^= 0xA001;
        } else {
          crc >>= 1;
        }
      }
    }
    return crc;
  }

 public:
  // Constructor: Pass the SoftwareSerial or HardwareSerial object
  MHZ19C_Active(Stream& serial) : _serial(&serial) {}

  void calibrate(uint16_t targetPPM) {
    if (targetPPM < 400 || targetPPM > 1500) return;  // Range per datasheet

    byte df1 = targetPPM >> 8;    // High byte
    byte df2 = targetPPM & 0xFF;  // Low byte

    // Construct command: 11 03 03 DF1 DF2 [cite: 108, 115]
    byte command[7] = {0x11, 0x03, 0x03, df1, df2, 0x00, 0x00};

    // Calculate and add CRC
    uint16_t crc = calculateCRC(command, 5);
    command[5] = crc & 0xFF;  // Modbus CRC usually Low byte first
    command[6] = crc >> 8;    // Then High byte

    // Send the command
    // Must be 11 03 03 DF1 DF2 CS
    _serial->write(command, 7);
  }

  MHZ19C_Data read(bool debug = false) {
    MHZ19C_Data data;

    // Check if a full 16-byte packet is available
    if (_serial->available() >= 16) {
      if (_serial->read() == 0x42) {    // Header 0x42
        if (_serial->read() == 0x4D) {  // Header 0x4D

          _serial->readBytes(_buffer, 14);

          if (debug) {
            Serial.print("42 4D ");  // Headers
            for (int i = 0; i < 14; i++) {
              Serial.print(" ");
              if (_buffer[i] < 0x10) Serial.print("0");
              Serial.print(_buffer[i], HEX);
              Serial.print(" ");
            }
            Serial.println();
          }

          // Map bytes to CO2 and Temp
          int co2 = ((_buffer[0] << 8) | _buffer[1]) - 2000;

          if (co2 < 400) {
            data.co2 = 400;
          } else {
            data.co2 = co2;
          }

          data.temp = _buffer[6];
          data.success = true;

          // Optional: Clear stale bytes to stay real-time
          while (_serial->available() > 16) {
            _serial->read();
          }
        }
      }
    }
    return data;
  }
};

#endif