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
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(14.28644, model.getRACoord(), "ra");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(18.97959, model.getDecCoord(), "dec");
  model.setEncoderValues(0,20);

  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(33.39644, model.getDecCoord(), "dec");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(15.30819, model.getRACoord(), "ra");

  model.setEncoderValues(45, 20);
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(-3.011309, model.getDecCoord(), "dec");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(17.176, model.getRACoord(), "ra");
}
void setup() {
  UNITY_BEGIN(); // IMPORTANT LINE!
  RUN_TEST(test_eq_to_horizontal);
  RUN_TEST(test_horizontal_to_eq);
  RUN_TEST(test_telescope_model);
  UNITY_END(); // IMPORTANT LINE!
}

void loop() {
  // Do nothing here.
}

int main() {
  setup();
  return 0;
}
