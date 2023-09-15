#ifndef TELESCOPE_MODEL_TIMEPOINT_H
#define TELESCOPE_MODEL_TIMEPOINT_H

#include <chrono>
#include <ctime>
#include <string>

// Type aliases for simplicity
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;
/**
 * Convert from %4d-%2d-%2dT%2d:%2d:%2dZ
 */
TimePoint convertToTimePointFromString(const std::string &utc) {
  int year = std::stoi(utc.substr(0, 4));
  int month = std::stoi(utc.substr(5, 2));
  int day = std::stoi(utc.substr(8, 2));
  int hour = std::stoi(utc.substr(11, 2));
  int minute = std::stoi(utc.substr(14, 2));
  int second = std::stoi(utc.substr(17, 2));

  std::tm timeStruct = {};
  timeStruct.tm_year = year - 1900; // Years since 1900
  timeStruct.tm_mon = month - 1;    // Months are 0-based
  timeStruct.tm_mday = day;
  timeStruct.tm_hour = hour;
  timeStruct.tm_min = minute;
  timeStruct.tm_sec = second;

  std::time_t tt = std::mktime(&timeStruct);
  return Clock::from_time_t(tt);
}

unsigned long convertTimePointToEpochSeconds(TimePoint tp) {
  // Convert the time_point to time_t
  std::time_t tt = Clock::to_time_t(tp);

  // Since time_t represents seconds since the epoch, we can static_cast it to
  // unsigned long
  return static_cast<unsigned long>(tt);
}
#endif
