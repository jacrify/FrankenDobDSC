#include "telescopeModel.h"
#include <Ephemeris.h>
#include <cstdint>
#include <unity.h> // Include the Unity test framework.

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
  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  // Choose a date and time (UTC)
  unsigned int day = 2, month = 9, year = 2023, hour = 10, minute = 0,
               second = 0;
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setAltEncoderStepsPerRevolution(360);
  model.setAzEncoderStepsPerRevolution(360);

  model.setEncoderValues(0, 0);
  // arcturus
  model.setPositionRaDec(14.28644, 18.97959);
  model.calculateCurrentPosition();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(14.28644, model.getRACoord(), "ra");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(18.97959, model.getDecCoord(), "dec");
  model.setEncoderValues(0, 20);
  model.calculateCurrentPosition();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(33.39644, model.getDecCoord(), "dec");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(15.30819, model.getRACoord(), "ra");

  model.setEncoderValues(45, 20);
  model.calculateCurrentPosition();
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-3.011309, model.getDecCoord(), "dec");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(17.176, model.getRACoord(), "ra");
}

void test_odd_times(void) {
  TelescopeModel model;
  model.setLatitude(-34.049120);
  model.setLongitude(151.042100);
  // Choose a date and time (UTC)
  unsigned int day = 3, month = 9, year = 2022, hour = 7, minute = 5,
               second = 33;
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
  model.setPositionRaDec(0, 0);
  model.calculateCurrentPosition();

  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0, model.getRACoord(), "ra");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(0, model.getDecCoord(), "dec");
}

void test_az_encoder_calibration(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(360);
  model.setAzEncoderStepsPerRevolution(360);

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(0, 0);
  model.setPositionRaDec(14.28644, 18.97959);
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
  model.setPositionRaDec(newEq.ra, newEq.dec);
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

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(0, 0);
  model.setPositionRaDec(14.28644, 18.97959);
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
  model.setPositionRaDec(newEq.ra, newEq.dec);

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

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(0, 0);
  model.setPositionRaDec(14.28644, 18.97959);
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
  model.setPositionRaDec(newEq.ra, newEq.dec);
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

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(100, 0);
  model.setPositionRaDec(14.28644, 18.97959);
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
  model.setPositionRaDec(newEq.ra, newEq.dec);

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

void test_large_encoder_values(void) {
  TelescopeModel model;
  model.setAltEncoderStepsPerRevolution(9999);
  model.setAzEncoderStepsPerRevolution(9999);

  model.setLatitude(-34.0493);
  model.setLongitude(151.0494);
  model.setUTCYear(2023);
  model.setUTCMonth(9);
  model.setUTCDay(2);
  model.setUTCHour(10);
  model.setUTCMinute(0);
  model.setUTCSecond(0);

  model.setEncoderValues(0, 0);
  model.setPositionRaDec(14.28644, 18.97959);
  // sets alt az
  model.calculateCurrentPosition();

  model.saveEncoderCalibrationPoint(); // Save this point

  model.setEncoderValues(INT64_MAX - 1, 0);
  model.setPositionRaDec(14.28644, 18.97959);
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint();

  // Simulate a 1 unit move.
  model.setEncoderValues(INT64_MAX, 0);
  model.setPositionRaDec(15.28644, 18.97959);
  model.calculateCurrentPosition();
  model.saveEncoderCalibrationPoint();

  long calculatedSteps = model.calculateAzEncoderStepsPerRevolution();

  float epsilon = 1.0;
  TEST_ASSERT_FLOAT_WITHIN_MESSAGE(
      epsilon, 360, calculatedSteps,
      "Azimuth Encoder Steps with large encoder values");
}

void setup() {
  UNITY_BEGIN(); // IMPORTANT LINE!
  RUN_TEST(test_eq_to_horizontal);
  RUN_TEST(test_horizontal_to_eq);
  RUN_TEST(test_telescope_model);
  RUN_TEST(test_az_encoder_calibration);
  RUN_TEST(test_alt_encoder_calibration);

  RUN_TEST(test_az_encoder_wraparound);
  RUN_TEST(test_alt_encoder_negative_move);

  RUN_TEST(test_large_encoder_values);
  RUN_TEST(test_odd_times);
  UNITY_END(); // IMPORTANT LINE!
}

void loop() {
  // Do nothing here.
}

int main() {
  setup();
  return 0;
}
