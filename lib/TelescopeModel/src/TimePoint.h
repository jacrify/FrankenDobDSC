#ifndef TELESCOPE_MODEL_TIMEPOINT_H
#define TELESCOPE_MODEL_TIMEPOINT_H

#include <chrono>
#include <ctime>
#include <string>


// Type aliases for simplicity
using Clock = std::chrono::system_clock;
using TimePoint = Clock::time_point;

TimePoint addMillisToTime(TimePoint tp, unsigned long millis);

TimePoint addSecondsToTime(TimePoint tp, double seconds) ;

void setSystemTime(const TimePoint &tp) ;

TimePoint createTimePoint(unsigned int day, unsigned int month,
                          unsigned int year, unsigned int hour,
                          unsigned int minute, unsigned int second);
/**
 * Convert from %4d-%2d-%2dT%2d:%2d:%2dZ
 */
TimePoint convertToTimePointFromString(const std::string &utc);

double differenceInSeconds(TimePoint early, TimePoint late);
std::string timePointToString(TimePoint tp);

unsigned long convertTimePointToEpochSeconds(TimePoint tp);

TimePoint getNow();
#endif
