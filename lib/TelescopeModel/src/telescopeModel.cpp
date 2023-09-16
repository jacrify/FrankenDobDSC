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
  HorizCoord h = HorizCoord(0, 0);
  EqCoord eq = EqCoord(180, 0);
  alignment.setNorthernHemisphere(false);
  alignment.addReferenceCoord(h, eq);
  h = HorizCoord(0, 90);
  eq = EqCoord(180, 90);
  alignment.addReferenceCoord(h, eq); // degreess
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

  firstSyncTime = getNow();
  secondSyncTime = getNow();
  alignmentModelSyncTime = getNow();

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

float TelescopeModel::getAltCoord() { return currentAlt; }
float TelescopeModel::getAzCoord() { return currentAz; }

/**
 * Perform straight interpolation to alt/az using encoders
 */
HorizCoord TelescopeModel::calculateAltAzFromEncoders(long altEncVal,
                                                      long azEncVal) {

  float alt =
      360.0 * ((float)(altEncVal)) / (float)altEncoderStepsPerRevolution;
  float az = 360.0 * ((float)(azEncVal)) / (float)azEncoderStepsPerRevolution;
  return HorizCoord(alt, az);
}

/**
 * @brief Calculates the current position of the telescope based on encoder
 * values, at a point in time.
 *
 * This method performs the following steps:
 * 1. Calculates alt az positon using encoder values
 * 2. Adjust that position based on error offsets, calcuated at last sync
 * 3. Convert that alt/az to equatorial coords, using the two star aligned model
 * 4. Adjust the ra of the result to reflect time that has passed since model
 * creation
 * @return void
 */
void TelescopeModel::calculateCurrentPosition(TimePoint timePoint) {
  log("");
  log("=====calculateCurrentPosition====");

  float altEncoderDegrees;
  float azEncoderDegrees;
  // convert encoder values to degrees
  HorizCoord encoderAltAz = calculateAltAzFromEncoders(altEnc, azEnc);
  log("Raw Alt az from encoders: \t\talt: %lf\taz:%lf",
      encoderAltAz.altInDegrees, encoderAltAz.aziInDegrees);

  HorizCoord adjustedAltAz = encoderAltAz.addOffset(
      errorToAddToEncoderResultAlt, errorToAddToEncoderResultAzi);

  log("Offset alt az from encoders: \talt: %lf\taz:%lf",
      adjustedAltAz.altInDegrees, adjustedAltAz.aziInDegrees);

  currentEqPosition = alignment.toReferenceCoord(adjustedAltAz);

  // Work out how many seconds since the model was created. Add this
  // to the RA to compensate for time passing.
  unsigned long timeDeltaSeconds =
      differenceInSeconds(alignmentModelSyncTime, timePoint);

  double raDeltaDegrees = secondsToRADeltaInDegrees(timeDeltaSeconds);
  log("Time delta seconds: %ld degrees: %lf", timeDeltaSeconds, raDeltaDegrees);

  currentEqPosition = currentEqPosition.addRAInDegrees(raDeltaDegrees);

  log("Final position\t\t\tra(h): %lf\tdec: %lf",
      currentEqPosition.getRAInHours(), currentEqPosition.getDecInDegrees());
  log("=====calculateCurrentPosition====");
  log("");
}

float TelescopeModel::getDecCoord() {
  return currentEqPosition.getDecInDegrees();
}
float TelescopeModel::getRACoord() { return currentEqPosition.getRAInHours(); }

/**
 * Convert a time period, in seconds to an ra delta.
 * Used to work out how much to adjust ra by over time.
 */
double TelescopeModel::secondsToRADeltaInDegrees(double secondsDelta) {
  double RA_delta_degrees = (secondsDelta / (24.0 * 3600.0)) * 360.0;
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
                                             TimePoint now) {

  alignment.addReferenceCoord(horiz1, eq1);

  HorizCoord horiz2 = horiz1.addOffset(90, 0);
  EqCoord eq2 = EqCoord(horiz2, now);
  alignment.addReferenceCoord(horiz2, eq2);

  alignment.calculateThirdReference();

  log("===Generated 1 star reference point===");
  log("Point 1: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz1.altInDegrees,
      horiz1.aziInDegrees, eq1.getRAInHours(), eq1.getDecInDegrees());
  log("Point 2: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz2.altInDegrees,
      horiz2.aziInDegrees, eq2.getRAInHours(), eq2.getDecInDegrees());
  alignmentModelSyncTime = now;
}

/**
 * Add a reference point to the two star alignment model.
 * Takes the last synced point as the point to put into the model.
 * Once two points are added, a third is calculated, and then the model is
 * built. The time of the first sync is used as the base time of the model (this
 * is stored in alignmentModelSyncTime for later offsets).
 * As there is a time gap between the first reference point being added and the
 * second, the ra of the second point is adjusted backwards by this time gap.
 */
void TelescopeModel::addReferencePoint() {
  log("");
  log("=====addReferencePoint====");
  double raDeltaDegrees = 0;
  if (alignment.getRefs() > 0) {
    double timeDelta = differenceInSeconds(firstSyncTime, secondSyncTime);

    raDeltaDegrees = secondsToRADeltaInDegrees(timeDelta);
  }
  EqCoord adjustedLastSyncEQ = lastSyncedEq.addRAInDegrees(-raDeltaDegrees);
  alignment.addReferenceCoord(lastSyncedHoriz, lastSyncedEq);

  if (alignment.getRefs() == 2) {
    log("Got two refs, calculating model...");
    alignment.calculateThirdReference();
    alignmentModelSyncTime = firstSyncTime; // store this so future partial
                                            // syncs don't clobber
  }

  log("=====addReferencePoint====");
  log("");
}
/**
 * @brief calibrates a position in the sky with current encoder values
 *
 * Called after a plate solve, or Sky Sarafi, passed an ra/dec.
 * Lat long and time are assumed to have been set (sky sarafi does this at
 * connect).
 *
 * Performs the following steps:
 * 1) Saves ra/dec as lastSyncedEq
 * 2) Converts this ra/dec into local alt az based on tie and location (not
 * using alignment model). This is where we'd expect the scope to be pointing,
 * all things being equal.
 * 3) Calculates actual alt/az using encoders
 * 4) Store the delta between actual and expect alt/az as an error offset. This
 * will be added to all position responses going forward. This allows us not to
 * set a neutral position for the encoders, and also gives us a "two speed"
 * system: the two star alignment model gives broad stroke location, then the
 * encoders can be used to find targets in a local area. Encoder errors matter
 * most over long distances, this approach minimises them.
 * 5) Saves the encoder values in case we want to use it to work out encoder resolutions
 * 6) If this is the first sync after startup, do a special one off one star align
 * 
 * */
void TelescopeModel::syncPositionRaDec(float raInHours, float decInDegrees,
                                       TimePoint now) {
  log("");
  log("=====syncPositionRaDec====");

  // get local alt/az of target.
  // Work out alt/az offset required, such that when added to
  // values calculated from encoder, we end up with this target
  // alt/az

  Ephemeris::setLocationOnEarth(latitude, longitude);
  Ephemeris::flipLongitude(false); // East is negative and West is positive

  lastSyncedEq.setRAInHours(raInHours);
  lastSyncedEq.setDecInDegrees(decInDegrees);

  log("Calculating expected alt az bazed on \tra(h): %lf \tdec: %lf and epoch "
      "time %s",
      lastSyncedEq.getRAInHours(), lastSyncedEq.getDecInDegrees(),
      timePointToString(now).c_str());

  //this uses lat/long/time to work out where scope should be pointing.
  lastSyncedHoriz = HorizCoord(lastSyncedEq, now);

  log("Expected local altaz\t\t\talt: %lf \t\taz:%lf",
      lastSyncedHoriz.altInDegrees, lastSyncedHoriz.aziInDegrees);
  log("Actual encoder values\t\t\talt: %ld \t\taz:%ld", altEnc, azEnc);

  HorizCoord calculatedAltAzFromEncoders =
      calculateAltAzFromEncoders(altEnc, azEnc);

  log("Calculated alt/az from encoders\t\talt: %lf\t\taz:%lf",
      calculatedAltAzFromEncoders.altInDegrees,
      calculatedAltAzFromEncoders.aziInDegrees);

  errorToAddToEncoderResultAlt =
      lastSyncedHoriz.altInDegrees - calculatedAltAzFromEncoders.altInDegrees;
  errorToAddToEncoderResultAzi =
      lastSyncedHoriz.aziInDegrees - calculatedAltAzFromEncoders.aziInDegrees;

  log("Stored alt/az delta\t\t\talt: %lf\t\taz:%lf",
      errorToAddToEncoderResultAlt, errorToAddToEncoderResultAzi);

  // these are used for adding reference points to the model

  // these are used for calibrating encoders. Some duplication here.
  //  Shuffle older alignment values into position 2
  //TODO make these coords
  altEncoderAlignValue2 = altEncoderAlignValue1;
  azEncoderAlignValue2 = azEncoderAlignValue1;
  azAlignValue2 = azAlignValue1;
  altAlignValue2 = altAlignValue1;

  // Update new alignment values
  azAlignValue1 = lastSyncedHoriz.aziInDegrees;
  altAlignValue1 = lastSyncedHoriz.altInDegrees;

  altEncoderAlignValue1 = altEnc;
  azEncoderAlignValue1 = azEnc;

  // if no refs set, then mark this as t=0
  if (alignment.getRefs() == 0) {
    firstSyncTime = now;
  } else if (alignment.getRefs() == 1) {
    secondSyncTime = now;
  }

  // special case: if this is the first alignment, then do a special one star
  // alignment
  if (defaultAlignment) {
    performOneStarAlignment(lastSyncedHoriz, lastSyncedEq, now);
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
