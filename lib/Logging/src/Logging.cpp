

#include "Logging.h"
#include <cstdarg>
#include <cstdio>
#include <iostream>
#include <WebSerial.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

bool webSerialReady;
void setWebSerialReady() { webSerialReady = true; }
void log(const char *fmt, ...) {
  const int BUFFER_SIZE = 1024;
  char buffer[BUFFER_SIZE];

  va_list args;
  va_start(args, fmt);

  vsnprintf(buffer, BUFFER_SIZE, fmt, args); // Format the string

  va_end(args);

#ifdef ARDUINO
  // If we're on an Arduino (or compatible) platform
  Serial.println(buffer);
  if (webSerialReady) {
    WebSerial.println(buffer);
  }
#else
  // For native environment
  std::cout << buffer << std::endl; // Print to console
#endif
}
