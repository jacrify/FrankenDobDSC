#ifndef TELESCOPE_MODEL_TIMEPOINT_H
#define TELESCOPE_MODEL_TIMEPOINT_H

#include <chrono>
#include <ctime>
#include <string>
#include <sys/time.h>
#include "TimePoint.h"

// Type aliases for simplicity
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;

TimePoint addMillisToTime(TimePoint tp, unsigned long millis) {
  return tp + std::chrono::milliseconds(millis);
}

TimePoint addSecondsToTime(TimePoint tp, double seconds) {
  return tp + std::chrono::duration_cast<std::chrono::system_clock::duration>(
                  std::chrono::duration<double>(seconds));
}

void setSystemTime(const TimePoint &tp) {
  // Convert TimePoint to time_t
  std::time_t tt = Clock::to_time_t(tp);

  // Convert time_t to timeval
  struct timeval tv;
  tv.tv_sec = tt;
  tv.tv_usec = 0; // Setting microseconds to 0 for simplicity

  // Set the system time
  settimeofday(&tv, nullptr);
}

TimePoint createTimePoint(unsigned int day, unsigned int month,
                          unsigned int year, unsigned int hour,
                          unsigned int minute, unsigned int second) {
  std::tm timeStruct = {};
  timeStruct.tm_year = year - 1900; // Years since 1900
  timeStruct.tm_mon = month - 1;    // Months are 0-based
  timeStruct.tm_mday = day;
  timeStruct.tm_hour = hour;
  timeStruct.tm_min = minute;
  timeStruct.tm_sec = second;

  /// Convert tm to time_t (local time)
  std::time_t tt = std::mktime(&timeStruct);

  // Adjust for time zone to get UTC
  std::tm utc_tm = *std::gmtime(&tt);
  tt -= std::mktime(&utc_tm) - tt;

  // Convert time_t to TimePoint
  return Clock::from_time_t(tt);
}
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

double differenceInSeconds(TimePoint early, TimePoint late) {
  // Calculate the difference in time_points
  std::chrono::duration<double> duration = late - early;

  // Return the duration in seconds as a double
  return duration.count();
}

std::string timePointToString(TimePoint tp) {
  // Convert the time_point to time_t
  std::time_t tt = Clock::to_time_t(tp);

  // Convert time_t to tm (time structure)
  std::tm *timeStruct = std::localtime(&tt);

  // Format the tm structure to a string
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeStruct);

  return std::string(buffer);
}

unsigned long convertTimePointToEpochSeconds(TimePoint tp) {
  // Convert the time_point to time_t
  std::time_t tt = Clock::to_time_t(tp);

  // Since time_t represents seconds since the epoch, we can static_cast it to
  // unsigned long
  return static_cast<unsigned long>(tt);
}

TimePoint getNow() { return std::chrono::system_clock::now(); }
#endif
