#include "telescopeModel.h"
#include <cmath>

TelescopeModel::TelescopeModel() {

  year = 0;
  month = 0;
  day = 0;
  hour = 0;
  min = 0;
  sec = 0;

  latitude = 0;
  longitude = 0;

  altEnc = 0;
  azEnc = 0;
  azEncoderStepsPerRevolution = 0;
  altEncoderStepsPerRevolution = 0;

  deltaSeconds = 0;

  // known eq position at sync
  raBasePos = 0;
  decBasePos = 0;
  // known encoder values at same
  altEncBaseValue = 0;
  azEncBaseValue = 0;
  currentRA = 0;
  currentDec = 0;
  currentAlt = 0;
  currentAz = 0;
}
void TelescopeModel::setEncoderValues(long encAlt, long encAz) {
  altEnc = encAlt;
  azEnc = encAz;
}

void TelescopeModel::setAzEncoderStepsPerRevolution(long azResolution) {
  azEncoderStepsPerRevolution = azResolution;
}
void TelescopeModel::setAltEncoderStepsPerRevolution(long altResolution) {
  altEncoderStepsPerRevolution = altResolution;
}

void TelescopeModel::setLatitude(float lat) { latitude = lat; }
void TelescopeModel::setLongitude(float lng) { longitude = lng; }

float TelescopeModel::getLatitude() { return latitude; }
float TelescopeModel::getLongitude() { return longitude; }

// TODO implement
float TelescopeModel::getAltCoord() { return currentAlt; }
float TelescopeModel::getAzCoord() { return currentAz; }

void TelescopeModel::setDeltaSeconds(double delta) { deltaSeconds = delta; }

/**
 * @brief Calculates the current position of the telescope based on encoder
 * values.
 *
 * This method performs the following steps:
 * 1. Converts the base equatorial (RA/Dec) coordinates to horizontal (Alt/Az)
 * coordinates.
 * 2. Adjusts the Alt/Az coordinates based on the encoder values.
 * 3. Checks for the Altitude value going "over the top" (beyond 90 degrees). If
 * so, adjusts the Altitude and Azimuth values accordingly.
 * 4. Converts the adjusted Alt/Az coordinates back to equatorial coordinates to
 * get the new RA/Dec values.
 *
 * @note The method uses the Ephemeris library for coordinate transformations.
 *
 * @return void
 */
void TelescopeModel::calculateCurrentPosition() {
  // When client syncs the position, we store the ra/dec of that
  // position as well as the encoder values.
  // So we start by converting that base position into known
  // alt/az coords

  Ephemeris::setLocationOnEarth(latitude, longitude);
  EquatorialCoordinates baseEqCoord;

  baseEqCoord.ra = raBasePos;
  baseEqCoord.dec = decBasePos;

  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          baseEqCoord, day, month, year, hour, min, sec);
  // add encoder values since then: we stored the encoder values
  // at position set
  float a = 360.0 * ((float)(altEnc - altEncBaseValue)) /
            (float)altEncoderStepsPerRevolution;
  altAzCoord.alt += a;

  float z = 360.0 * ((float)(azEnc - azEncBaseValue)) /
            (float)azEncoderStepsPerRevolution;
  altAzCoord.azi += z;

  currentAlt = altAzCoord.alt;
  currentAz = altAzCoord.azi;

  // handle situation where scope goes "over the top"
  if (currentAlt > 90) {
    currentAlt = 180 - currentAlt;
    currentAz += 180;
    if (currentAz > 360) {
      currentAz -= 360;
    }
  }
  // convert back to eq to get new ra/dec
  EquatorialCoordinates adjustedEqCoord =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
          altAzCoord, day, month, year, hour, min, sec);
  if (adjustedEqCoord.ra >= (24.0-1e-5)) {
    currentRA = adjustedEqCoord.ra - 24.0;
    if (currentRA < 0) {
      currentRA = 0.0;
    }
  } else {
    currentRA = adjustedEqCoord.ra;
  }

  // currentRA = fmod(adjustedEqCoord.ra, 24.0);
  // if (fabs(currentRA - 24.0) < 1e-6) {
  //   currentRA = 0.0;
  // }

  currentDec = adjustedEqCoord.dec;
}
float TelescopeModel::getDecCoord() { return currentDec; }
float TelescopeModel::getRACoord() { return currentRA; }

void TelescopeModel::setPositionRaDec(float ra, float dec) {
  raBasePos = ra;
  decBasePos = dec;
  altEncBaseValue = altEnc;
  azEncBaseValue = azEnc;
}

void TelescopeModel::setUTCYear(int y) { year = y; }
void TelescopeModel::setUTCMonth(int m) { month = m; }
void TelescopeModel::setUTCDay(int d) { day = d; }
void TelescopeModel::setUTCHour(int h) { hour = h; }
void TelescopeModel::setUTCMinute(int m) { min = m; }
void TelescopeModel::setUTCSecond(int s) { sec = s; }

/**
 * @brief Saves the encoder calibration point.
 *
 * It first calculates horizontal coordinates (alt/az) from the current
 * equatorial coordinates (ra/dec), on a specific date and time. Then, it stores
 * the old alignment values, and updates the new alignment values based on the
 * calculated alt/az values and the current encoder readings.
 */
void TelescopeModel::saveEncoderCalibrationPoint() {
  // Initialize equatorial coordinates with current base positions.
  EquatorialCoordinates baseEqCoord;
  baseEqCoord.ra = raBasePos;
  baseEqCoord.dec = decBasePos;

  // Convert current equatorial coordinates to horizontal coordinates for a
  // certain date and time.
  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          baseEqCoord, day, month, year, hour, min, sec);

  // Store older alignment values.
  altEncoderAlignValue2 = altEncoderAlignValue1;
  azEncoderAlignValue2 = azEncoderAlignValue1;
  azAlignValue2 = azAlignValue1;
  altAlignValue2 = altAlignValue1;

  // Update new alignment values using the converted horizontal coordinates and
  // current encoder readings.
  azAlignValue1 = altAzCoord.azi;
  altAlignValue1 = altAzCoord.alt;
  altEncoderAlignValue1 = altEnc;
  azEncoderAlignValue1 = azEnc;
}

long TelescopeModel::getAzEncoderStepsPerRevolution() {
  return azEncoderStepsPerRevolution;
}
long TelescopeModel::getAltEncoderStepsPerRevolution() {
  return altEncoderStepsPerRevolution;
}
long TelescopeModel::getAltEncoderAlignValue1() const {
  return altEncoderAlignValue1;
}

long TelescopeModel::getAltEncoderAlignValue2() const {
  return altEncoderAlignValue2;
}

long TelescopeModel::getAzEncoderAlignValue1() const {
  return azEncoderAlignValue1;
}

long TelescopeModel::getAzEncoderAlignValue2() const {
  return azEncoderAlignValue2;
}

float TelescopeModel::getAltAlignValue1() const { return altAlignValue1; }

float TelescopeModel::getAltAlignValue2() const { return altAlignValue2; }

float TelescopeModel::getAzAlignValue1() const { return azAlignValue1; }

float TelescopeModel::getAzAlignValue2() const { return azAlignValue2; }

/**
 * @brief Calculates the number of encoder steps per full 360° azimuth
 * revolution.
 *
 * It first computes the movement in the azimuth for both encoder and
 * coordinates. It then handles any wraparound in azimuth. The ratio of encoder
 * move to coordinate move yields the number of encoder steps per degree of
 * azimuth, which is then scaled to a full revolution.
 *
 * @return Number of encoder steps required for a full 360° azimuth revolution.
 */
long TelescopeModel::calculateAzEncoderStepsPerRevolution() {

  // Calculate difference in azimuth encoder alignment values.
  long encoderMove = azEncoderAlignValue2 - azEncoderAlignValue1;

  // Calculate difference in azimuth alignment values.
  float coordMove = azAlignValue2 - azAlignValue1;

  // Handle wraparound for azimuth (which is defined from -180° to 180°).
  if (coordMove < -180) {
    coordMove += 360;
  } else if (coordMove > 180) {
    coordMove -= 360;
  }

  if (abs(coordMove) < 0.000001)
    return 0; //avoid division by zero here. May propagate the issue though.

  // Compute steps per degree by dividing total encoder movement by total
  // coordinate movement.
  float stepsPerDegree = (float)encoderMove / coordMove;

  // Return steps per full revolution (360°) by scaling up stepsPerDegree.
  return stepsPerDegree * 360.0;
}

/**
 * @brief Calculates the number of encoder steps per hypothetical full 360°
 * altitude revolution.
 *
 * This method computes the movement in the altitude for both encoder and
 * coordinates. It then handles any wraparound in altitude considering that the
 * telescope can move from slightly below the horizon to slightly past zenith.
 * The ratio of encoder move to coordinate move yields the number of encoder
 * steps per degree of altitude, which is then scaled hypothetically to a full
 * revolution.
 *
 * @return Number of encoder steps required for a hypothetical full 360°
 * altitude revolution.
 */
long TelescopeModel::calculateAltEncoderStepsPerRevolution() {

  // Calculate difference in altitude encoder alignment values
  long encoderMove = altEncoderAlignValue2 - altEncoderAlignValue1;

  // Calculate difference in altitude alignment values
  float coordMove = altAlignValue2 - altAlignValue1;

  // Handle potential wraparound for altitude (which is defined between -45° to
  // 45°) If your scope can move from slightly below the horizon to slightly
  // past zenith
  if (coordMove < -45) {
    coordMove += 90;
  } else if (coordMove > 45) {
    coordMove -= 90;
  }
  if (abs(coordMove) < 0.000001)
    return 0; // avoid division by zero here. May propagate the issue though.

  // Compute steps per degree by dividing total encoder movement by total
  // coordinate movement.
  float stepsPerDegree = (float)encoderMove / coordMove;

  // Return steps per hypothetical full revolution (360°) by scaling up
  // stepsPerDegree.
  return stepsPerDegree * 360.0;
}
