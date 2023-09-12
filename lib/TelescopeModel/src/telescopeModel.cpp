#include "telescopeModel.h"
#include "Logging.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unity.h>

TelescopeModel::TelescopeModel() {
  // Default alignment for alt az in south
  // alignment.addReference(0, 0, M_PI, 0);//radians
  alignment.addReferenceDeg(0, 0, 180, 0); // degrees

  // alignment.addReference(0, M_PI_2, M_PI, M_PI_2);//radians
  alignment.addReferenceDeg(0, 90, 180, 90); // degreess
  alignment.calculateThirdReference();
  // alignment.reset();

  latitude = 0;
  longitude = 0;

  altEnc = 0;
  azEnc = 0;
  azEncoderStepsPerRevolution = 0;
  altEncoderStepsPerRevolution = 0;

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

  firstSyncTime = 0;
  secondSyncTime = 0;
  alignmentModelSyncTime = 0;

  altOffsetToAddToEncoderResult = 0;
  azOffsetToAddToEncoderResult = 0;
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

// // given known alt and az values, and known encoder values, return offsets
// // such that calculateAltAzFromEncoders() for these encoder values
// // would always return these alt az values.
// // TODO delete if not used
// void TelescopeModel::calculateEncoderOffsetFromAltAz(float alt, float az,
//                                                      long altEncVal,
//                                                      long azEncVal,
//                                                      long &altEncOffset,
//                                                      long &azEncOffset) {

//   float calculatedAlt =
//       360.0 * ((float)(altEncVal)) / (float)altEncoderStepsPerRevolution;
//   float calculatedAz =
//       360.0 * ((float)(azEncVal)) / (float)azEncoderStepsPerRevolution;
//   altEncOffset = calculatedAlt - alt;
//   azEncOffset = calculatedAz - az;
// }

void TelescopeModel::calculateAltAzFromEncoders(float &alt, float &az,
                                                long altEncVal, long azEncVal) {
  alt = 360.0 * ((float)(altEncVal)) / (float)altEncoderStepsPerRevolution;
  az = 360.0 * ((float)(azEncVal)) / (float)azEncoderStepsPerRevolution;
}
bool TelescopeModel::isNorthernHemisphere() { return latitude >= 0; }
/**
 * @brief Calculates the current position of the telescope based on encoder
 * values.
 *
 * This method performs the following steps:
 * 1. Converts the base equatorial (RA/Dec) coordinates to horizontal
 * (Alt/Az) coordinates.
 * 2. Adjusts the Alt/Az coordinates based on the encoder values.
 * 3. Checks for the Altitude value going "over the top" (beyond 90
 * degrees). If so, adjusts the Altitude and Azimuth values accordingly.
 * 4. Converts the adjusted Alt/Az coordinates back to equatorial
 * coordinates to get the new RA/Dec values.
 *
 * @note The method uses the Ephemeris library for coordinate
 * transformations.
 *
 * @return void
 */
void TelescopeModel::calculateCurrentPosition(unsigned long timeMillis) {

  log("Calculating current position for time %ld", timeMillis);
  // When client syncs the position, we store the ra/dec of that
  // position as well as the encoder values.
  // So we start by converting that base position into known
  // alt/az coords
  double ra;
  double dec;
  float altEncoderDegrees;
  float azEncoderDegrees;
  // convert encoder values to degrees
  calculateAltAzFromEncoders(altEncoderDegrees, azEncoderDegrees, altEnc,
                             azEnc);

  log("Alt az from encoders: \t\talt: %lf\taz:%lf", altEncoderDegrees,
      azEncoderDegrees);

  adjustAltAzBasedOnOffsets(altEncoderDegrees, azEncoderDegrees);
  log("Offset alt az from encoders: \t\talt: %lf\taz:%lf", altEncoderDegrees,
      azEncoderDegrees);
  double azAngle;
  if (latitude > 90) {
    if (isNorthernHemisphere()) {
      azAngle = 360.0 - azEncoderDegrees;
    } else {
      azAngle = azEncoderDegrees;
    }
  }
  double raDeltaDegrees = 0;
  // if (alignment.getRefs() > 0) {
  unsigned long timedelta = timeMillis - alignmentModelSyncTime;
  raDeltaDegrees = millisecondsToRADeltaInDegrees(timedelta);
  log("RA Adjustment due to time\t\traDelta: %lf", raDeltaDegrees);

  alignment.toReferenceDeg(ra, dec, azAngle, altEncoderDegrees);
  ra = fmod(fmod(ra, 360) + 360, 360);

  currentRA = ra + raDeltaDegrees;
  currentDec = dec;
  log("Final position\t\ra: %lf\tdec: %lf", currentRA, currentDec);
}
float TelescopeModel::getDecCoord() { return currentDec; }
float TelescopeModel::getRACoord() { return currentRA; }

double TelescopeModel::millisecondsToRADeltaInDegrees(
    unsigned long millisecondsDelta) {
  double RA_delta_degrees =
      (millisecondsDelta / (24.0 * 3600.0 * 1000.0)) * 360.0;
  return RA_delta_degrees;
}

void TelescopeModel::addReferencePoint() {

  double raDeltaDegrees = 0;
  if (alignment.getRefs() > 0) {
    unsigned long timedelta = secondSyncTime - firstSyncTime;

    // std::cout << "lastSyncedTime :" << secondSyncTime << "\n\r";
    // std::cout << "firstSyncTime :" << firstSyncTime << "\n\r";
    // std::cout << "timedelta :" << timedelta << "\n\r";
    // std::cout << "raDeltaDegrees :" << raDeltaDegrees << "\n\r";

    raDeltaDegrees = millisecondsToRADeltaInDegrees(timedelta);
  }

  if (isNorthernHemisphere()) {
    alignment.addReferenceDeg(lastSyncedRa - raDeltaDegrees, lastSyncedDec,
                              360 - lastSyncedAz, lastSyncedAlt);
    std::cout << "North\n\r";
  } else {
    alignment.addReferenceDeg(lastSyncedRa - raDeltaDegrees, lastSyncedDec,
                              lastSyncedAz, lastSyncedAlt);
  }
  if (alignment.getRefs() == 2) {
    alignment.calculateThirdReference();
    alignmentModelSyncTime =
        firstSyncTime; // store this so future partial syncs don't clobber
  }
  int i = alignment.refs;
  std::cout << "Refs after add:" << i << "\n\r\n\r";
}
/**
 * @brief calibrates a position in the sky with current encoder values
 *
 * Two main functions:
 * 1) Store the passed ra,dec, enoder, and time values for use in reference
 * point sync
 * 2) Calculate local alt/az values, and store them for use as
 * offsets
 *
 *
 */
void TelescopeModel::syncPositionRaDec(float ra, float dec,
                                       unsigned long time) {

  // get local alt/az of target.
  // Work out alt/az offset required, such that when added to
  // values calculated from encoder, we end up with this target
  // alt/az
  Ephemeris::Ephemeris::setLocationOnEarth(latitude, longitude);
  Ephemeris::
      // East is negative and West is positive
      Ephemeris::flipLongitude(false);

  EquatorialCoordinates eq;
  eq.ra = ra / 15; // Ephemeris uses hours not degrees , 1 hour = 15 degrees
  eq.dec = dec;

  log("Calculating local alt az bazed on \tra(h): %lf \tdec: %lf", eq.ra,
      eq.dec);

  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(eq, time);
  // TODO souther hemi only?
  if (!isNorthernHemisphere())
    altAzCoord.alt = -altAzCoord.alt;

  log("Resulting local altaz\t\t\talt: %lf \t\taz:%lf", altAzCoord.alt,
      altAzCoord.azi);

  float calculatedAlt, calculatedAz;
  calculateAltAzFromEncoders(calculatedAlt, calculatedAz, altEnc, azEnc);

  log("Calculated alt/az from encoders\t\talt: %lf \taz:%lf", calculatedAlt,
      calculatedAz);

  storeAltAzOffset(altAzCoord.alt, altAzCoord.azi, calculatedAlt, calculatedAz);

  // these are used for adding reference points to the model
  lastSyncedRa = ra;
  lastSyncedDec = dec;
  lastSyncedAlt = altAzCoord.alt;
  lastSyncedAz = altAzCoord.azi;

  // these are used for calibrating encoders. Some duplication here.
  //  Shuffle older alignment values into position 2
  altEncoderAlignValue2 = altEncoderAlignValue1;
  azEncoderAlignValue2 = azEncoderAlignValue1;
  azAlignValue2 = azAlignValue1;
  altAlignValue2 = altAlignValue1;

  // Update new alignment values
  azAlignValue1 = altAzCoord.azi;
  altAlignValue1 = altAzCoord.alt;
  altEncoderAlignValue1 = altEnc;
  azEncoderAlignValue1 = azEnc;

  // lastSyncedAltEncoder = altEnc + altOffsetToAddToEncoderResult;

  // if no refs set, then mark this as t=0
  if (alignment.getRefs() == 0) {
    firstSyncTime = time;
  } else if (alignment.getRefs() == 1) {
    secondSyncTime = time;
  }
  // // calculate alt/az using current (saved) model
  // double azModeledCounterclockwise;
  // double altModeled;

  // // find time from now to saved sync time.
  // double timeOffset =
  //     millisecondsToRADeltaInDegrees(time) - alignmentModelSyncTime;

  // alignment.toInstrumentDeg(azModeledCounterclockwise, altModeled,
  //                           ra + timeOffset, dec);
  // azModeledCounterclockwise =
  //     fmod(fmod(azModeledCounterclockwise, 360) + 360, 360);

  // float azFromEncoderClockwise;
  // float altFromEncoder;
  // calculateAltAzFromEncoders(altFromEncoder, azFromEncoderClockwise,
  // altEnc,
  //                            azEnc);

  // altErrorDelta = altFromEncoder - altModeled;
  // azErrorDelta = azFromEncoderClockwise - (360 -
  // azModeledCounterclockwise);

  // todo do offsets here later

  // currentRA = ra;
  // currentDec = dec;
  // currentAlt = alt;
  // currentAz = 360 - azcounterclockwise;

  // // the following used for offset calculations later, ie mapping encoder
  // to
  // // alt/az
  // altBaseValue = alt;
  // azBaseValue = 360 - azcounterclockwise;
  // altEncBaseValue = altEnc;
  // azEncBaseValue = azEnc;

  // // the following used for encoder calibration and reference point
  // setting raBasePos = ra; decBasePos = dec; baseTime = time; baseRaOffset
  // = raOffset;
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
 * coordinates. It then handles any wraparound in azimuth. The ratio of
 * encoder move to coordinate move yields the number of encoder steps per
 * degree of azimuth, which is then scaled to a full revolution.
 *
 * @return Number of encoder steps required for a full 360° azimuth
 * revolution.
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
    return 0; // avoid division by zero here. May propagate the issue though.

  // Compute steps per degree by dividing total encoder movement by total
  // coordinate movement.
  float stepsPerDegree = (float)encoderMove / coordMove;

  // Return steps per full revolution (360°) by scaling up stepsPerDegree.
  return stepsPerDegree * 360.0;
}

void TelescopeModel::storeAltAzOffset(float actualAlt, float actualAz,
                                      float encoderBasedAlt,
                                      float encoderBasedAz) {
  altOffsetToAddToEncoderResult = actualAlt - encoderBasedAlt;
  azOffsetToAddToEncoderResult = actualAz - encoderBasedAz;

  log("Calcluated sync alt/az offset\t\talt:%lf\taz:%lf",
      altOffsetToAddToEncoderResult, azOffsetToAddToEncoderResult);
}

void TelescopeModel::adjustAltAzBasedOnOffsets(float &alt, float &az) {
  alt += altOffsetToAddToEncoderResult;
  az += azOffsetToAddToEncoderResult;
}
/**
 * @brief Calculates the number of encoder steps per hypothetical full 360°
 * altitude revolution.
 *
 * This method computes the movement in the altitude for both encoder and
 * coordinates. It then handles any wraparound in altitude considering that
 * the telescope can move from slightly below the horizon to slightly past
 * zenith. The ratio of encoder move to coordinate move yields the number of
 * encoder steps per degree of altitude, which is then scaled hypothetically
 * to a full revolution.
 *
 * @return Number of encoder steps required for a hypothetical full 360°
 * altitude revolution.
 */
long TelescopeModel::calculateAltEncoderStepsPerRevolution() {

  // Calculate difference in altitude encoder alignment values
  long encoderMove = altEncoderAlignValue2 - altEncoderAlignValue1;

  // Calculate difference in altitude alignment values
  float coordMove = altAlignValue2 - altAlignValue1;

  // Handle potential wraparound for altitude (which is defined between -45°
  // to 45°) If your scope can move from slightly below the horizon to
  // slightly past zenith
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
