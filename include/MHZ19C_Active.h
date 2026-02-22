#ifndef MHZ19C_ACTIVE_H
#define MHZ19C_ACTIVE_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// Data Sheet - SC8 CO2 5KV1.4
// https://community.openenergymonitor.org/uploads/short-url/tDiJ3EWtv7OlcHZnNgX3dFv8Cpv.pdf

struct MHZ19C_Data {
  int co2 = 0;
  int co2_alt = 0;
  int temp = 0;
  bool success = false;
  bool checksum = false;
};

class MHZ19C_Active {
 private:
  Stream* _serial;
  byte _buffer[16];

 public:
  MHZ19C_Active(Stream& serial) : _serial(&serial) {}

  void calibrate(uint16_t targetPPM) {
    if (targetPPM < 400 || targetPPM > 1500) return;  // Range per datasheet

    byte df1 = targetPPM >> 8;    // High byte
    byte df2 = targetPPM & 0xFF;  // Low byte

    byte command[6] = {0x11, 0x03, 0x03, df1, df2, 0x00};
    byte checksum = 0;

    for (int i = 0; i < (sizeof(command) / sizeof(byte)); i++) {
      checksum += command[i];
    }

    // 30 - magic number manufacturer inferred
    checksum = (checksum + 0x1E) & 0xFF;

    // insert end
    command[(sizeof(command) / sizeof(byte)) - 1] = checksum;

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

          _buffer[0] = 0x42;
          _buffer[1] = 0x4D;
          _serial->readBytes(&_buffer[2], 14);

          if (debug) {
            for (int i = 0; i < 16; i++) {
              if (_buffer[i] < 0x10) Serial.print("0");
              Serial.print(_buffer[i], HEX);
              Serial.print(" ");
            }
            Serial.println();
          }

          // Map bytes to CO2 and Temp
          // byte 2 and 3 - reverse engineer testing
          int co2 = ((_buffer[2] << 8) | _buffer[3]) -
                    2000;  // hehe - remove when calibrated

          // byte 4 and 5 - official docs
          int co2_alt = ((_buffer[4] << 8) | _buffer[5]);

          // hehe - remove when calibrated
          if (co2 < 400) {
            data.co2 = 400;
          } else {
            data.co2 = co2;
          }

          data.temp = _buffer[6];
          data.success = true;
        }
      }
    }
    return data;
  }
};

#endif