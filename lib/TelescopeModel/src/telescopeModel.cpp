#include "TelescopeModel.h"
#include "Ephemeris.h"
#include "Logging.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <unity.h>
/**
 * Used at boot to provide a starting hardcoded alignment. Will be wrong.
 */
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
  latitude = -34.049120;
  longitude = 151.042100;
  Ephemeris::setLocationOnEarth(latitude, longitude);

  performBaselineAlignment();

  altEnc = 0;
  azEnc = 0;
  altDelta = 0;
  aziDelta = 0;
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
  // TODO #3 Make clearAlignement reset EQ platform time?
  baseAlignmentSynchPoints.clear();
  baseSyncPoint = SynchPoint();
  lastSyncPoint = SynchPoint();
  altDelta = 0;
  aziDelta = 0;
  alignment.reset();
  alignment.clean();
  baseSyncPoint.isValid = false;
  performBaselineAlignment();
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

void TelescopeModel::setLatitude(float lat) {
  latitude = lat;
  Ephemeris::setLocationOnEarth(latitude, longitude);
}
void TelescopeModel::setLongitude(float lng) {
  longitude = lng;
  Ephemeris::setLocationOnEarth(latitude, longitude);
}

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
 * 3. Convert that alt/az to equatorial coords, using the two star aligned
 * model
 * 4. Adjust the ra of the result to reflect time that has passed since model
 * creation
 * @return void
 */
void TelescopeModel::calculateCurrentPosition(TimePoint &timePoint) {
  // log("");
  // log("=====calculateCurrentPosition====");

  float altEncoderDegrees;
  float azEncoderDegrees;
  // convert encoder values to degrees
  HorizCoord encoderAltAz = calculateAltAzFromEncoders(altEnc, azEnc);
  // log("Raw Alt az from encoders: \t\talt: %lf\taz:%lf\tat time:%s",
  //     encoderAltAz.altInDegrees, encoderAltAz.aziInDegrees,
  //     timePointToString(timePoint).c_str());

  HorizCoord offsetAltAz = encoderAltAz.addOffset(altDelta, aziDelta);
  // log("Offset from encoders: \t\talt: %lf\taz:%lf\tat time:%s",
  //     offsetAltAz.altInDegrees, offsetAltAz.aziInDegrees,
  //     timePointToString(timePoint).c_str());

  currentEqPosition = alignment.toReferenceCoord(offsetAltAz);
  // log("Base position\t\t\tra(h): %lf\tdec: %lf",
  //     currentEqPosition.getRAInHours(), currentEqPosition.getDecInDegrees());

  // Work out how many seconds since the model was created. Add this
  // to the RA to compensate for time passing.
  double timeDeltaSeconds = 0;

  if (baseSyncPoint.isValid) {
    // function does timepoint-base.timepoint
    // so will be positive as time moves forward
    timeDeltaSeconds = differenceInSeconds(baseSyncPoint.timePoint, timePoint);
  }

  double raDeltaDegrees = secondsToRADeltaInDegrees(timeDeltaSeconds);
  // log("Time delta seconds: %lf degrees: %lf", timeDeltaSeconds,
  // raDeltaDegrees);

  // this should substract from calculated position as has passed:
  // if scope is pointing to same position, but earth has turned,
  // ra pointed to gets smaller
  currentEqPosition = currentEqPosition.addRAInDegrees(-raDeltaDegrees);

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
 * alignment. Then they can choose to do more accurate manual alignment
 * points.
 */
void TelescopeModel::performOneStarAlignment(SynchPoint &syncPoint) {
  log("Time (local) for one star alignment: %s",
      timePointToString(syncPoint.timePoint).c_str());
  alignment.addReferenceCoord(syncPoint.encoderAltAz, syncPoint.eqCoord);

  HorizCoord horiz2 = syncPoint.encoderAltAz.addOffset(80, 0);
  // this is calculated based on current lat long time
  EqCoord eq2 = EqCoord(horiz2, syncPoint.timePoint);
  alignment.addReferenceCoord(horiz2, eq2);

  alignment.calculateThirdReference();

  log("===Generated 1 star reference point===");
  log("Point 1: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf",
      syncPoint.encoderAltAz.altInDegrees, syncPoint.encoderAltAz.aziInDegrees,
      syncPoint.eqCoord.getRAInHours(), syncPoint.eqCoord.getDecInDegrees());
  log("Point 2: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz2.altInDegrees,
      horiz2.aziInDegrees, eq2.getRAInHours(), eq2.getDecInDegrees());
  baseSyncPoint = syncPoint; // creates syncpoint with no error
}

/**
 * Designed to help do basic polar alignment after startup.
 * Assumes scope has been started level, and pointing along platform south
 * axis. Assumes lat long time has been set. Calculates expected ra/dec for
 * this lat long, and creates basic model. You can the point at a star using
 * scope, and rotate platform until star in planetarium matches.
 */
void TelescopeModel::performZeroedAlignment(TimePoint now) {
  log("Performing zero alignment");

  HorizCoord h = HorizCoord(0, 180); // this is if we were pointing south
  log("Time for zero alignment: %s", timePointToString(now).c_str());
  EqCoord eq = EqCoord(h, now); // uses Epheremis to calculate.

  log("Zero Point: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", h.altInDegrees,
      h.aziInDegrees, eq.getRAInHours(), eq.getDecInDegrees());
  HorizCoord h2 = HorizCoord(0, 0); // this is what encoders will actually read
  SynchPoint oneStarSync = SynchPoint(eq, h2, now, eq, 0, 0);
  performOneStarAlignment(oneStarSync);
}
/**
 * Add a reference point to the two star alignment model.
 * Takes the last synced point as the point to put into the model.
 * Once two points are added, a third is calculated, and then the model is
 * built. The time of the first sync is used as the base time of the model
 * (this is stored in alignmentModelSyncTime for later offsets). As there is a
 * time gap between the first reference point being added and the second, the
 * ra of the second point is adjusted backwards by this time gap.
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

//   EqCoord adjustedLastSyncEQ =
//   lastSyncedEq.addRAInDegrees(-raDeltaDegrees);
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
/**
 * Whenever a model is built, one of the points is picked as the
 * base syncpoint. When the model is queried later, the difference
 * between that time and the timestamp of this base point is then
 * used to offset the ra output of the model, to model the stars
 * moving since the model was made. The other points syncpoints
 * are also grounded back to this base syncpoint time.
 *
 */
void TelescopeModel::addReferencePoints(std::vector<SynchPoint> &points) {

  if (points.size() != 2) {
    log("Size of add reference points is %d not 2!", points.size());
    return;
  }
  SynchPoint point1 = points[0];
  SynchPoint point2 = points[1];
  // SynchPoint point3 = points[2];

  log("");
  log("=====addReferencePoints====");

  // double raDeltaDegrees = 0;
  alignment.clean();
  alignment.reset();

  // align all points to same time
  baseSyncPoint = point1;
  // if p1 is at midnight, and p2 is 60 seconds later
  // p2DegreeDelta=60
  // point2.eqCoord needs ra adjusted BACK so position
  // is where it was 60 seconds ago.
  double p1top2TimeInSeconds =
      differenceInSeconds(point1.timePoint, point2.timePoint);
  double p2DegreeDelta = secondsToRADeltaInDegrees(p1top2TimeInSeconds);

  EqCoord p2Adjusted = point2.eqCoord.addRAInDegrees(-p2DegreeDelta);

  // double p1ToP3TimeInSeconds =
  //     differenceInSeconds(point1.timePoint, point3.timePoint);
  // double p3DegreeDelta = secondsToRADeltaInDegrees(p1ToP3TimeInSeconds);
  // EqCoord p3Adjusted = point3.eqCoord.addRAInDegrees(-p3DegreeDelta);

  alignment.addReferenceCoord(point1.encoderAltAz, point1.eqCoord);
  alignment.addReferenceCoord(point2.encoderAltAz, p2Adjusted);
  alignment.calculateThirdReference();
  // alignment.addReferenceCoord(point3.encoderAltAz, p3Adjusted);

  log("Calculated model from three references...");

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
 * The general approach is:
 *
 * At startup, scope does a rough model based on an assumption
 * we're pointing south at the horizon.
 *
 * Afterwards, the user needs to take three platesolves some
 * distance apart. These are used to build a model, and are
 * stored until the alignment is cleared.
 *
 * After that, whenever the user platesolves, this alignment is
 * used and compared to the platesolve position. Where they differ
 * from what the encoders say, the model is used to calculate
 * an alt/az position, and thus an encoder position. The delta
 * between the actual and derived encoder values is stored
 * and applied to future encoder calcs.
 *
 *
 * Whenever a model is built, one of the points is picked as the
 * base syncpoint. When the model is queried later (calculateCurrentPosition),
 * the difference between that time and the timestamp of this base
 * point is then used to offset the ra output of the model, to model
 * the stars  moving since the model was made. The other points syncpoints
 * are also grounded back to this base syncpoint time.
 *
 * The time of the syncpoints is also adjusted at creation
 * by the sidereal offset of the eq platform, if present, similar
 * to the calculateCurrentPositino logic.
 *
 * When a fourth or firth sync is performed, we only calculate a
 * alt/az adjustment. As the input for this we apply the same
 * delta back to the base syncpoint.
 *
 * The way time shifts work is a bit tricky:
 *
 * Point 1:
 * Consider the scope pointing to ra 0 with the platform centered and off.
 * At 12:00 am, we platesolve at this point.
 *
 * Point 2:
 * Then we shift the scope by ra + 10 minutes and wait 10 minutes,
 * then take another platesolve.
 * During this time the planet rotates. The scope is now pointing at
 * ra 20 .
 * When this second point is used for building the model, we calculate
 * point 2 time - point 1 time = 10-0, and subtract that from point2 ra
 * to get an ra of 10 degrees to be used in the model (same as if we'd
 * platsolved straight away).
 *
 * Point 3:
 * Now we shift the scope another + 10 ra so it is pointing at ra 30.
 * We turn the platform on and wait ten minutes, then platesolve again
 *
 * The planet rotates +10 ra, but this is cancelled by the platform, so
 * the scope is still pointing at 30 ra.
 *
 * We do the same math as before: point 3 time-point 1 time = 20-0,
 * and subtract that from point 3 ra to get an ra of 10 degreess to be
 * used in the model.
 *
 * However now the platform emits a delta of -10 minutes to center.
 * This is subtracted from the platesolved ra to get ra 30 to be used in the
 * model. (This is done in platform.calculateAdjustedTime(), and
 * adjusts the timepoint passed into this method before passing)
 *
 *
 *
 * Performs the following steps:
 * 1) Calculates actual alt/az using encoders
 *
 * 2) If this is the first reference point captured, do a 1 star align.
 *
 * 3) Calculate current position using current model (used for error calcs)
 *
 * 4) Using the passed ta, encoder horizontal pos, and calculated, create a
 * sync point
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
                                       TimePoint &now) {
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

  log("Calculating expected alt az bazed on \tra(degrees): %lf \tdec: %lf "
      "and  "
      "time "
      "%s",
      lastSyncedEq.getRAInDegrees(), lastSyncedEq.getDecInDegrees(),
      timePointToString(now).c_str());

  // HorizCoord modeledAltAz = alignment.toInstrumentCoord(lastSyncedEq);
  // log("Calculated alt/az from model\t\talt: %lf\t\taz:%lf",
  //     modeledAltAz.altInDegrees, modeledAltAz.aziInDegrees);

  HorizCoord calculatedAltAzFromEncoders =
      calculateAltAzFromEncoders(altEnc, azEnc);
  // assumes alt is zeroed to horizon at power on

  log("Calculated alt/az from encoders\t\talt: %lf\t\taz:%lf",
      calculatedAltAzFromEncoders.altInDegrees,
      calculatedAltAzFromEncoders.aziInDegrees);

  // check where model would say we are. Stored in currentEqPosition to use as
  // error calc. Use unadjusted values, deltas get recalced further down.
  altDelta = 0;
  aziDelta = 0;
  calculateCurrentPosition(now);

  SynchPoint thisSyncPoint =
      SynchPoint(lastSyncedEq, calculatedAltAzFromEncoders, now,
                 currentEqPosition, altEnc, azEnc);

  // compare to last to use for encoder calibration

  if (lastSyncPoint.isValid) {
    calculatedAltEncoderRes =
        calculateAltEncoderStepsPerRevolution(thisSyncPoint, lastSyncPoint);
    calculatedAziEncoderRes =
        calculateAzEncoderStepsPerRevolution(thisSyncPoint, lastSyncPoint);
  }
  lastSyncPoint = thisSyncPoint;

  if (baseAlignmentSynchPoints.size() == 2) {
    // Adjust time back to model time
    double basePointToNowTimeInSeconds =
        differenceInSeconds(baseSyncPoint.timePoint, now);
    double basePointToNowDeltaInDegrees =
        secondsToRADeltaInDegrees(basePointToNowTimeInSeconds);

    // where was this point at time of model creation?
    // basePointToNowDeltaInDegrees is positive as time moves forward
    // and a fixed alt az point results in ra decreasing over time
    // so to find ra of point some time ago we add the delta
    EqCoord adjusted =
        lastSyncedEq.addRAInDegrees(basePointToNowDeltaInDegrees);
    log("Adjusted ra (degrees) %lf", adjusted.getRAInDegrees());

    // altDelta and aziDelta will be ADDED to encoder values in
    // calculateAlzAzFromEncoders.
    // So if modeled alt encoder is 100, but actualy is 50,
    // modeleded-alt=50, and when calculating we get the right result.
    HorizCoord modeledAltAz = alignment.toInstrumentCoord(adjusted);
    altDelta =
        modeledAltAz.altInDegrees - calculatedAltAzFromEncoders.altInDegrees;
    aziDelta =
        modeledAltAz.aziInDegrees - calculatedAltAzFromEncoders.aziInDegrees;

    log("3 points in base aligment. Calculated alt offset: %ld and az offset "
        ": "
        "%ld",
        altDelta, aziDelta);
    calculateCurrentPosition(now);
  } else {
    log("Adding new point to base alignment, total will be %d ",
        baseAlignmentSynchPoints.size() + 1);
    log("Last sync point ra: %lf", lastSyncPoint.eqCoord.getRAInDegrees());

    baseAlignmentSynchPoints.push_back(lastSyncPoint);
    if (baseAlignmentSynchPoints.size() == 2) {
      addReferencePoints(baseAlignmentSynchPoints);
    }
  }

  // special case: if this is the first alignment, then do a one star
  // alignment. This give us rough positions for doing 3 star
  if (defaultAlignment) {
    log("First point added, doing one off one star alignement");

    performOneStarAlignment(thisSyncPoint);
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
 * Assumes the move between last sync point and current sync point
 * was an azi move only.
 *
 * Calculates the distance in degrees between the two ra/dec
 * positions.
 *
 * Interpolates that onto encoder values
 *
 * @return Number of encoder steps required for a full 360° azimuth
 * revolution.
 */
long TelescopeModel::calculateAzEncoderStepsPerRevolution(
    SynchPoint &startPoint, SynchPoint &endPoint) {

  EqCoord start = startPoint.eqCoord;
  EqCoord end = endPoint.eqCoord;

  double distance = start.calculateDistanceInDegrees(end);
  double encoderMove = abs(startPoint.azEncoder - endPoint.azEncoder);

  double stepsPerDegree = encoderMove / distance;
  long stepsPerRevolution = stepsPerDegree * 360;
  log("Azi Encoder Res Calcs:Distance: %f Encoder Move: %lf Steps per "
      "degree: "
      "%f Calculated steps per revolution  : %ld",
      distance, encoderMove, stepsPerDegree, stepsPerRevolution);
  // Return steps per full revolution (360°) by scaling up stepsPerDegree.
  return stepsPerRevolution;
}

/**
 * @brief Calculates the number of encoder steps per hypothetical full 360°
 * altitude revolution.
 *
 * Assumes the move between last sync point and current sync point
 * was an alt move only.
 * @return Number of encoder steps required for a hypothetical full 360°
 * altitude revolution.
 */
long TelescopeModel::calculateAltEncoderStepsPerRevolution(
    SynchPoint &startPoint, SynchPoint &endPoint) {

  EqCoord start = startPoint.eqCoord;
  EqCoord end = endPoint.eqCoord;

  double distance = start.calculateDistanceInDegrees(end);

  double encoderMove = abs(startPoint.altEncoder - endPoint.altEncoder);
  double stepsPerDegree = encoderMove / distance;
  long stepsPerRevolution = stepsPerDegree * 360;
  log("Alt Encoder Res Calcs: Distance: %f Encoder Move: %lf Steps per "
      "degree: "
      "%f Calculated steps "
      "per revolution  : %ld",
      distance, encoderMove, stepsPerDegree, stepsPerRevolution);
  // Return steps per full revolution (360°) by scaling up stepsPerDegree.
  return -stepsPerRevolution;
}
