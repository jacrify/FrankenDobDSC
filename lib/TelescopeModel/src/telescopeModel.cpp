#include "telescopeModel.h"
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

  raOffset = 0;

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

void TelescopeModel::setRaOffset(double delta) { raOffset = delta; }

// given known alt and az values, and known encoder values, return offsets
// such that calculateAltAzFromEncoders() for these encoder values
// would always return these alt az values.
// TODO delete if not used
void TelescopeModel::calculateEncoderOffsetFromAltAz(float alt, float az,
                                                     long altEncVal,
                                                     long azEncVal,
                                                     long &altEncOffset,
                                                     long &azEncOffset) {

  float calculatedAlt =
      360.0 * ((float)(altEncVal)) / (float)altEncoderStepsPerRevolution;
  float calculatedAz =
      360.0 * ((float)(azEncVal)) / (float)azEncoderStepsPerRevolution;
  altEncOffset = calculatedAlt - alt;
  azEncOffset = calculatedAz - az;
}

void TelescopeModel::calculateAltAzFromEncoders(float &alt, float &az,
                                                long altEncVal, long azEncVal) {
  alt = 360.0 * ((float)(altEncVal)) / (float)altEncoderStepsPerRevolution;

  // float a = 360.0 * ((float)(altEnc - altEncBaseValue)) /
  //           (float)altEncoderStepsPerRevolution;
  az = 360.0 * ((float)(azEncVal)) / (float)azEncoderStepsPerRevolution;
  // float z = 360.0 * ((float)(azEnc - azEncBaseValue)) /
  //           (float)azEncoderStepsPerRevolution;

  // currentAlt = altBaseValue + a;
  // currentAz = azBaseValue + z;

  // handle situation where scope goes "over the top"
  // if (currentAlt > 90) {
  //   currentAlt = 180 - currentAlt;
  //   currentAz += 180;
  //   if (currentAz > 360) {
  //     currentAz -= 360;
  //   }
  // }
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
  double ccAz;
  if (isNorthernHemisphere())
    ccAz = 360.0 - azEncoderDegrees;
  else
    ccAz = azEncoderDegrees;
  // double ccAz = 360.0 - azEncoderDegrees + azOffsetToAddToEncoderResult;
  // double talt = (double)altEncoderDegrees + altOffsetToAddToEncoderResult;
  double talt = (double)altEncoderDegrees;

  double raDeltaDegrees = 0;
  // if (alignment.getRefs() > 0) {
  unsigned long timedelta = timeMillis - alignmentModelSyncTime;
  raDeltaDegrees = millisecondsToRADeltaInDegrees(timedelta);
  // std::cout << "Time delta in degrees :" << raDeltaDegrees << "\n\r";
  // // }

  // std::cout << "calculateposition : ccaz:" << ccAz << " talt: " << talt
  //           << "\n\r";

  // first two get updated
  std::cout << "Az for calc:" << ccAz << "\n\r";
  std::cout << "Alt for calc:" << talt << "\n\r";
  std::cout << "RA Delta:" << raDeltaDegrees << "\n\r";

  alignment.toReferenceDeg(ra, dec, ccAz, talt);
  ra=fmod(fmod(ra, 360) + 360, 360);

  currentRA = ra + raDeltaDegrees;
  currentDec = dec;
  std::cout << "Calculateposition Result : currentRa:" << currentRA
            << " currentDec: " << currentDec << "\n\r";
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
  // when last sync happened, we saved encoded values and ra/dec.
  float lastSyncAltDegrees;
  float lastSyncAzDegrees;
  // convert those saved encoded values to degrees
  calculateAltAzFromEncoders(lastSyncAltDegrees, lastSyncAzDegrees,
                             lastSyncedAltEncoder, lastSyncedAzEncoder);
  double raDeltaDegrees = 0;
  if (alignment.getRefs() > 0) {
    unsigned long timedelta = secondSyncTime - firstSyncTime;

    // std::cout << "lastSyncedTime :" << secondSyncTime << "\n\r";
    // std::cout << "firstSyncTime :" << firstSyncTime << "\n\r";
    // std::cout << "timedelta :" << timedelta << "\n\r";
    // std::cout << "raDeltaDegrees :" << raDeltaDegrees << "\n\r";

    raDeltaDegrees = millisecondsToRADeltaInDegrees(timedelta);
  }

  // std::cout << "adding ref point\n\r";
  // std::cout << "Last RA :" << lastSyncedRa << "\n\r";
  // std::cout << "Adjusted RA :" << lastSyncedRa - raDeltaDegrees << "\n\r";
  // std::cout << "Last Dec :" << lastSyncedDec << "\n\r";
  // std::cout << " lastSyncAzDegrees :" << lastSyncAzDegrees << "\n\r";
  // std::cout << "360- lastSyncAzDegrees :" << 360 - lastSyncAzDegrees <<
  // "\n\r"; std::cout << "lastSyncAltDegrees :" << lastSyncAltDegrees <<
  // "\n\r";

  // reverse az. Adjust ra back to first sync frame of reference.

  std::cout << "RA for ref:" << lastSyncedRa << "\n\r";
  std::cout << "Dec for ref:" << lastSyncedDec << "\n\r";
  std::cout << "Azi for ref:" << lastSyncAzDegrees << "\n\r";
  std::cout << "Alt for ref:" << lastSyncAltDegrees << "\n\r";
  std::cout << "RA Delta:" << raDeltaDegrees << "\n\r";
  if (isNorthernHemisphere()) {
    alignment.addReferenceDeg(lastSyncedRa - raDeltaDegrees, lastSyncedDec,
                              360 - lastSyncAzDegrees, lastSyncAltDegrees);
    std::cout << "North\n\r";
  } else {
    alignment.addReferenceDeg(lastSyncedRa - raDeltaDegrees, lastSyncedDec,
                              lastSyncAzDegrees, lastSyncAltDegrees);

                              
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
 * point sync 2) Calculate local alt/az values, and store them for use as
 * offsets 2) Using stored transformations, calculate current alt/az in degrees
 * based passed ra/dec. Then calculate alt/az using only the encoders. Then
 *    store the delta (error) to be added in calulatePosition().
 *
 *
 */
void TelescopeModel::syncPositionRaDec(float ra, float dec,
                                       unsigned long time) {

  // float offSetInHours =
  //     Ephemeris::hoursMinutesSecondsToFloatingHours(0, 0, raOffset);
  // double offsetInDegrees = 360 * offSetInHours / 24.0;

  // get local alt/az of target.
  // Work out alt/az offset required, such that when added to
  // values calculated from encoder, we end up with this target
  // alt/az

  Ephemeris::setLocationOnEarth(latitude, longitude);

  EquatorialCoordinates eq;
  eq.ra = ra;
  eq.dec = dec;
  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(eq, time);
  std::cout << "Ephemeris time : " << time << "\n\r";
  std::cout << "Ephemeris alt : " << altAzCoord.alt << "\n\r";
  std::cout << "Ephermis az : " << altAzCoord.azi << "\n\r";
  float calculatedAlt, calculatedAz;
  calculateAltAzFromEncoders(calculatedAlt, calculatedAz, altEnc, azEnc);

  std::cout << "Calculated alt from encoders : " << calculatedAlt << "\n\r";
  std::cout << "Calculated az from encoders: " << calculatedAz << "\n\r";
  altOffsetToAddToEncoderResult = altAzCoord.alt - calculatedAlt;
  //  azOffsetToAddToEncoderResult = altAzCoord.azi - calculatedAz;

  std::cout << "Calculated altOffsetToAddToEncoderResult : "
            << altOffsetToAddToEncoderResult << "\n\r";
  std::cout << "Calculated azOffsetToAddToEncoderResult : "
            << azOffsetToAddToEncoderResult << "\n\r";

  lastSyncedRa = ra;
  lastSyncedDec = dec;
  // lastSyncedAltEncoder = altEnc + altOffsetToAddToEncoderResult;
  lastSyncedAltEncoder = altEnc;
  lastSyncedAzEncoder = azEnc;

  // if no refs set, then mark this as t=0
  if (alignment.getRefs() == 0) {
    firstSyncTime = time;
    std::cout << "First sync time set" << firstSyncTime << "\n\r\n\r";
  } else if (alignment.getRefs() == 1) {
    secondSyncTime = time;
    std::cout << "Second sync time set" << secondSyncTime << "\n\r\n\r";
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

void TelescopeModel::setUTCYear(int y) { year = y; }
void TelescopeModel::setUTCMonth(int m) { month = m; }
void TelescopeModel::setUTCDay(int d) { day = d; }
void TelescopeModel::setUTCHour(int h) { hour = h; }
void TelescopeModel::setUTCMinute(int m) { min = m; }
void TelescopeModel::setUTCSecond(int s) { sec = s; }

/**
 * @brief Saves the encoder calibration point.
 *
 * Used for working out whether encoders are calibrated, ie steps per
 * revoloution. It first calculates horizontal coordinates (alt/az) from the
 * current equatorial coordinates (ra/dec), on a specific date and time. Then,
 * it stores the old alignment values, and updates the new alignment values
 * based on the calculated alt/az values and the current encoder readings.
 */
void TelescopeModel::saveEncoderCalibrationPoint() {
  // Initialize equatorial coordinates with current  position. Assumed to be
  // set before calling.
  EquatorialCoordinates baseEqCoord;
  baseEqCoord.ra = raBasePos;
  baseEqCoord.dec = decBasePos;

  // Convert current equatorial coordinates to horizontal coordinates for this
  //  date and time (assumed to be set before calling)
  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          baseEqCoord, day, month, year, hour, min, sec);

  // Store older alignment values.
  altEncoderAlignValue2 = altEncoderAlignValue1;
  azEncoderAlignValue2 = azEncoderAlignValue1;
  azAlignValue2 = azAlignValue1;
  altAlignValue2 = altAlignValue1;

  // Update new alignment values using the converted horizontal coordinates
  // and current encoder readings.
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
