#include "telescopeModel.h"
#include "Logging.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unity.h>

void TelescopeModel::performBaselineAlignment() {
  HorizCoord h = HorizCoord(0, 0);
  EqCoord eq = EqCoord(180, 0);
  alignment.setNorthernHemisphere(false);
  alignment.addReferenceCoord(h, eq);
  h = HorizCoord(0, 90);
  eq = EqCoord(180, 90);
  alignment.addReferenceCoord(h, eq); // degreess
  alignment.calculateThirdReference();
  defaultAlignment = true;
}
TelescopeModel::TelescopeModel() {
  // Default alignment for alt az in south
  // alignment.addReference(0, 0, M_PI, 0);//radians
  performBaselineAlignment();
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

  // firstSyncTime = getNow();
  // secondSyncTime = getNow();
  // alignmentModelSyncTime = getNow();

  // errorToAddToEncoderResultAlt = 0;
  // errorToAddToEncoderResultAzi = 0;
}

void TelescopeModel::clearAlignment() {

  synchPoints.clear();
  baseSyncPoint = SynchPoint();
  baseSyncPoint.isValid = false;
  performBaselineAlignment();
}
SynchPoint TelescopeModel::addSynchPointAndFindFarthest(SynchPoint sp,
                                                        double trimRadius) {

  SynchPoint maxDistPoint;
  double maxDistance = -1.0;

  auto it = std::remove_if(
      synchPoints.begin(), synchPoints.end(), [&](const SynchPoint &point) {
        double distance = sp.eqCoord.calculateDistanceInDegrees(point.eqCoord);

        // Only update maxDistance and maxDistPoint for points that are not
        // being removed
        if (distance >= trimRadius && distance > maxDistance) {
          maxDistance = distance;
          maxDistPoint = point;
        }

        return distance < trimRadius;
      });

  synchPoints.erase(it, synchPoints.end());
  synchPoints.push_back(sp);

  if (maxDistance < 0) {
    return SynchPoint(); // Return an invalid SynchPoint
  } else {
    maxDistPoint.isValid =
        true; // Ensure the returned SynchPoint is marked as valid
    return maxDistPoint;
  }
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
  // log("");
  // log("=====calculateCurrentPosition====");

  float altEncoderDegrees;
  float azEncoderDegrees;
  // convert encoder values to degrees
  HorizCoord encoderAltAz = calculateAltAzFromEncoders(altEnc, azEnc);
  // log("Raw Alt az from encoders: \t\talt: %lf\taz:%lf\tat time:%s",
  //     encoderAltAz.altInDegrees, encoderAltAz.aziInDegrees,
  //     timePointToString(timePoint).c_str());

  currentEqPosition = alignment.toReferenceCoord(encoderAltAz);

  // Work out how many seconds since the model was created. Add this
  // to the RA to compensate for time passing.
  unsigned long timeDeltaSeconds = 0;
  if (baseSyncPoint.isValid) {
    timeDeltaSeconds = differenceInSeconds(baseSyncPoint.timePoint, timePoint);
  }

  double raDeltaDegrees = secondsToRADeltaInDegrees(timeDeltaSeconds);
  // log("Time delta seconds: %ld degrees: %lf", timeDeltaSeconds, raDeltaDegrees);
`
  currentEqPosition = currentEqPosition.addRAInDegrees(raDeltaDegrees);

  // log("Final position\t\t\tra(h): %lf\tdec: %lf",
  //     currentEqPosition.getRAInHours(), currentEqPosition.getDecInDegrees());
  // log("=====calculateCurrentPosition====");
  // log("");
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
  log("Time (local) for one star alignment: %ld",
      timePointToString(now).c_str());

  HorizCoord horiz2 = horiz1.addOffset(91, 0);
  // this is calculated based on current lat long time

  EqCoord eq2 = EqCoord(horiz2, now);
  alignment.addReferenceCoord(horiz2, eq2);

  alignment.calculateThirdReference();

  log("===Generated 1 star reference point===");
  log("Point 1: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz1.altInDegrees,
      horiz1.aziInDegrees, eq1.getRAInHours(), eq1.getDecInDegrees());
  log("Point 2: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz2.altInDegrees,
      horiz2.aziInDegrees, eq2.getRAInHours(), eq2.getDecInDegrees());
  baseSyncPoint =
      SynchPoint(eq1, horiz1, now, eq1); // creates syncpoint with no error
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
// void TelescopeModel::addReferencePoint() {
//   log("");
//   log("=====addReferencePoint====");
//   double raDeltaDegrees = 0;
//   if (alignment.getRefs() > 0) {
//     double timeDelta = differenceInSeconds(firstSyncTime, secondSyncTime);

//     raDeltaDegrees = secondsToRADeltaInDegrees(timeDelta);
//   }
//   // BUG? what happens when platform is running?
//   //

//   EqCoord adjustedLastSyncEQ = lastSyncedEq.addRAInDegrees(-raDeltaDegrees);
//   alignment.addReferenceCoord(lastSyncedHoriz, lastSyncedEq);

//   if (alignment.getRefs() == 2) {
//     log("Got two refs, calculating model...");
//     alignment.calculateThirdReference();
//     alignmentModelSyncTime = firstSyncTime; // store this so future partial
//                                             // syncs don't clobber
//   }

//   log("=====addReferencePoint====");
//   log("");
// }

void TelescopeModel::addReferencePoints(SynchPoint oldest, SynchPoint newest) {
  log("");
  log("=====addReferencePoints====");

  double raDeltaDegrees = 0;
  alignment.clean();

  alignment.addReferenceCoord(oldest.horizCoord, oldest.eqCoord);

  // compensate for time gap between two points.
  double timeDelta = differenceInSeconds(oldest.timePoint, newest.timePoint);
  raDeltaDegrees = secondsToRADeltaInDegrees(timeDelta);
  EqCoord adjustedLastSyncEQ = newest.eqCoord.addRAInDegrees(-raDeltaDegrees);
  // BUG? what happens when platform is running?
  //
  alignment.addReferenceCoord(newest.horizCoord, adjustedLastSyncEQ);

  log("Got two refs, calculating model...");
  alignment.calculateThirdReference();

  // syncs don't clobber

  log("=====addReferencePoints====");
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
 * 1) Calculates actual alt/az using encoders
 *
 * 2) If this is the first reference point captured, do a 1 star align.
 *
 * 3) Calculate current position using current model (used for error calcs)
 *
 * 4) Using the passed ta, encoder horizontal pos, and calculated, create a sync
 * point
 *
 * 5) Add the sync point to the list. This checks for historical sync points
 * that are far away from this one. If one is found, it is returned
 *
 * 6) If this far sync point is returned, then recalc the model using both
 * points.
 *
 * 7)  Saves the encoder values in case we want to use it to work out encoder
 * resolutions
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

  EqCoord lastSyncedEq;
  lastSyncedEq.setRAInHours(raInHours);
  lastSyncedEq.setDecInDegrees(decInDegrees);

  log("Calculating expected alt az bazed on \tra(h): %lf \tdec: %lf and  time "
      "%s",
      lastSyncedEq.getRAInHours(), lastSyncedEq.getDecInDegrees(),
      timePointToString(now).c_str());

  HorizCoord calculatedAltAzFromEncoders =
      calculateAltAzFromEncoders(altEnc, azEnc);
  // assumes alt is zeroed to horizon at power on

  log("Calculated alt/az from encoders\t\talt: %lf\t\taz:%lf",
      calculatedAltAzFromEncoders.altInDegrees,
      calculatedAltAzFromEncoders.aziInDegrees);

  // check where model would say we are. Stored in currentEqPosition to use as error calc
  calculateCurrentPosition(now);

  lastSyncPoint = SynchPoint(lastSyncedEq, calculatedAltAzFromEncoders, now,
                             currentEqPosition);

  lastSyncPoint.altEncoder = altEnc;
  lastSyncPoint.azEncoder = azEnc;

  SynchPoint furthest = addSynchPointAndFindFarthest(
      lastSyncPoint, SYNCHPOINT_FILTER_DISTANCE_DEGREES);

  if (furthest.isValid) {
    log("Recalculating model...");
    addReferencePoints(furthest, lastSyncPoint);
    baseSyncPoint = furthest;

  } else {
    log("No usable syncpoint found...");
  }

  // special case: if this is the first alignment, then do a special one star
  // alignment
  if (defaultAlignment) {
    performOneStarAlignment(calculatedAltAzFromEncoders, lastSyncedEq, now);
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

  // TODO I don't think these work. They just use the calculated values as
  // input, but should use modeled. ie we should translate ra/dec to alt/az, and
  // use those.
  //  Calculate difference in azimuth encoder alignment values.
  long encoderMove = lastSyncPoint.azEncoder - baseSyncPoint.azEncoder;

  // Calculate difference in azimuth alignment values.
  float coordMove = lastSyncPoint.horizCoord.aziInDegrees -
                    baseSyncPoint.horizCoord.aziInDegrees;

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
  long encoderMove = lastSyncPoint.altEncoder - baseSyncPoint.altEncoder;

  // Calculate difference in altitude alignment values
  float coordMove = lastSyncPoint.horizCoord.altInDegrees -
                    baseSyncPoint.horizCoord.altInDegrees;

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
