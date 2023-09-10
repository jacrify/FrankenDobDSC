#include "CoordConv.hpp"
#include "telescopeModel.h"
#include <Ephemeris.h>
#include <cstdint>
#include <iostream>
#include <stdio.h>
#include <string>
#include <unity.h> // Include the Unity test framework.

unsigned long convertDateTimeToMillis(unsigned int day, unsigned int month,
                                      unsigned int year, unsigned int hour,
                                      unsigned int minute,
                                      unsigned int second) {
  // Calculate the number of days from 1970 to the given year
  unsigned long daysSince1970 = (year - 1970) * 365UL;
  // Adjust for leap years
  for (unsigned int y = 1970; y < year; y++) {
    if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
      daysSince1970++;
    }
  }
  // Add days for each month
  static const unsigned long daysInMonth[] = {0,   31,  59,  90,  120, 151,
                                              181, 212, 243, 273, 304, 334};
  daysSince1970 += daysInMonth[month - 1];
  // Adjust for leap year if necessary
  if (((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) && month > 2) {
    daysSince1970++;
  }
  // Add the day of the month
  daysSince1970 += day - 1;
  // Calculate the total seconds
  unsigned long totalSeconds =
      (daysSince1970 * 86400UL) + (hour * 3600UL) + (minute * 60UL) + second;
  // Convert to milliseconds
  unsigned long totalMillis = totalSeconds * 1000UL;
  return totalMillis;
}

void test_eq_to_horizontal(void) {

  // Set location on earth for horizontal coordinates transformations
  Ephemeris::setLocationOnEarth(-34.0, 2.0, 44.0, // Lat: 48°50'11"
                                151.0, 3.0, 3.0); // Lon: -2°20'14"
  Ephemeris::
      // East is negative and West is positive
      Ephemeris::flipLongitude(false);

  // Set altitude to improve rise and set precision
  Ephemeris::setAltitude(110);

  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;

  EquatorialCoordinates eqCoord;
  // should be arcturus
  eqCoord.ra =
      Ephemeris::hoursMinutesSecondsToFloatingHours(14, 16, 46); // 2h31m49s
  eqCoord.dec = Ephemeris::degreesMinutesSecondsToFloatingDegrees(
      19, 4, 21); // +89° 15′ 51″

  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          eqCoord, day, month, year, hour, minute, second);
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(6.224824, altAzCoord.alt, "Alt");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(298.06, altAzCoord.azi, "Az");
  //   TEST_ASSERT_EQUAL_FLOAT_MESSAGE(
  //       Ephemeris::degreesMinutesSecondsToFloatingDegrees(6, 21, 37.7),
  //       altAzCoord.alt, "Alt");
  //   TEST_ASSERT_EQUAL_FLOAT_MESSAGE(
  //       Ephemeris::degreesMinutesSecondsToFloatingDegrees(298, 3, 3.7),
  //       altAzCoord.azi, "Az");
}

void test_horizontal_to_eq(void) {

  // Set location on earth for horizontal coordinates transformations
  Ephemeris::setLocationOnEarth(-34.0, 2.0, 44.0, // Lat: 48°50'11"
                                151.0, 3.0, 3.0); // Lon: -2°20'14"

  // East is negative and West is positive
  Ephemeris::flipLongitude(false);

  // Set altitude to improve rise and set precision
  Ephemeris::setAltitude(110);

  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;

  HorizontalCoordinates altAzCoord;

  altAzCoord.azi =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(298, 3, 3.7);
  altAzCoord.alt =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(6, 21, 37.7);

  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(6.360472, altAzCoord.alt, "Alt");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(298.051, altAzCoord.azi, "Az");
  EquatorialCoordinates eqCoord =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
          altAzCoord, day, month, year, hour, minute, second);
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(14.28644, eqCoord.ra, "ra");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(18.97959, eqCoord.dec, "dec");
}

void test_telescope_model(void) {

  TelescopeModel model;
  // model.setLatitude(-34.0493);
  // model.setLongitude(151.0494);
  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;

  unsigned long timeMillis =
      convertDateTimeToMillis(day, month, year, hour, minute, second);

  // model.setUTCYear(2023);
  // model.setUTCMonth(9);
  // model.setUTCDay(2);
  // model.setUTCHour(10);
  // model.setUTCMinute(0);
  // model.setUTCSecond(0);

  model.setAltEncoderStepsPerRevolution(36000); // 100 ticks per degree
  model.setAzEncoderStepsPerRevolution(36000);

  double star1AltAxis = 83.87; // degrees
  double star1AzmAxis = 99.25; // anticlockwise)

  model.setEncoderValues(star1AltAxis * 100, 36000 - star1AzmAxis * 100);

  // model.currentAlt = star1AltAxis;
  // model.currentAz = 360 - star1AzmAxis;

  double star1RAHours = Ephemeris::hoursMinutesSecondsToFloatingHours(0, 7, 54);
  double star1RADegrees = 360 * star1RAHours / 24.0;
  double star1Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 27, 56);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 1.97498552, star1RADegrees, "star1ra");
  double star1Dec = 29.038;

  model.setPositionRaDec(star1RADegrees, star1Dec, timeMillis);
  model.addReferencePoint();
  //  double star1AltAxis = 83.87;
  // double star1AzmAxis = 99.25; // anticlockwise)
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1AzmAxis, model.currentAz,
  //                                  "currentaz");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1AltAxis, model.currentAlt,
  //                                  "currentalt");

  // model.currentDec = star1Dec;
  // model.currentRA = star1RADegrees;
  // std::cout << "Current alt:" << model.currentAlt << "\n\r";
  

  double star2AltAxis = 35.04;  // degrees
  double star2AzmAxis = 310.98; // anticlockwise)

  model.setEncoderValues(star2AltAxis * 100, 36000 - star2AzmAxis * 100);
  // model.currentAlt = star2AltAxis;
  // model.currentAz = 360 - star2AzmAxis;

  double star2RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(2, 21, 45);

  double star2Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 37, 02);
  // time travel
  star2RAHours = star2RAHours - (star2Time - star1Time);

  double star2RADegress = 360 * star2RAHours / 24.0;

  double star2Dec = 89.222;
  // model.currentDec = star2Dec;
  // model.currentRA = star2RADegress;
  model.setPositionRaDec(star2RADegress, star2Dec,
                         timeMillis + 546000); // time passed in millis
  model.addReferencePoint();
  model.calculateThirdReference(); //prob not needed



  // If you want to aim the telescope at b Cet (ra = 0h43m07s, dec = -18.038)
  // This calculated telescope coordinates is very close to the measured
  // telescope coordinates,
  // j = 130.46, , q = 37.67o

  // set encoders to ra/dec values from example
  double star3AltAxis = 37.67;  // degrees
  double star3AzmAxis = 130.21; // anticlockwise)

  model.setEncoderValues(star3AltAxis * 100, 36000 - star3AzmAxis * 100);
  // model.currentAlt = star3AltAxis;
  // model.currentAz = 360 - star3AzmAxis;

  double star3RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(0, 43, 07);
  double star3Dec = -18.038;

  double star3Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 52, 12);
  // time travel
  star3RAHours = star3RAHours - (star3Time - star1Time);

  double star3RADegrees = 360 * star3RAHours / 24.0;
  // star3RA = fmod(star3RA, 360);
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, star3RA, "star3RA");
  model.calculateCurrentPosition();

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, model.currentDec, "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3RADegrees, model.currentRA, "ra");
}

void test_odd_times(void) {
  TelescopeModel model;
  model.setLatitude(-34.049120);
  model.setLongitude(151.042100);
  // Choose a date and time (UTC)
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  unsigned long timeMillis =
      convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setAltEncoderStepsPerRevolution(108229);
  model.setAzEncoderStepsPerRevolution(-30000);

  model.setEncoderValues(0, 0);
  // arcturus
  model.setPositionRaDec(0, 0, timeMillis);
  model.calculateCurrentPosition();

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1, 0.0, model.getRACoord(), "ra");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1, 0.0, model.getDecCoord(), "dec");
}

void test_az_encoder_calibration(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(360);
  model.setAzEncoderStepsPerRevolution(360);
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  unsigned long timeMillis =
      convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(0, 0);
  model.setPositionRaDec(14.28644, 18.97959, timeMillis);
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint(); // Save this point

  // Generate new Alt/Az values from a known move
  HorizontalCoordinates newAltAz;
  newAltAz.alt = model.getAltCoord() + 1.0; // Assuming a 1 degree move in Alt
  newAltAz.azi = model.getAzCoord() + 1.0;  // And a 1 degree move in Az

  // Convert the new Alt/Az to Equatorial using the hardcoded time values
  EquatorialCoordinates newEq =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(newAltAz, 2, 9,
                                                                2023, 10, 0, 0);

  // Set the new position using the converted RA/DEC and simulate new encoder
  // values
  model.setPositionRaDec(newEq.ra, newEq.dec, timeMillis);
  model.setEncoderValues(
      0, 100); // Here the azimuth encoder value has increased by 100
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint(); // Save this point

  long calculatedSteps = model.calculateAzEncoderStepsPerRevolution();

  float epsilon = 1.0; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, 36000, calculatedSteps,
                                   "Azimuth Encoder Steps per Revolution");
}

void test_alt_encoder_calibration(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(9999);
  model.setAzEncoderStepsPerRevolution(9999);

  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  unsigned long timeMillis =
      convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(0, 0);
  model.setPositionRaDec(14.28644, 18.97959, timeMillis);
  // sets alt az
  model.calculateCurrentPosition();

  model.saveEncoderCalibrationPoint(); // Save this point

  // Generate new Alt/Az values from a known move
  HorizontalCoordinates newAltAz;
  newAltAz.alt = model.getAltCoord() + 1.0; // Assuming a 1 degree move in Alt
  newAltAz.azi = model.getAzCoord();        // Keep azimuth same

  // Convert the new Alt/Az to Equatorial using the hardcoded time values
  EquatorialCoordinates newEq =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(newAltAz, 2, 9,
                                                                2023, 10, 0, 0);

  // Set the new position using the converted RA/DEC and simulate new encoder
  // values
  model.setPositionRaDec(newEq.ra, newEq.dec, timeMillis);

  model.setEncoderValues(
      100, 0); // Here the altitude encoder value has increased by 100
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint(); // Save this point

  long calculatedSteps = model.calculateAltEncoderStepsPerRevolution();
  // Use
  float epsilon = 1.0; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, 36000, calculatedSteps,
                                   "Altitude Encoder Steps per Revolution");
}

void test_az_encoder_wraparound(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(360);
  model.setAzEncoderStepsPerRevolution(360);
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  unsigned long timeMillis =
      convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(0, 0);
  model.setPositionRaDec(14.28644, 18.9795, timeMillis);
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint(); // Save this point

  // Generate new Alt/Az values from a known move
  HorizontalCoordinates newAltAz;
  newAltAz.alt = model.getAltCoord() + 1.0; // Assuming a 1 degree move in Alt
  newAltAz.azi = model.getAzCoord() + 170;  // And a 1 degree move in Az

  // Convert the new Alt/Az to Equatorial using the hardcoded time values
  EquatorialCoordinates newEq =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(newAltAz, 2, 9,
                                                                2023, 10, 0, 0);

  // Set the new position using the converted RA/DEC and simulate new encoder
  // values
  model.setPositionRaDec(newEq.ra, newEq.dec, timeMillis);
  model.setEncoderValues(
      0, 17000); // Here the azimuth encoder value has increased by 100
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint(); // Save this point

  long calculatedSteps = model.calculateAzEncoderStepsPerRevolution();

  float epsilon = 1.0; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, 36000, calculatedSteps,
                                   "Azimuth Encoder Steps per Revolution");
}

void test_alt_encoder_negative_move(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(9999);
  model.setAzEncoderStepsPerRevolution(9999);
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  unsigned long timeMillis =
      convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(100, 0);
  model.setPositionRaDec(14.28644, 18.97959, timeMillis);
  // sets alt az
  model.calculateCurrentPosition();

  model.saveEncoderCalibrationPoint(); // Save this point

  // Generate new Alt/Az values from a known move
  HorizontalCoordinates newAltAz;
  newAltAz.alt = model.getAltCoord() - 1.0; // Assuming a 1 degree move in Alt
  newAltAz.azi = model.getAzCoord();        // Keep azimuth same

  // Convert the new Alt/Az to Equatorial using the hardcoded time values
  EquatorialCoordinates newEq =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(newAltAz, 2, 9,
                                                                2023, 10, 0, 0);

  // Set the new position using the converted RA/DEC and simulate new encoder
  // values
  model.setPositionRaDec(newEq.ra, newEq.dec, timeMillis);

  model.setEncoderValues(
      0, 0); // Here the altitude encoder value has decreased by 100
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint(); // Save this point

  long calculatedSteps = model.calculateAltEncoderStepsPerRevolution();
  // Use
  float epsilon = 1.0; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, 36000, calculatedSteps,
                                   "Altitude Encoder Steps per Revolution");
}

void test_two_star_takeshi_example() {

  // http: // takitoshimi.starfree.jp/matrix/matrix_method_rev_e.pdf

  // Default alignment for alt az in south
  //  alignment.addReference(0, 0, M_PI, 0);//radians
  //  alignment.addReference(0, 0, 180, 0); //degrees

  // alignment.addReference(0, M_PI_2, M_PI, M_PI_2);//radians
  // alignment.addReference(0, 90, 180,90); //degreess

  // alignment.calculateThirdReference();
  CoordConv alignment;

  double star1AltAxis = 83.87;
  double star1AzmAxis = 99.25; // anticlockwise)

  double star1RAHours = Ephemeris::hoursMinutesSecondsToFloatingHours(0, 7, 54);
  double star1RADegrees = 360 * star1RAHours / 24.0;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 1.97498552, star1RADegrees, "star1ra");
  double star1Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 27, 56);
  double star1Dec = 29.038;
  alignment.addReferenceDeg(star1RADegrees, star1Dec, star1AzmAxis,
                            star1AltAxis);

  double star2AltAxis = 35.04;
  double star2AzmAxis = 310.98; // anticlockwise)
  double star2RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(2, 21, 45);

  double star2Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 37, 02);
  // time travel
  star2RAHours = star2RAHours - (star2Time - star1Time);

  double star2RADegress = 360 * star2RAHours / 24.0;

  double star2Dec = 89.222;
  alignment.addReferenceDeg(star2RADegress, star2Dec, star2AzmAxis,
                            star2AltAxis);
  int i = alignment.getRefs();
  std::cout << "===Alignment ref: " << i << "\n\r";
  alignment.calculateThirdReference();

  double star3RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(0, 43, 07);

  double star3Dec = -18.038;

  double star3Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 52, 12);
  // time travel
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, star3RA, "star3RA");
  star3RAHours = star3RAHours - (star3Time - star1Time);

  double star3RADegrees = 360 * star3RAHours / 24.0;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 4.712482, star3RADegrees, "ra");
  // star3RA = fmod(star3RA, 360);
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, star3RA, "star3RA");

  double outaxis1;
  double outaxis2;
  alignment.toInstrumentDeg(outaxis1, outaxis2, star3RADegrees, star3Dec);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, outaxis2, "alt");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 130.21, outaxis1, "az");

  alignment.toReferenceDeg(star3RADegrees, star3Dec,outaxis1, outaxis2  );

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 4.712482, star3RADegrees, "ra");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, -18.038, star3Dec, "dec");
  // 3.7002 degree error
  // 14.8 minutes or 888 seconds 14 m 48 seconds
}

void setup() {

  // test_telescope_model();
  UNITY_BEGIN(); // IMPORTANT LINE!

  // RUN_TEST(test_eq_to_horizontal);
  // RUN_TEST(test_horizontal_to_eq);
  RUN_TEST(test_telescope_model);
  // RUN_TEST(test_az_encoder_calibration);
  // RUN_TEST(test_alt_encoder_calibration);
  // RUN_TEST(test_az_encoder_wraparound);
  // RUN_TEST(test_alt_encoder_negative_move);
  // RUN_TEST(test_odd_times);

  RUN_TEST(test_two_star_takeshi_example);

  UNITY_END(); // IMPORTANT LINE!
}

void loop() {
  // Do nothing here.
}

int main() {

  setup();
  return 0;
}
