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
  defaultAlignment = true;

  latitude = -34.049120;
  longitude = 151.042100;

  altEnc = 0;
  azEnc = 0;
  azEncoderStepsPerRevolution = 0;
  altEncoderStepsPerRevolution = 0;

  // known eq position at sync
  // raBasePos = 0;
  // decBasePos = 0;
  // known encoder values at same
  // altEncBaseValue = 0;
  // azEncBaseValue = 0;

  // currentAlt = 0;
  // currentAz = 0;

  firstSyncTime = 0;
  secondSyncTime = 0;
  alignmentModelSyncTime = 0;

  errorToAddToEncoderResultAlt = 0;
  errorToAddToEncoderResultAzi = 0;
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

HorizCoord TelescopeModel::calculateAltAzFromEncoders(long altEncVal,
                                                      long azEncVal) {

  float alt =
      360.0 * ((float)(altEncVal)) / (float)altEncoderStepsPerRevolution;
  float az = 360.0 * ((float)(azEncVal)) / (float)azEncoderStepsPerRevolution;
  return HorizCoord(alt, az);
}
bool TelescopeModel::isNorthernHemisphere() { return latitude > 0; }
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
  log("");
  log("=====calculateCurrentPosition====");
  // log("Calculating current position for time %ld", timeMillis);
  // When client syncs the position, we store the ra/dec of that
  // position as well as the encoder values.
  // So we start by converting that base position into known
  // alt/az coords
  double raInDegrees;
  double decInDegrees;
  float altEncoderDegrees;
  float azEncoderDegrees;
  // convert encoder values to degrees
  HorizCoord encoderAltAz = calculateAltAzFromEncoders(altEnc, azEnc);
  log("Raw Alt az from encoders: \t\talt: %lf\taz:%lf", encoderAltAz.alt,
      encoderAltAz.azi);

  HorizCoord adjustedAltAz = encoderAltAz.addOffset(
      errorToAddToEncoderResultAlt, errorToAddToEncoderResultAzi);

  log("Offset alt az from encoders: \talt: %lf\taz:%lf", adjustedAltAz.alt,
      adjustedAltAz.azi);
  TakiHorizCoord takiCoord =
      TakiHorizCoord(adjustedAltAz, isNorthernHemisphere());

  log("Taki coord for reference calc: alt: %lf\taz: %lf", takiCoord.altAngle,
      takiCoord.aziAngle);
  // alignment.toReferenceDeg(raInDegrees, decInDegrees, takiCoord.aziAngle, takiCoord.altAngle);
  alignment.toInstrumentDeg(raInDegrees, decInDegrees, takiCoord.aziAngle,
                           takiCoord.altAngle);

  raInDegrees = fmod(fmod(raInDegrees, 360) + 360, 360);

  double raDeltaDegrees = 0;
  
  log("Time passed: %ld \t (since alignment model creation time: %ld)", timeMillis, alignmentModelSyncTime);

  unsigned long timedelta = 0;
  if (alignmentModelSyncTime != 0) {
    timedelta = timeMillis - alignmentModelSyncTime;
  }

  raDeltaDegrees = millisecondsToRADeltaInDegrees(timedelta);
  log("Time delta millis: %ld degrees: %lf", timedelta, raDeltaDegrees);

  currentEqPosition.setRAInDegrees(raInDegrees + raDeltaDegrees);
  currentEqPosition.setDecInDegrees(decInDegrees);
      // log("RA: %lf", currentRA);
      // currentDec = dec;
  log("Final position\t\t\tra(h): %lf\tdec: %lf", currentEqPosition.getRAInHours(),
      currentEqPosition.getDecInDegrees());
  log("=====calculateCurrentPosition====");
  log("");
}
float TelescopeModel::getDecCoord() { return currentEqPosition.getDecInDegrees(); }
float TelescopeModel::getRACoord() { return currentEqPosition.getRAInHours(); }

double TelescopeModel::millisecondsToRADeltaInDegrees(
    unsigned long millisecondsDelta) {
  double RA_delta_degrees =
      (millisecondsDelta / (24.0 * 3600.0 * 1000.0)) * 360.0;
  return RA_delta_degrees;
}

/**
 * This is a special case where the scope has just been initialised,
 * and no real aligment model is present. If so, after first sync,
 * an alignment point is added for the current ra/dec and calculated alt/az.
 * Then a second alignment point is added. This second point is calculated
 * by taking the calculated alt az, manipulating it so we get a position
 * on the other side of the sky, calulating ra/dec for that position, then
 * setting a second alignment point for that position.
 *
 * The net effect should be that after the first sync, the user gets rough
 * alignment. Then they can choose to do more accurate manual alignment points.
 */
void TelescopeModel::performOneStarAlignment(HorizCoord horiz1, EqCoord eq1,
                                             unsigned long time) {

  TakiHorizCoord taki1 = TakiHorizCoord(horiz1, isNorthernHemisphere());

  alignment.addReferenceDeg(taki1.aziAngle, taki1.altAngle,eq1.getRAInDegrees(),
                            eq1.getDecInDegrees() );

  HorizCoord horiz2 = horiz1.addOffset(90, 0);

  EqCoord eq2 = EqCoord(horiz2, time);

  TakiHorizCoord taki2 = TakiHorizCoord(horiz2, isNorthernHemisphere());

  alignment.addReferenceDeg(taki2.aziAngle, taki2.altAngle,eq2.getRAInDegrees(),
                            eq2.getDecInDegrees() );
  alignment.calculateThirdReference();
  log("===Generated 1 star reference point===");
  log("Point 1: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz1.alt,
      horiz1.azi, eq1.getRAInHours(), eq1.getDecInDegrees());
  log("Point 2: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz2.alt, horiz2.azi,
      eq2.getRAInHours(), eq2.getDecInDegrees());
  alignmentModelSyncTime=time;
}

void TelescopeModel::addReferencePoint() {
  log("");
  log("=====addReferencePoint====");
  double raDeltaDegrees = 0;
  if (alignment.getRefs() > 0) {
    unsigned long timedelta = secondSyncTime - firstSyncTime;
    raDeltaDegrees = millisecondsToRADeltaInDegrees(timedelta);
  }
  TakiHorizCoord taki = TakiHorizCoord(lastSyncedHoriz, isNorthernHemisphere());

  alignment.addReferenceDeg(taki.aziAngle,
                            taki.altAngle,lastSyncedEq.getRAInDegrees() -
                                raDeltaDegrees,
                            lastSyncedEq.getDecInDegrees() );

  if (alignment.getRefs() == 2) {
    log("Got two refs, calculating model...");
    alignment.calculateThirdReference();
    alignmentModelSyncTime =
        firstSyncTime; // store this so future partial syncs don't clobber
  }

  log("=====addReferencePoint====");
  log("");
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
void TelescopeModel::syncPositionRaDec(float raInHours, float decInDegrees,
                                       unsigned long time) {
  log("");
  log("=====syncPositionRaDec====");

  // get local alt/az of target.
  // Work out alt/az offset required, such that when added to
  // values calculated from encoder, we end up with this target
  // alt/az
  log("lat: %lf, long: %lf", latitude, longitude);
  Ephemeris::setLocationOnEarth(latitude, longitude);
  Ephemeris::flipLongitude(false); // East is negative and West is positive
  log("lat long set");

  lastSyncedEq.setRAInHours(raInHours);
  lastSyncedEq.setDecInDegrees(decInDegrees);

  log("Calculating expected alt az bazed on \tra(h): %lf \tdec: %lf",
      lastSyncedEq.getRAInHours(), lastSyncedEq.getDecInDegrees());
      

  lastSyncedHoriz = HorizCoord(lastSyncedEq, time);

  log("Expected local altaz\t\t\talt: %lf \t\taz:%lf", lastSyncedHoriz.alt,
      lastSyncedHoriz.azi);
  log("Actual encoder values\t\t\talt: %ld \t\taz:%ld", altEnc, azEnc);

  HorizCoord calculatedAltAzFromEncoders =
      calculateAltAzFromEncoders(altEnc, azEnc);

  log("Calculated alt/az from encoders\t\talt: %lf\t\taz:%lf",
      calculatedAltAzFromEncoders.alt, calculatedAltAzFromEncoders.azi);

  errorToAddToEncoderResultAlt =
      lastSyncedHoriz.alt - calculatedAltAzFromEncoders.alt;
  errorToAddToEncoderResultAzi =
      lastSyncedHoriz.azi - calculatedAltAzFromEncoders.azi;

  log("Stored alt/az delta\t\t\talt: %lf\t\taz:%lf",
      errorToAddToEncoderResultAlt, errorToAddToEncoderResultAzi);

  // these are used for adding reference points to the model

  // these are used for calibrating encoders. Some duplication here.
  //  Shuffle older alignment values into position 2
  altEncoderAlignValue2 = altEncoderAlignValue1;
  azEncoderAlignValue2 = azEncoderAlignValue1;
  azAlignValue2 = azAlignValue1;
  altAlignValue2 = altAlignValue1;

  // Update new alignment values
  azAlignValue1 = lastSyncedHoriz.azi;
  altAlignValue1 = lastSyncedHoriz.alt;

  altEncoderAlignValue1 = altEnc;
  azEncoderAlignValue1 = azEnc;

  // if no refs set, then mark this as t=0
  if (alignment.getRefs() == 0) {
    firstSyncTime = time;
  } else if (alignment.getRefs() == 1) {
    secondSyncTime = time;
  }

  // special case: if this is the first alignment, then do a special one star
  // alignment
  if (defaultAlignment) {
    performOneStarAlignment(lastSyncedHoriz, lastSyncedEq, time);
    defaultAlignment = false;
  }
  log("=====syncPositionRaDec====");
  log("");
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
