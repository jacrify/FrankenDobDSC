#include "CoordConv.hpp"
#include "Logging.h"
#include "TelescopeModel.h"
#include <Ephemeris.h>

#include "TimePoint.h"
#include <cstdint>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string>
#include <unity.h>
#define PI 3.14159265
#include <iostream>

bool isLeapYear(unsigned int year) {
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}
unsigned long convertDateTimeToMillis(unsigned int day, unsigned int month,
                                      unsigned int year, unsigned int hour,
                                      unsigned int minute,
                                      unsigned int second) {
  // These arrays account for the number of days in a given month and leap years
  static const unsigned int daysInMonth[] = {0,  31, 28, 31, 30, 31, 30,
                                             31, 31, 30, 31, 30, 31};
  static const unsigned int daysInMonthLeap[] = {0,  31, 29, 31, 30, 31, 30,
                                                 31, 31, 30, 31, 30, 31};

  // Function to check if a given year is a leap year
  bool isLeap = isLeapYear(year);

  unsigned long totalDays = 0;

  // Calculate days for years
  for (unsigned int y = 1970; y < year; ++y) {
    totalDays += isLeapYear(y) ? 366 : 365;
  }

  // Calculate days for months
  for (unsigned int m = 1; m < month; ++m) {
    totalDays += isLeapYear(year) ? daysInMonthLeap[m] : daysInMonth[m];
  }

  // Add the days of the current month
  totalDays += day - 1;

  unsigned long totalSeconds = totalDays * 86400 + // Days to seconds
                               hour * 3600 +       // Hours to seconds
                               minute * 60 +       // Minutes to seconds
                               second;             // Add the given seconds

  // Convert to milliseconds
  return totalSeconds * 1000;
}
// void testTimestepConversion() {
//   Ephemeris::setLocationOnEarth(-34.0, 2.0, 44.0, // Lat: 48°50'11"
//                                 151.0, 3.0, 3.0); // L
//   Ephemeris::
//       // East is negative and West is positive
//       Ephemeris::flipLongitude(false);
//   EqCoord eq=EqCoord();
//   eq.setRAInHours(12.995290);
//   eq.setDecInDegrees(5.904804);

//   HorizCoord h = HorizCoord(eq, 1694763308);
//   log("Alt: %lf\tAzi:%lf",h.altInDegrees,h.aziInDegrees);

// }

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

void test_eq_coord_distance(void) {
  // First set of coordinates
  EqCoord eq1;
  eq1.setDecInDegrees(10);
  eq1.setRAInDegrees(90);

  // Second set of coordinates
  EqCoord eq2;
  eq2.setDecInDegrees(10);
  eq2.setRAInDegrees(10);

  // Expected distance
  double expected_distance = 78.85;
  float epsilon = .5; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, expected_distance,
                                   eq1.calculateDistanceInDegrees(eq2),
                                   "distance is wrong");

  eq1.setRAInDegrees(40);
  eq2.setRAInDegrees(360 - 40);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, expected_distance,
                                   eq1.calculateDistanceInDegrees(eq2),
                                   "distance is wrong");
}

void test_horizontal_to_eq_eq_constructor(void) {

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

  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  //   unsigned long time =
  //       convertDateTimeToMillis(day, month, year, hour, minute, second)/1000;

  TEST_ASSERT_EQUAL_INT64_MESSAGE(
      1693648800, convertTimePointToEpochSeconds(star1Time), "time");
  HorizontalCoordinates altAzCoord;

  altAzCoord.azi =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(298, 3, 3.7);
  altAzCoord.alt =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(6, 21, 37.7);

  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(6.360472, altAzCoord.alt, "Alt");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(298.051, altAzCoord.azi, "Az");

  HorizCoord h = HorizCoord(altAzCoord);
  EqCoord eqCoord = EqCoord(h, star1Time);
  //    EquatorialCoordinates eqCoord =
  //       Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
  //           altAzCoord, day, month, year, hour, minute, second);
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(14.28644, eqCoord.getRAInHours(), "ra");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(18.97959, eqCoord.getDecInDegrees(), "dec");

  h = HorizCoord(0, 180);
  eqCoord = EqCoord(h, star1Time);
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(14.28644, eqCoord.getRAInHours(), "ra");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(18.97959, eqCoord.getDecInDegrees(), "dec");
}

// void test_telescope_model_takeshi(void) {
//   // this test will never work without knowing where taki lives, needs lat
//   long
//   // to be correct
//   log("======test_telescope_model_takeshi=====");
//   TelescopeModel model;
//   model.setLatitude(34.0493);
//   model.setLongitude(151.0494);
//   // Choose a date and time (UTC)
//   unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
//                second = 0;

//   unsigned long timeMillis =
//       convertDateTimeToMillis(day, month, year, hour, minute, second);

//   // model.setUTCYear(2023);
//   // model.setUTCMonth(9);
//   // model.setUTCDay(2);
//   // model.setUTCHour(10);
//   // model.setUTCMinute(0);
//   // model.setUTCSecond(0);

//   model.setAltEncoderStepsPerRevolution(36000); // 100 ticks per degree
//   model.setAzEncoderStepsPerRevolution(36000);

//   double star1AltAxis = 83.87; // degrees
//   double star1AzmAxis = 99.25; // anticlockwise)

//   // start pointing exactly at star, if zero position of encoders is north
//   and
//   // flat.
//   model.setEncoderValues(star1AltAxis * 100, 36000 - star1AzmAxis * 100);

//   // time of observation
//   double star1Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 27,
//   56);

//   double star1RAHours = Ephemeris::hoursMinutesSecondsToFloatingHours(0, 7,
//   54); double star1RADegrees = 360 * star1RAHours / 24.0;
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 1.97498552, star1RADegrees,
//   "star1ra");

//   double star1Dec = 29.038;

//   log("Star 1: ");
//   log("    \t\t\t\t\talt: %lf\taz: %lf", star1AltAxis, star1AzmAxis);
//   log("       \t\t\t\t\tra: %lf\tdec: %lf", star1RADegrees, star1Dec);
//   log("Star time: %ld ", timeMillis);

//   model.syncPositionRaDec(star1RADegrees, star1Dec, timeMillis);
//   model.addReferencePoint();

//   double star2AltAxis = 35.04;  // degrees
//   double star2AzmAxis = 310.98; // anticlockwise)

//   model.setEncoderValues(star2AltAxis * 100, 36000 - star2AzmAxis * 100);

//   double star2Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 37,
//   02);

//   double star2RAHours =
//       Ephemeris::hoursMinutesSecondsToFloatingHours(2, 21, 45);
//   // star2RAHours = star2RAHours - (star2Time - star1Time);//adjust for time
//   // shift
//   double star2RADegress = 360 * star2RAHours / 24.0;

//   double star2Dec = 89.222;

//   log("Star 2: ");
//   log("    \t\t\t\t\talt: %lf\taz: %lf", star2AltAxis, star2AzmAxis);
//   log("       \t\t\t\t\tra: %lf\tdec: %lf", star2RADegress, star2Dec);
//   log("Star time: %ld ", timeMillis + 546000);

//   // 546000 milliseconds since first sync
//   model.syncPositionRaDec(star2RADegress, star2Dec, timeMillis + 546000);
//   //  timeMillis + 546000); // time passed in millis
//   model.addReferencePoint();

//   // If you want to aim the telescope at b Cet (ra = 0h43m07s, dec = -18.038)
//   // This calculated telescope coordinates is very close to the measured
//   // telescope coordinates,
//   // j = 130.46, , q = 37.67o

//   // set encoders to ra/dec values from example
//   double star3AltAxis = 37.67;  // degrees
//   double star3AzmAxis = 130.21; // anticlockwise)

//   model.setEncoderValues(star3AltAxis * 100, 36000 - star3AzmAxis * 100);

//   double star3Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 52,
//   12);

//   double star3RAHours =
//       Ephemeris::hoursMinutesSecondsToFloatingHours(0, 43, 07);

//   double star3RADegrees = 360 * star3RAHours / 24.0;

//   double star3Dec = -18.038;

//   log("Star 3: ");
//   log("    \t\t\t\t\talt: %lf\taz: %lf", star3AltAxis, star3AzmAxis);
//   log("       \t\t\t\t\tra: %lf\tdec: %lf", star3RADegrees, star3Dec);
//   log("Star time: %ld ", timeMillis + 1456000);

//   // 1456000 milliseconds since first sync
//   model.calculateCurrentPosition(timeMillis + 1456000);

//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, model.getDecCoord(),
//   "dec"); TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3RAHours,
//   model.getRACoord(),
//                                    "ra");
// }

void test_odd_times(void) {
  TelescopeModel model;
  model.setLatitude(-34.049120);
  model.setLongitude(151.042100);
  // Choose a date and time (UTC)
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  //   unsigned long timeMillis =
  //       convertDateTimeToMillis(day, month, year, hour, minute, second);

  model.setAltEncoderStepsPerRevolution(108229);
  model.setAzEncoderStepsPerRevolution(-30000);

  model.setEncoderValues(0, 0);
  // arcturus
  model.syncPositionRaDec(0, 0, star1Time);
  // model.calculateCurrentPosition();

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1, 0.0, model.getRACoord(), "ra");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.1, 0.0, model.getDecCoord(), "dec");
}

void test_az_encoder_calibration(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(360);
  model.setAzEncoderStepsPerRevolution(360);
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  Ephemeris::setLocationOnEarth(-34.0493, 151.0494);
  Ephemeris::flipLongitude(false);

  //   unsigned long timeMillis =
  //       convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);

  model.setEncoderValues(0, 0);
  HorizCoord startAltAz = HorizCoord(0, 0);
  EqCoord startEQ = EqCoord(startAltAz, star1Time);
  //   EqCoord oldEq = EqCoord(14.28644, 18.97959);
  model.syncPositionRaDec(startEQ.getRAInHours(), startEQ.getDecInDegrees(),
                          star1Time);
  // model.calculateCurrentPosition();
  //   model.saveEncoderCalibrationPoint(); // Save this point

  // Generate new Alt/Az values from a known move

  //   HorizCoord newAltAz=HorizCoord(model.getAltCoord(),model.getAzCoord());
  double azRotation = 10;
  HorizCoord newAltAz = startAltAz.addOffset(0, azRotation);

  EqCoord newEq = EqCoord(newAltAz, star1Time);
  log("Distance in eq coords %lf", newEq.calculateDistanceInDegrees(startEQ));

  // Set the new position using the converted RA/DEC and simulate new encoder
  // values
  double aziEncoderIncrease = 10000;
  model.setEncoderValues(0, aziEncoderIncrease);
  model.syncPositionRaDec(newEq.getRAInHours(), newEq.getDecInDegrees(),
                          star1Time);
  // Here the azimuth encoder value has increased by 100
  // model.calculateCurrentPosition();
  //   model.saveEncoderCalibrationPoint(); // Save this point

  long calculatedSteps = model.calculatedAziEncoderRes;

  float epsilon = 1.0; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
      epsilon, aziEncoderIncrease / azRotation * 360.0, calculatedSteps,
      "Azimuth Encoder Steps per Revolution");
}

void test_alt_encoder_calibration(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(360);
  model.setAzEncoderStepsPerRevolution(360);
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  Ephemeris::setLocationOnEarth(-34.0493, 151.0494);
  Ephemeris::flipLongitude(false);

  //   unsigned long timeMillis =
  //       convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);

  model.setEncoderValues(0, 0);

  HorizCoord startAltAz = HorizCoord(0, 0);
  EqCoord startEq = EqCoord(startAltAz, star1Time);
  model.syncPositionRaDec(startEq.getRAInHours(), startEq.getDecInDegrees(),
                          star1Time);
  // model.calculateCurrentPosition();
  //   model.saveEncoderCalibrationPoint(); // Save this point

  // Generate new Alt/Az values from a known move

  //   HorizCoord newAltAz=HorizCoord(model.getAltCoord(),model.getAzCoord());
  double altRotation = 10;
  HorizCoord newAltAz = startAltAz.addOffset(altRotation, 0);

  EqCoord newEq = EqCoord(newAltAz, star1Time);

  // Set the new position using the converted RA/DEC and simulate new encoder
  // values
  long altEncoderIncrease = 1000;
  log("===second sync");
  model.setEncoderValues(altEncoderIncrease, 0);
  model.syncPositionRaDec(newEq.getRAInHours(), newEq.getDecInDegrees(),
                          star1Time);
  // Here the azimuth encoder value has increased by 100
  // model.calculateCurrentPosition();
  //   model.saveEncoderCalibrationPoint(); // Save this point

  long calculatedSteps = model.calculatedAltEncoderRes;
  float epsilon = 1.0; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
      epsilon, -altEncoderIncrease / altRotation * 360.0, calculatedSteps,
      "Alt Encoder Steps per Revolution");
}

void test_az_encoder_wraparound(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(360);
  model.setAzEncoderStepsPerRevolution(360);
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  //   unsigned long timeMillis =
  //       convertDateTimeToMillis(day, month, year, hour, minute, second);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);

  model.setEncoderValues(0, 0);
  model.syncPositionRaDec(14.28644, 18.9795, star1Time);
  // model.calculateCurrentPosition();
  //   model.saveEncoderCalibrationPoint(); // Save this point

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
  model.syncPositionRaDec(newEq.ra, newEq.dec, star1Time);
  model.setEncoderValues(
      0, 17000); // Here the azimuth encoder value has increased by 100
  // model.calculateCurrentPosition();
  //   model.saveEncoderCalibrationPoint(); // Save this point

  long calculatedSteps = model.calculatedAziEncoderRes;

  float epsilon = 1.0; // or whatever small value you're comfortable with
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, 36000, calculatedSteps,
                                   "Azimuth Encoder Steps per Revolution");
}

// void test_alt_encoder_negative_move(void) {
//   TelescopeModel model;
//   model.setAltEncoderStepsPerRevolution(9999);
//   model.setAzEncoderStepsPerRevolution(9999);
//   unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
//                second = 33;
//   unsigned long timeMillis =
//       convertDateTimeToMillis(day, month, year, hour, minute, second);
//   model.setLatitude(-34.0493);
//   model.setLongitude(151.0494);

//   model.setEncoderValues(100, 0);
//   model.syncPositionRaDec(14.28644, 18.97959, timeMillis);
//   // sets alt az
//   // model.calculateCurrentPosition();

//   //   model.saveEncoderCalibrationPoint(); // Save this point

//   // Generate new Alt/Az values from a known move
//   HorizontalCoordinates newAltAz;
//   newAltAz.alt = model.getAltCoord() - 1.0; // Assuming a 1 degree move in
//   Alt newAltAz.azi = model.getAzCoord();        // Keep azimuth same

//   // Convert the new Alt/Az to Equatorial using the hardcoded time values
//   EquatorialCoordinates newEq =
//       Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(newAltAz, 2,
//       9,
//                                                                 2023, 10, 0,
//                                                                 0);

//   // Set the new position using the converted RA/DEC and simulate new encoder
//   // values
//   model.syncPositionRaDec(newEq.ra, newEq.dec, timeMillis);

//   model.setEncoderValues(
//       0, 0); // Here the altitude encoder value has decreased by 100
//   // model.calculateCurrentPosition();
//   //   model.saveEncoderCalibrationPoint(); // Save this point

//   long calculatedSteps = model.calculateAltEncoderStepsPerRevolution();
//   // Use
//   float epsilon = 1.0; // or whatever small value you're comfortable with
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(epsilon, 36000, calculatedSteps,
//                                    "Altitude Encoder Steps per Revolution");
// }

// void test_two_star_alignment_takeshi_example() {

//   // http: // takitoshimi.starfree.jp/matrix/matrix_method_rev_e.pdf

//   // Default alignment for alt az in south
//   //  alignment.addReference(0, 0, M_PI, 0);//radians
//   //  alignment.addReference(0, 0, 180, 0); //degrees

//   // alignment.addReference(0, M_PI_2, M_PI, M_PI_2);//radians
//   // alignment.addReference(0, 90, 180,90); //degreess

//   // alignment.calculateThirdReference();
//   CoordConv alignment;

//   double star1AltAxis = 83.87;
//   double star1AzmAxis = 99.25; // anticlockwise)

//   double star1RAHours = Ephemeris::hoursMinutesSecondsToFloatingHours(0, 7,
//   54); double star1RADegrees = 360 * star1RAHours / 24.0;
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 1.97498552, star1RADegrees,
//   "star1ra"); double star1Time =
//   Ephemeris::hoursMinutesSecondsToFloatingHours(21, 27, 56); double star1Dec
//   = 29.038; alignment.addReferenceDeg(star1RADegrees, star1Dec, star1AzmAxis,
//                             star1AltAxis);

//   double star2AltAxis = 35.04;
//   double star2AzmAxis = 310.98; // anticlockwise)
//   double star2RAHours =
//       Ephemeris::hoursMinutesSecondsToFloatingHours(2, 21, 45);

//   double star2Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 37,
//   02);
//   // time travel
//   star2RAHours = star2RAHours - (star2Time - star1Time);

//   double star2RADegress = 360 * star2RAHours / 24.0;

//   double star2Dec = 89.222;
//   alignment.addReferenceDeg(star2RADegress, star2Dec, star2AzmAxis,
//                             star2AltAxis);
//   int i = alignment.getRefs();
//   std::cout << "===Alignment ref: " << i << "\n\r";
//   alignment.calculateThirdReference();

//   double star3RAHours =
//       Ephemeris::hoursMinutesSecondsToFloatingHours(0, 43, 07);

//   double star3Dec = -18.038;

//   double star3Time = Ephemeris::hoursMinutesSecondsToFloatingHours(21, 52,
//   12);
//   // time travel
//   // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, star3RA, "star3RA");
//   star3RAHours = star3RAHours - (star3Time - star1Time);

//   double star3RADegrees = 360 * star3RAHours / 24.0;
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 4.712482, star3RADegrees, "ra");
//   // star3RA = fmod(star3RA, 360);
//   // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, star3RA, "star3RA");

//   double outaxis1;
//   double outaxis2;
//   alignment.toInstrumentDeg(outaxis1, outaxis2, star3RADegrees, star3Dec);

//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, outaxis2, "alt");
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 130.21, outaxis1, "az");

//   double outra, outdec;
//   alignment.toReferenceDeg(outra, outdec, outaxis1, outaxis2);

//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3RADegrees, outra, "ra");
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, outdec, "dec");
//   // 3.7002 degree error
//   // 14.8 minutes or 888 seconds 14 m 48 seconds
// }
/**
 * Test two star alignment, using wrapper classes.
 */
void test_two_star_alignment_mylocation_wrappers() {

  // Note that in southern hemisphere, we
  // a) measure azimuth in an anticlockwise direction, so
  //    no need to do 360-az  (transformation expects anticlockwise azi)
  // b) measure alt negative to what transformation expects.

  // http: // takitoshimi.starfree.jp/matrix/matrix_method_rev_e.pdf

  CoordConv alignment;
  alignment.setNorthernHemisphere(false);

  HorizCoord vegaAltAz;
  vegaAltAz.setAlt(17, 9, 19.5);
  vegaAltAz.setAzi(357, 13, 18.8);

  //   TakiHorizCoord vegaTaki = TakiHorizCoord(vegaAltAz, false);
  //   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, -17.1, vegaTaki.altAngle,
  //   "takitalt"); TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15,357.2,
  //   vegaTaki.aziAngle, "takitalt");

  EqCoord vegaRaDec;
  vegaRaDec.setRAInHours(18, 37, 43.68);
  vegaRaDec.setDecInDegrees(38, 48, 9.2);

  alignment.addReferenceCoord(vegaAltAz, vegaRaDec);

  // fomalhaut
  HorizCoord fomalhautAltAz;

  fomalhautAltAz.setAlt(37, 36, 24.3);
  fomalhautAltAz.setAzi(103, 17, 9.4);
  //   TakiHorizCoord fomalhautTaki = TakiHorizCoord(fomalhautAltAz, false);

  EqCoord fomalhautRaDec;
  fomalhautRaDec.setRAInHours(22, 58, 56.46);
  fomalhautRaDec.setDecInDegrees(-29, 29, 47.7);

  alignment.addReferenceCoord(fomalhautAltAz, fomalhautRaDec);

  alignment.calculateThirdReference();

  // altair
  EqCoord altairRaDecExpected;
  altairRaDecExpected.setRAInHours(19, 51, 54.9);
  altairRaDecExpected.setDecInDegrees(8, 55, 38.5);

  HorizCoord altairAltAziExpected;
  altairAltAziExpected.setAlt(44, 33, 25.8);
  altairAltAziExpected.setAzi(21, 55, 36.3);

  HorizCoord altairAltAz = alignment.toInstrumentCoord(altairRaDecExpected);

  //   double outaxis1;
  //   double outaxis2;
  //   alignment.toReferenceDeg(outaxis1, outaxis2, star3RADegrees, star3Dec);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, altairAltAziExpected.altInDegrees,
                                   altairAltAz.altInDegrees, "alt");
  // this is a big delta, not sure why.
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.55, altairAltAziExpected.aziInDegrees,
                                   altairAltAz.aziInDegrees, "az");

  EqCoord altairRaDec = alignment.toReferenceCoord(altairAltAz);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, altairRaDecExpected.getDecInDegrees(),
                                   altairRaDec.getDecInDegrees(), "dec");
  // this is a big delta, not sure why.
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.55, altairRaDecExpected.getRAInDegrees(),
                                   altairRaDec.getRAInDegrees(), "ra");
}

void test_two_star_alignment_mylocation_wrappers_offset() {

  // test what happens if there is an offset on the encoders
  // at the start. We assume always that alt=0 on encoders
  // is horizontal: the alignment model can handle azi rotation
  // but not alt.

  double altOffset = 0;
  double aziOffset = 15;

  CoordConv alignment;
  alignment.setNorthernHemisphere(false);

  HorizCoord vegaAltAz;
  vegaAltAz.setAlt(17, 9, 19.5);
  vegaAltAz.setAzi(357, 13, 18.8);
  vegaAltAz = vegaAltAz.addOffset(altOffset, aziOffset);

  //   TakiHorizCoord vegaTaki = TakiHorizCoord(vegaAltAz, false);
  //   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, -17.1, vegaTaki.altAngle,
  //   "takitalt"); TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15,357.2,
  //   vegaTaki.aziAngle, "takitalt");

  EqCoord vegaRaDec;
  vegaRaDec.setRAInHours(18, 37, 43.68);
  vegaRaDec.setDecInDegrees(38, 48, 9.2);

  alignment.addReferenceCoord(vegaAltAz, vegaRaDec);

  // fomalhaut
  HorizCoord fomalhautAltAz;

  fomalhautAltAz.setAlt(37, 36, 24.3);
  fomalhautAltAz.setAzi(103, 17, 9.4);

  fomalhautAltAz = fomalhautAltAz.addOffset(altOffset, aziOffset);
  //   TakiHorizCoord fomalhautTaki = TakiHorizCoord(fomalhautAltAz, false);

  EqCoord fomalhautRaDec;
  fomalhautRaDec.setRAInHours(22, 58, 56.46);
  fomalhautRaDec.setDecInDegrees(-29, 29, 47.7);

  alignment.addReferenceCoord(fomalhautAltAz, fomalhautRaDec);

  alignment.calculateThirdReference();

  // altair
  EqCoord altairRaDecExpected;
  altairRaDecExpected.setRAInHours(19, 51, 54.9);
  altairRaDecExpected.setDecInDegrees(8, 55, 38.5);

  HorizCoord altairAltAziExpected;
  altairAltAziExpected.setAlt(44, 33, 25.8);
  altairAltAziExpected.setAzi(21, 55, 36.3);

  altairAltAziExpected = altairAltAziExpected.addOffset(altOffset, aziOffset);

  EqCoord altairRaDec = alignment.toReferenceCoord(altairAltAziExpected);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, altairRaDecExpected.getDecInDegrees(),
                                   altairRaDec.getDecInDegrees(), "dec");
  // this is a big delta, not sure why.
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.55, altairRaDecExpected.getRAInDegrees(),
                                   altairRaDec.getRAInDegrees(), "ra");

  HorizCoord altairAltAz = alignment.toInstrumentCoord(altairRaDecExpected);
  // altairAltAz = altairAltAz.addOffset(altOffset, aziOffset);
  //   double outaxis1;
  //   double outaxis2;
  //   alignment.toReferenceDeg(outaxis1, outaxis2, star3RADegrees,
  //   star3Dec);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, altairAltAziExpected.altInDegrees,
                                   altairAltAz.altInDegrees, "alt");
  // this is a big delta, not sure why.
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.55, altairAltAziExpected.aziInDegrees,
                                   altairAltAz.aziInDegrees, "az");
}

// void test_two_star_alignment_mylocation() {

//   // Note that in southern hemisphere, we
//   // a) measure azimuth in an anticlockwise direction, so
//   //    no need to do 360-az  (transformation expects anticlockwise azi)
//   // b) measure alt negative to what transformation expects.

//   // http: // takitoshimi.starfree.jp/matrix/matrix_method_rev_e.pdf

//   CoordConv alignment;

//   // star 1: Vega. Note negative as southern hemisphhere
//   double star1AltAxis =
//       -Ephemeris::degreesMinutesSecondsToFloatingDegrees(17, 9, 19.5);

//   // note no 360-...
//   double star1AzmAxis = Ephemeris::degreesMinutesSecondsToFloatingDegrees(
//       357, 13, 18.8); // anticlockwise)

//   double star1RAHours =
//       Ephemeris::hoursMinutesSecondsToFloatingHours(18, 37, 43.68);
//   double star1RADegrees = 360 * star1RAHours / 24.0;
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 279.432, star1RADegrees, "star1ra");

//   double star1Dec =
//       Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 48, 9.2);
//   alignment.addReferenceDeg(star1AzmAxis, star1AltAxis, star1RADegrees,
//                             star1Dec);

//   // fomalhaut
//   double star2AltAxis =
//       -Ephemeris::degreesMinutesSecondsToFloatingDegrees(37, 36, 24.3);
//   // anticlockwise
//   double star2AzmAxis =
//       Ephemeris::degreesMinutesSecondsToFloatingDegrees(103, 17, 9.4);

//   double star2RAHours =
//       Ephemeris::hoursMinutesSecondsToFloatingHours(22, 58, 56.46);

//   double star2RADegress = 360 * star2RAHours / 24.0;

//   double star2Dec =
//       Ephemeris::degreesMinutesSecondsToFloatingDegrees(-29, 29, 47.7);

//   alignment.addReferenceDeg(star2AzmAxis, star2AltAxis, star2RADegress,
//                             star2Dec);

//   alignment.calculateThirdReference();

//   // altair
//   double star3RAHours =
//       Ephemeris::hoursMinutesSecondsToFloatingHours(19, 51, 54.9);
//   double star3RADegrees = 360 * star3RAHours / 24.0;

//   double star3Dec =
//       Ephemeris::degreesMinutesSecondsToFloatingDegrees(8, 55, 38.5);

//   // double star3Time =
//   //     Ephemeris::hoursMinutesSecondsToFloatingHours(19, 51, 54.96);
//   // time travel
//   // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, star3RA, "star3RA");
//   double star3AltAxis =
//       -Ephemeris::degreesMinutesSecondsToFloatingDegrees(44, 33, 25.8);
//   // anticlockwise
//   double star3AzmAxis =
//       Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3);

//   std::cout << "Star 3 RA: " << star3RADegrees << "\n\r";
//   std::cout << "Star 3 Dec: " << star3Dec << "\n\r";
//   std::cout << "Star 3 alt: " << star3AltAxis << "\n\r";
//   std::cout << "Star 3 alt: " << star3AzmAxis << "\n\r";

//   // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 4.712482, star3RADegrees, "ra");
//   // star3RA = fmod(star3RA, 360);
//   // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 37.67, star3RA, "star3RA");

//   double outaxis1;
//   double outaxis2;
//   alignment.toReferenceDeg(outaxis1, outaxis2, star3RADegrees, star3Dec);

//   std::cout << "Calced Alt axis: " << outaxis2 << "\n\r";
//   std::cout << "Calced Azi axis: " << outaxis1 << "\n\r";

//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3AltAxis, outaxis2, "alt");
//   // this is a big delta, not sure why.
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.55, star3AzmAxis, outaxis1, "az");

//   double outra, outdec;
//   alignment.toInstrumentDeg(outra, outdec, star3AzmAxis, star3AltAxis);

//   outra = outra + 360; // wtf. need to normalise

//   std::cout << "Calced dec: " << outdec << "\n\r";
//   std::cout << "Calced ra: " << outra << "\n\r";
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, outdec, "dec");
//   TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, star3RADegrees, outra, "ra");

//   // 3.7002 degree error
//   // 14.8 minutes or 888 seconds 14 m 48 seconds
// }

void test_telescope_model_mylocation_with_offset() {
  log("======test_telescope_model_mylocation_with_offset=====");

  TelescopeModel model;
  // add arbitary value to encoders. Should be handled by first sync.
  int altEncoderOffset = 5000;
  int aziEncoderOffset = 5000;

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  //   unsigned long timeMillis =
  //       convertDateTimeToMillis(day, month, year, hour, minute, second);

  // model.setUTCYear(2023);
  // model.setUTCMonth(9);
  // model.setUTCDay(2);
  // model.setUTCHour(10);
  // model.setUTCMinute(0);
  // model.setUTCSecond(0);

  model.setAltEncoderStepsPerRevolution(36000); // 100 ticks per degree
  model.setAzEncoderStepsPerRevolution(36000);

  // star 1: Vega
  double star1AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(17, 9, 19.5);
  // anticlockwise
  double star1AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(357, 13, 18.8);

  // start pointing exactly at star, if zero position of encoders is north and
  // flat.
  model.setEncoderValues(star1AltAxis * 100 + altEncoderOffset,
                         star1AzmAxis * 100 + aziEncoderOffset);

  // time s::hoursMinutesSecondsToFloatingHours(21, 27, 56);of observation
  //   double star1Time = timeMillis;

  double star1RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(18, 37, 43.68);
  double star1RADegrees = 360 * star1RAHours / 24.0;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 279.432, star1RADegrees, "star1ra");

  double star1Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 48, 9.2);

  log("Star 1: Vega");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star1AltAxis, star1AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star1RADegrees, star1Dec);
  //   log("Star time: %ld ", timeMillis);

  model.syncPositionRaDec(star1RAHours, star1Dec, star1Time);
  //   model.addReferencePoint();
  model.calculateCurrentPosition(star1Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1RAHours, model.getRACoord(),
                                   "calculated ra");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1Dec, model.getDecCoord(),
                                   "calculated dec");

  // fomalhut
  double star2AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(37, 36, 24.3);
  // anticlockwise
  double star2AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(103, 17, 9.4);

  model.setEncoderValues(star2AltAxis * 100 + altEncoderOffset,
                         star2AzmAxis * 100 + aziEncoderOffset);

  //   double star2Time = timeMillis; // use same time for now

  double star2RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(22, 58, 56.46);
  // star2RAHours = star2RAHours - (star2Time - star1Time);//adjust for time
  // shift
  double star2RADegress = 360 * star2RAHours / 24.0;

  double star2Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(-29, 29, 47.7);

  log("Star 2: Fomalhaut");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star2AltAxis, star2AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star2RADegress, star2Dec);
  //   log("Star time: %ld ", timeMillis);

  // 546000 milliseconds since first sync
  model.syncPositionRaDec(star2RAHours, star2Dec, star1Time);
  //  timeMillis + 546000); // time passed in millis
  //   model.addReferencePoint();

  // If you want to aim the telescope at b Cet (ra = 0h43m07s, dec = -18.038)
  // This calculated telescope coordinates is very close to the measured
  // telescope coordinates,
  // j = 130.46, , q = 37.67o

  // set encoders to ra/dec values from example

  // altair
  double star3AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(44, 33, 25.8);
  // anticlockwise
  double star3AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3);
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
  //     0.01, 21.92675 ,
  //     Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3),
  //     "???");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 338.0732, star3AzmAxis,
  //                                  "star3AzmAxis");
  model.setEncoderValues(star3AltAxis * 100 + altEncoderOffset,
                         star3AzmAxis * 100 + aziEncoderOffset);

  //   double star3Time = timeMillis;

  double star3RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(19, 51, 54.96);

  double star3RADegrees = 360 * star3RAHours / 24.0;

  double star3Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(8, 55, 38.5);

  log("Star 3: Altair");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star3AltAxis, star3AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star3RADegrees, star3Dec);
  //   log("Star time: %ld ", timeMillis);

  // 1456000 milliseconds since first sync
  model.calculateCurrentPosition(star1Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, star3RAHours, model.getRACoord(), "ra");
}

void test_eq_to_horizontal_vega(void) {
  Ephemeris::setLocationOnEarth(-34.0493, 151.0494);
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;

  //   // Set location on earth for horizontal coordinates transformations
  //   Ephemeris::setLocationOnEarth(-34.0, 2.0, 44.0, // Lat: 48°50'11"
  //                                 151.0, 3.0, 3.0); // Lon: -2°20'14"
  Ephemeris::
      // East is negative and West is positive
      Ephemeris::flipLongitude(false);

  // Set altitude to improve rise and set precision
  //   Ephemeris::setAltitude(110);

  EquatorialCoordinates eqCoord;

  eqCoord.ra = Ephemeris::hoursMinutesSecondsToFloatingHours(18, 37, 43.68);
  ;
  eqCoord.dec = Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 48, 9.2);
  ; // +89° 15′ 51″

  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          eqCoord, day, month, year, hour, minute, second);
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(17.09805, altAzCoord.alt, "Alt");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(357.6236, altAzCoord.azi, "Az");
  //   TEST_ASSERT_EQUAL_FLOAT_MESSAGE(
  //       Ephemeris::degreesMinutesSecondsToFloatingDegrees(6, 21, 37.7),
  //       altAzCoord.alt, "Alt");
  //   TEST_ASSERT_EQUAL_FLOAT_MESSAGE(
  //       Ephemeris::degreesMinutesSecondsToFloatingDegrees(298, 3, 3.7),
  //       altAzCoord.azi, "Az");
}

void test_continuity() {
  log("======test_continuity=====");
  // Simulate spinning the scope around in a spiral.
  // No asserts yet (TODO)
  // Just checks for discontinuities due to wierd floating point issues.

  TelescopeModel model;

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  model.setAltEncoderStepsPerRevolution(36000); // 100 ticks per degree
  model.setAzEncoderStepsPerRevolution(36000);

  // star 1: Vega
  HorizCoord vega;
  vega.setAlt(17, 9, 19.5);
  vega.setAzi(357, 13, 18.8);

  model.setEncoderValues(vega.altInDegrees * 100, vega.aziInDegrees * 100);
  double star1RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(18, 37, 43.68);
  double star1RADegrees = 360 * star1RAHours / 24.0;
  double star1Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 48, 9.2);

  model.syncPositionRaDec(star1RAHours, star1Dec, star1Time);
  model.calculateCurrentPosition(star1Time);

  double star2AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(37, 36, 24.3);

  double star2AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(103, 17, 9.4);
  model.setEncoderValues(star2AltAxis * 100, star2AzmAxis * 100);

  double star2RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(22, 58, 56.46);
  // star2RAHours = star2RAHours - (star2Time - star1Time);//adjust for time
  // shift
  double star2RADegress = 360 * star2RAHours / 24.0;

  double star2Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(-29, 29, 47.7);

  TimePoint star2time = addSecondsToTime(star1Time, 180);
  model.syncPositionRaDec(star2RAHours, star2Dec, star2time);
  model.calculateCurrentPosition(star2time);

  int timeShiftSeconds = 90;
  TimePoint positionTime = addSecondsToTime(star2time, timeShiftSeconds);
  EqCoord previousEqPosition;
  for (size_t alt = 0; alt < 90; alt += 1) {
    for (size_t azi = 0; azi < 360; azi += 1) {
      model.setEncoderValues(alt * 100, azi * 100);
      model.calculateCurrentPosition(positionTime);
      double currentRa = model.getRACoord() * 15.0;
      double currentDec = model.getDecCoord();
      double distance = model.currentEqPosition.calculateDistanceInDegrees(
          previousEqPosition);

      if (distance > 2)
        log("Alt: %d\tazi : %d\t\t\t\tRA: %lf\tDec: %lf\tDistance: %lf ", alt,
            azi, currentRa, currentDec, distance);
      previousEqPosition = model.currentEqPosition;
      positionTime = addSecondsToTime(positionTime, timeShiftSeconds);
    }
  }
}

void test_telescope_model_mylocation() {
  log("======test_telescope_model_mylocation=====");

  TelescopeModel model;

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  model.setAltEncoderStepsPerRevolution(36000); // 100 ticks per degree
  model.setAzEncoderStepsPerRevolution(36000);

  // star 1: Vega
  HorizCoord vega;
  vega.setAlt(17, 9, 19.5);
  vega.setAzi(357, 13, 18.8);

  // start pointing exactly at star, if zero position of encoders is north
  // and flat.
  model.setEncoderValues(vega.altInDegrees * 100, vega.aziInDegrees * 100);

  // time s::hoursMinutesSecondsToFloatingHours(21, 27, 56);of observation

  double star1RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(18, 37, 43.68);
  double star1RADegrees = 360 * star1RAHours / 24.0;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 279.432, star1RADegrees, "star1ra");

  double star1Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 48, 9.2);

  log("Star 1: Vega");
  log("    \t\t\t\t\talt: %lf\taz: %lf", vega.altInDegrees, vega.aziInDegrees);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star1RADegrees, star1Dec);
  //   log("Star time: %ld ", timeMillis);

  model.syncPositionRaDec(star1RAHours, star1Dec, star1Time);
  //   model.addReferencePoint();

  model.calculateCurrentPosition(star1Time);

  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1RAHours, model.getRACoord(),
  //                                  "calculated ra");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1Dec, model.getDecCoord(),
  //                                  "calculated dec");

  // fomalhut

  double star2AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(37, 36, 24.3);
  // anticlockwise
  double star2AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(103, 17, 9.4);

  model.setEncoderValues(star2AltAxis * 100, star2AzmAxis * 100);

  // use same time for now

  double star2RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(22, 58, 56.46);
  // star2RAHours = star2RAHours - (star2Time - star1Time);//adjust for time
  // shift
  double star2RADegress = 360 * star2RAHours / 24.0;

  double star2Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(-29, 29, 47.7);

  log("Star 2: Fomalhaut");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star2AltAxis, star2AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star2RADegress, star2Dec);
  //   log("Star time: %ld ", timeMillis);

  // 546000 milliseconds since first sync
  model.syncPositionRaDec(star2RAHours, star2Dec, star1Time);
  //  timeMillis + 546000); // time passed in millis
  //   model.addReferencePoint();

  model.calculateCurrentPosition(star1Time);

  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star2RAHours, model.getRACoord(),
  //                                  "calculated ra");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star2Dec, model.getDecCoord(),
  //                                  "calculated dec");
  // If you want to aim the telescope at b Cet (ra = 0h43m07s, dec = -18.038)
  // This calculated telescope coordinates is very close to the measured
  // telescope coordinates,
  // j = 130.46, , q = 37.67o

  // set encoders to ra/dec values from example

  // altair
  double star3AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(44, 33, 25.8);
  // anticlockwise
  double star3AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3);
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
  //     0.01, 21.92675 ,
  //     Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3),
  //     "???");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 338.0732, star3AzmAxis,
  //                                  "star3AzmAxis");
  model.setEncoderValues(star3AltAxis * 100, star3AzmAxis * 100);

  double star3RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(19, 51, 54.96);

  double star3RADegrees = 360 * star3RAHours / 24.0;

  double star3Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(8, 55, 38.5);

  log("Star 3: Altair");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star3AltAxis, star3AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star3RADegrees, star3Dec);
  //   log("Star time: %ld ", timeMillis);

  // 1456000 milliseconds since first sync

  model.syncPositionRaDec(star3RAHours, star3Dec, star1Time);
  model.calculateCurrentPosition(star1Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, star3RAHours, model.getRACoord(), "ra");

  // alnair
  double star4AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(50, 32, 52.9);

  // anticlockwise
  double star4AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(124, 21, 54);
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
  //     0.01, 21.92675 ,
  //     Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3),
  //     "???");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 338.0732, star3AzmAxis,
  //                                  "star3AzmAxis");
  model.setEncoderValues(star4AltAxis * 100, star4AzmAxis * 100);

  double star4RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(22, 9, 42.54);
  double star4RADegrees = 360 * star4RAHours / 24.0;

  double star4Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(-46, 50, 46.2);

  log("Star 4: Alnair");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star4AltAxis, star4AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star4RADegrees, star4Dec);

  model.syncPositionRaDec(star4RAHours, star4Dec, star1Time);
  model.calculateCurrentPosition(star1Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star4Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, star4RAHours, model.getRACoord(), "ra");

  //check what happens ten minutes later. Scope is pointing to same spot,
  //but earth should have turned. So same alt az should give ra ten minutes back
  long timeShiftHours =1;
  TimePoint oneHourLater =
      addSecondsToTime(star1Time, timeShiftHours * 60 * 60);
  double expectedRAHours = model.getRACoord()-timeShiftHours;

  model.calculateCurrentPosition(oneHourLater);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star4Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, expectedRAHours, model.getRACoord(),
                                   "ra");

  //Now we'll do a sycnc at this same point.

  //Scope is pointing to this ra/dec at this time, 
  //as per previous assert
  //so there should be no offset applied here

  model.syncPositionRaDec(model.getRACoord(),model.getDecCoord(),oneHourLater);
  model.calculateCurrentPosition(oneHourLater);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star4Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, expectedRAHours, model.getRACoord(),
                                   "ra");

  //now introduce an error of one hour ra
  model.syncPositionRaDec(model.getRACoord()+1.0, model.getDecCoord(),
                          oneHourLater);
  model.calculateCurrentPosition(oneHourLater);
  //model should compensate
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star4Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, expectedRAHours+1.0, model.getRACoord(),
                                   "ra");
}

void test_telescope_model_mylocation_with_tilt() {
  log("======test_telescope_model_mylocation=====");
  // simulates platform not being flat by changing lat long.
  // does model compensate?
  TelescopeModel model;

  model.setLatitude(-34.0455);
  // model.setLatitude(-24.0493); //<---- 10
  model.setLongitude(151.0494); //<---- 10
  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;
  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);

  model.setAltEncoderStepsPerRevolution(36000); // 100 ticks per degree
  model.setAzEncoderStepsPerRevolution(36000);

  // star 1: Vega
  HorizCoord vega;
  vega.setAlt(17, 9, 19.5);
  vega.setAzi(357, 13, 18.8);

  // start pointing exactly at star, if zero position of encoders is north
  // and flat.
  model.setEncoderValues(vega.altInDegrees * 100, vega.aziInDegrees * 100);

  // time s::hoursMinutesSecondsToFloatingHours(21, 27, 56);of observation

  double star1RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(18, 37, 43.68);
  double star1RADegrees = 360 * star1RAHours / 24.0;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 279.432, star1RADegrees, "star1ra");

  double star1Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 48, 9.2);

  log("Star 1: Vega");
  log("    \t\t\t\t\talt: %lf\taz: %lf", vega.altInDegrees, vega.aziInDegrees);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star1RADegrees, star1Dec);
  //   log("Star time: %ld ", timeMillis);

  model.syncPositionRaDec(star1RAHours, star1Dec, star1Time);
  //   model.addReferencePoint();

  model.calculateCurrentPosition(star1Time);

  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1RAHours, model.getRACoord(),
  //                                  "calculated ra");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1Dec, model.getDecCoord(),
  //                                  "calculated dec");

  // fomalhut

  double star2AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(37, 36, 24.3);
  // anticlockwise
  double star2AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(103, 17, 9.4);

  model.setEncoderValues(star2AltAxis * 100, star2AzmAxis * 100);

  // use same time for now

  double star2RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(22, 58, 56.46);
  // star2RAHours = star2RAHours - (star2Time - star1Time);//adjust for time
  // shift
  double star2RADegress = 360 * star2RAHours / 24.0;

  double star2Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(-29, 29, 47.7);

  log("Star 2: Fomalhaut");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star2AltAxis, star2AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star2RADegress, star2Dec);
  //   log("Star time: %ld ", timeMillis);

  // 546000 milliseconds since first sync
  model.syncPositionRaDec(star2RAHours, star2Dec, star1Time);
  //  timeMillis + 546000); // time passed in millis
  //   model.addReferencePoint();

  model.calculateCurrentPosition(star1Time);

  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star2RAHours, model.getRACoord(),
  //                                  "calculated ra");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star2Dec, model.getDecCoord(),
  //                                  "calculated dec");
  // // If you want to aim the telescope at b Cet (ra = 0h43m07s, dec = -18.038)
  // This calculated telescope coordinates is very close to the measured
  // telescope coordinates,
  // j = 130.46, , q = 37.67o

  // set encoders to ra/dec values from example

  // altair
  double star3AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(44, 33, 25.8);
  // anticlockwise
  double star3AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3);
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
  //     0.01, 21.92675 ,
  //     Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3),
  //     "???");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 338.0732, star3AzmAxis,
  //                                  "star3AzmAxis");
  model.setEncoderValues(star3AltAxis * 100, star3AzmAxis * 100);

  double star3RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(19, 51, 54.96);

  double star3RADegrees = 360 * star3RAHours / 24.0;

  double star3Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(8, 55, 38.5);

  log("Star 3: Altair");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star3AltAxis, star3AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star3RADegrees, star3Dec);
  //   log("Star time: %ld ", timeMillis);

  // 1456000 milliseconds since first sync
  model.calculateCurrentPosition(star1Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, star3RAHours, model.getRACoord(), "ra");
}

void test_telescope_model_mylocation_with_time_deltas() {
  log("======test_telescope_model_mylocation_with_time_deltas=====");

  TelescopeModel model;
  //   model.setLatitude(-34.0455);
  //   model.setLongitude(151.0508333);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;

  TimePoint star1Time = createTimePoint(day, month, year, hour, minute, second);
  //   unsigned long timeMillis =
  //       convertDateTimeToMillis(day, month, year, hour, minute, second)/1000;

  // model.setUTCYear(2023);
  // model.setUTCMonth(9);
  // model.setUTCDay(2);
  // model.setUTCHour(10);
  // model.setUTCMinute(0);
  // model.setUTCSecond(0);

  model.setAltEncoderStepsPerRevolution(36000); // 100 ticks per degree
  model.setAzEncoderStepsPerRevolution(36000);

  // star 1: Vega

  double star1AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(17, 9, 19.5);
  // anticlockwise
  double star1AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(357, 13, 18.8);

  // start pointing exactly at star, if zero position of encoders is north
  // and flat.
  model.setEncoderValues(star1AltAxis * 100, star1AzmAxis * 100);

  // time s::hoursMinutesSecondsToFloatingHours(21, 27, 56);of observation
  //   double star1Time = timeMillis;

  double star1RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(18, 37, 43.68);
  double star1RADegrees = 360 * star1RAHours / 24.0;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 279.432, star1RADegrees, "star1ra");

  double star1Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 48, 9.2);

  log("Star 1: Vega");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star1AltAxis, star1AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star1RADegrees, star1Dec);
  //   log("Star time: %ld ", timeMillis);

  model.syncPositionRaDec(star1RAHours, star1Dec, star1Time);
  //   model.addReferencePoint();

  model.calculateCurrentPosition(star1Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1RAHours, model.getRACoord(),
                                   "calculated ra");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star1Dec, model.getDecCoord(),
                                   "calculated dec");

  // fomalhaut
  // five minutes later
  day = 2, month = 9, year = 2023, hour = 10, minute = 5, second = 0;
  TimePoint star2Time = createTimePoint(day, month, year, hour, minute, second);

  //   timeMillis = convertDateTimeToMillis(day, month, year, hour, minute,
  //   second)/1000;

  double star2AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(38, 37, 4.1);
  // anticlockwise
  double star2AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(102, 46, 3.7);

  model.setEncoderValues(star2AltAxis * 100, star2AzmAxis * 100);

  //   double star2Time = timeMillis; // use same time for now

  double star2RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(22, 58, 56.46);
  // star2RAHours = star2RAHours - (star2Time - star1Time);//adjust for time
  // shift
  double star2RADegress = 360 * star2RAHours / 24.0;

  double star2Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(-29, 29, 47.7);

  log("Star 2: Fomalhaut");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star2AltAxis, star2AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star2RADegress, star2Dec);
  //   log("Star time: %ld ", timeMillis);

  // 546000 milliseconds since first sync
  model.syncPositionRaDec(star2RAHours, star2Dec, star2Time);
  //  timeMillis + 546000); // time passed in millis
  //   model.addReferencePoint();
  model.calculateCurrentPosition(star2Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star2RAHours, model.getRACoord(),
                                   "calculated ra");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, star2Dec, model.getDecCoord(),
                                   "calculated dec");
  // If you want to aim the telescope at b Cet (ra = 0h43m07s, dec = -18.038)
  // This calculated telescope coordinates is very close to the measured
  // telescope coordinates,
  // j = 130.46, , q = 37.67o

  // set encoders to ra/dec values from example

  // altair

  // five minutes later again
  day = 2;
  month = 9, year = 2023, hour = 10, minute = 10, second = 0;
  TimePoint star3Time = createTimePoint(day, month, year, hour, minute, second);

  //   timeMillis = convertDateTimeToMillis(day, month, year, hour, minute,
  //   second)/1000;

  double star3AltAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(45, 16, 37.4);
  // anticlockwise
  double star3AzmAxis =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(18, 34, 31.1);
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
  //     0.01, 21.92675 ,
  //     Ephemeris::degreesMinutesSecondsToFloatingDegrees(21, 55, 36.3),
  //     "???");
  // TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.01, 338.0732, star3AzmAxis,
  //                                  "star3AzmAxis");
  model.setEncoderValues(star3AltAxis * 100, star3AzmAxis * 100);

  double star3RAHours =
      Ephemeris::hoursMinutesSecondsToFloatingHours(19, 51, 54.96);

  double star3RADegrees = 360 * star3RAHours / 24.0;

  double star3Dec =
      Ephemeris::degreesMinutesSecondsToFloatingDegrees(8, 55, 38.5);

  log("Star 3: Altair");
  log("    \t\t\t\t\talt: %lf\taz: %lf", star3AltAxis, star3AzmAxis);
  log("       \t\t\t\t\tra: %lf\tdec: %lf", star3RADegrees, star3Dec);

  // 1456000 milliseconds since first sync
  model.calculateCurrentPosition(star3Time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, star3Dec, model.getDecCoord(), "dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.5, star3RAHours, model.getRACoord(), "ra");
}

void test_coords() {

  HorizCoord start;
  start = HorizCoord(88, 180);
  HorizCoord changed;
  changed = start.addOffset(3, 0);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 89, changed.altInDegrees,
                                   "alt should loop");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 0, changed.aziInDegrees,
                                   "azi should loop");

  changed = start.addOffset(0, 181);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 88, changed.altInDegrees,
                                   "alt should stay same");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 1, changed.aziInDegrees,
                                   "azi should loop");

  start = HorizCoord(95, 180);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 95, start.altInDegrees,
                                   "alt should  notloop");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 180, start.aziInDegrees,
                                   "azi should loop");
}
void test_model_one_star_align() {

  TelescopeModel model;
  //   model.setLatitude(-34.0455);
  //   model.setLongitude(151.0508333);
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);

  Ephemeris::setLocationOnEarth(-34.0, 2.0, 57.5,  // Lat: 48°50'11"
                                151.0, 2.0, 57.8); // Lon: -2°20'14"

  Ephemeris::flipLongitude(false);
  // Choose a date and time (UTC)
  unsigned int day = 16, month = 9, year = 2023, hour = 6, minute = 39,
               second = 0;

  TimePoint time = createTimePoint(day, month, year, hour, minute, second);

  HorizCoord h1 = HorizCoord(0, 0);
  EqCoord e1 = EqCoord();
  e1.setDecInDegrees(38.8);
  e1.setRAInHours(18.6288);
  SynchPoint point = SynchPoint(e1, h1, time, e1, 0, 0);
  // vega
  model.performOneStarAlignment(point);
}
void test_one_star_align_principle() {
  CoordConv alignment;
  Ephemeris::setLocationOnEarth(-34.0, 2.0, 44.0, // Lat: 48°50'11"
                                151.0, 3.0, 3.0); // Lon: -2°20'14"

  Ephemeris::flipLongitude(false);

  HorizCoord startHoriz = HorizCoord(70.322090, 249.138016);

  unsigned int day = 13, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;

  TimePoint time = createTimePoint(day, month, year, hour, minute, second);
  unsigned long time2 =
      convertDateTimeToMillis(day, month, year, hour, minute, second) / 1000;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
      0.15, time2, convertTimePointToEpochSeconds(time), "time check");

  EqCoord startEq = EqCoord(startHoriz, time);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, -38.797001, startEq.getDecInDegrees(),
                                   "start dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 17.95871, startEq.getRAInHours(),
                                   "start ra");

  alignment.addReferenceCoord(startHoriz, startEq);

  HorizCoord endHoriz = startHoriz.addOffset(90, 0);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 19.67791, endHoriz.altInDegrees,
                                   "modified alt");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 69.138016, endHoriz.aziInDegrees,
                                   "modified alt");

  EqCoord endEq = EqCoord(endHoriz, time);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 5.124745, endEq.getDecInDegrees(),
                                   "end dec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 23.68311, endEq.getRAInHours(),
                                   "end ra");

  alignment.addReferenceCoord(endHoriz, endEq);
  alignment.calculateThirdReference();

  EqCoord outEq = alignment.toReferenceCoord(startHoriz);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, startEq.getDecInDegrees(),
                                   outEq.getDecInDegrees(), "outdec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, startEq.getRAInDegrees(),
                                   outEq.getRAInDegrees(), "outra");
  HorizCoord thirdHoriz = startHoriz.addOffset(-10, 0);

  //   EqCoord down10=EqCoord();
  //   down10.setRAInHours(19,6,23.62);
  //   down10.setDecInDegrees(-39, 14, 8.3);

  EqCoord down10 = EqCoord(thirdHoriz, time);
  //   double outRaDegrees;
  //   double outDecDegrees;
  //   alignment.toInstrumentDeg(outra, outdec, star3AzmAxis, star3AltAxis);

  outEq = alignment.toReferenceCoord(thirdHoriz);

  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, down10.getDecInDegrees(),
                                   outEq.getDecInDegrees(), "outdec");
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, down10.getRAInDegrees(),
                                   outEq.getRAInDegrees(), "outra");
}

void test_time_difference(void) {
  TimePoint point1 = getNow();
  //   std::this_thread::sleep_for(std::chrono::seconds(1)); // Delay for one
  //   second

  TimePoint point2 = addMillisToTime(point1, 1500);
  double diff = differenceInSeconds(point1, point2);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 1.5, diff, "time");
  diff = differenceInSeconds(point2, point1);
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, -1.5, diff, "time");
}

void setup() {

  // test_telescope_model();
  UNITY_BEGIN(); // IMPORTANT LINE!

  // RUN_TEST(test_telescope_model_starting_offset);
  //  RUN_TEST(test_az_encoder_wraparound);
  // RUN_TEST(test_alt_encoder_negative_move);
  // RUN_TEST(test_odd_times);

  // RUN_TEST(test_telescope_model_takeshi);

  //   RUN_TEST(test_two_star_alignment_takeshi_example);
  //   RUN_TEST(test_two_star_alignment_mylocation);

  //   RUN_TEST(test_telescope_model_mylocation_with_offset);
  //   RUN_TEST(test_horizontal_to_eq_eq_constructor);
  //====
//   RUN_TEST(test_az_encoder_calibration);
//   RUN_TEST(test_alt_encoder_calibration);

//   RUN_TEST(test_eq_to_horizontal);
//   //   RUN_TEST(test_horizontal_to_eq);
//   RUN_TEST(test_eq_to_horizontal_vega);

//   RUN_TEST(test_two_star_alignment_mylocation_wrappers);
//   RUN_TEST(test_two_star_alignment_mylocation_wrappers_offset);

  RUN_TEST(test_telescope_model_mylocation);
//   RUN_TEST(test_telescope_model_mylocation_with_tilt);
//   RUN_TEST(test_one_star_align_principle);
//   RUN_TEST(test_coords);

//   RUN_TEST(test_model_one_star_align);
//   RUN_TEST(test_eq_coord_distance);

//   RUN_TEST(test_time_difference);
  //====
  //   RUN_TEST(test_continuity);

//   RUN_TEST(test_telescope_model_mylocation_with_time_deltas);

  //   RUN_TEST(testTimestepConversion);

  UNITY_END(); // IMPORTANT LINE!
}

void loop() {
  // Do nothing here.
}

int main() {

  setup();
  return 0;
}
