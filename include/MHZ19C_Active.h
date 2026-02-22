#ifndef MHZ19C_ACTIVE_H
#define MHZ19C_ACTIVE_H

#include <Arduino.h>
#include <SoftwareSerial.h>

struct MHZ19C_Data {
  int co2 = 0;
  int temp = 0;
  bool success = false;
};

class MHZ19C_Active {
 private:
  Stream* _serial;
  byte _buffer[14];

 public:
  // Constructor: Pass the SoftwareSerial or HardwareSerial object
  MHZ19C_Active(Stream& serial) : _serial(&serial) {}

  MHZ19C_Data read() {
    MHZ19C_Data data;

    // Check if a full 16-byte packet is available
    if (_serial->available() >= 16) {
      if (_serial->read() == 0x42) {    // Header 0x42
        if (_serial->read() == 0x4D) {  // Header 0x4D

          _serial->readBytes(_buffer, 14);

          // Map bytes to CO2 and Temp
          data.co2 = (_buffer[4] << 8) | _buffer[5];
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