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

  baseSyncPoint.isValid = false;
  performBaselineAlignment();
}

// Find the farthest two points from passed synchpoint.
// Returns vector containing passed point, and two farthest points
std::vector<SynchPoint>
TelescopeModel::findFarthest(SynchPoint &sp,
                             std::vector<SynchPoint> &baseSyncPoints) {
  SynchPoint maxDistPoint1;
  SynchPoint maxDistPoint2;
  double maxDistance1 = -1.0;
  double maxDistance2 = -1.0;

  for (auto &element : baseSyncPoints) {
    double distance = sp.eqCoord.calculateDistanceInDegrees(element.eqCoord);
    if (distance > maxDistance1) {
      maxDistPoint2 = maxDistPoint1; // Previous farthest point now becomes
                                     // second farthest
      maxDistance2 = maxDistance1;
      maxDistPoint1 = element; // New farthest point
      maxDistance1 = distance;
    } else if (distance > maxDistance2) {
      maxDistPoint2 = element; // New second farthest point
      maxDistance2 = distance;
    }
  }

  std::vector<SynchPoint> response;
  if (maxDistPoint1.isValid && maxDistPoint2.isValid) {
    response.push_back(sp);
    response.push_back(maxDistPoint1);
    response.push_back(maxDistPoint2);
  }
  return response;
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

  currentEqPosition = alignment.toReferenceCoord(encoderAltAz);
  // log("Base position\t\t\tra(h): %lf\tdec: %lf",
  //     currentEqPosition.getRAInHours(),
  //     currentEqPosition.getDecInDegrees());

  // Work out how many seconds since the model was created. Add this
  // to the RA to compensate for time passing.
  double timeDeltaSeconds = 0;
  if (baseSyncPoint.isValid) {
    timeDeltaSeconds = differenceInSeconds(baseSyncPoint.timePoint, timePoint);
  }

  double raDeltaDegrees = secondsToRADeltaInDegrees(timeDeltaSeconds);
  // log("Time delta seconds: %lf degrees: %lf", timeDeltaSeconds,
  // raDeltaDegrees);

  currentEqPosition = currentEqPosition.addRAInDegrees(raDeltaDegrees);

  // log("Final position\t\t\tra(h): %lf\tdec: %lf",
  //     currentEqPosition.getRAInHours(),
  //     currentEqPosition.getDecInDegrees());
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
void TelescopeModel::performOneStarAlignment(HorizCoord &horiz1, EqCoord &eq1,
                                             TimePoint &now) {
  log("Time (local) for one star alignment: %s",
      timePointToString(now).c_str());
  alignment.addReferenceCoord(horiz1, eq1);

  HorizCoord horiz2 = horiz1.addOffset(80, 0);
  // this is calculated based on current lat long time
  EqCoord eq2 = EqCoord(horiz2, now);
  alignment.addReferenceCoord(horiz2, eq2);

  alignment.calculateThirdReference();

  log("===Generated 1 star reference point===");
  log("Point 1: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz1.altInDegrees,
      horiz1.aziInDegrees, eq1.getRAInHours(), eq1.getDecInDegrees());
  log("Point 2: \t\talt: %lf\taz:%lf\tra(h): %lf\tdec:%lf", horiz2.altInDegrees,
      horiz2.aziInDegrees, eq2.getRAInHours(), eq2.getDecInDegrees());
  baseSyncPoint = SynchPoint(eq1, horiz1, now,
                             eq1); // creates syncpoint with no error
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
  performOneStarAlignment(h2, eq, now);
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

  if (points.size() != 3) {
    log("Size of add reference points is %d not 3!", points.size());
    return;
  }
  SynchPoint point1 = points[0];
  SynchPoint point2 = points[1];
  SynchPoint point3 = points[2];

  log("");
  log("=====addReferencePoints====");

  // double raDeltaDegrees = 0;
  alignment.clean();

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

  double p1ToP3TimeInSeconds =
      differenceInSeconds(point1.timePoint, point3.timePoint);
  double p3DegreeDelta = secondsToRADeltaInDegrees(p1ToP3TimeInSeconds);
  EqCoord p3Adjusted = point3.eqCoord.addRAInDegrees(-p3DegreeDelta);

  alignment.addReferenceCoord(point1.encoderAltAz, point1.eqCoord);
  alignment.addReferenceCoord(point2.encoderAltAz, p2Adjusted);
  alignment.addReferenceCoord(point3.encoderAltAz, p3Adjusted);

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
 * After that, whenever the user platesolves, a three star alignment
 * will be performed using the platesolved point, and the two alignment
 * points that are farthest away. This means that after a platesolve
 * the model should point to the exact location of the scope.
 *
 * That location is then discarded after the next platesolve:
 * the original three remain.
 *
 * Whenever a model is built, one of the points is picked as the
 * base syncpoint. When the model is queried later, the difference
 * between that time and the timestamp of this base point is then
 * used to offset the ra output of the model, to model the stars
 * moving since the model was made. The other points syncpoints
 * are also grounded back to this base syncpoint time.
 *
 * The time of the syncpoints is also adjusted at creation
 * by the sidereal offset of the eq platform, if present, similar
 * to the calculateCurrentPositino logic.
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

  log("Calculating expected alt az bazed on \tra(h): %lf \tdec: %lf and  "
      "time "
      "%s",
      lastSyncedEq.getRAInHours(), lastSyncedEq.getDecInDegrees(),
      timePointToString(now).c_str());

  HorizCoord modeledAltAz = alignment.toInstrumentCoord(lastSyncedEq);
  log("Calculated alt/az from model\t\talt: %lf\t\taz:%lf",
      modeledAltAz.altInDegrees, modeledAltAz.aziInDegrees);

  HorizCoord calculatedAltAzFromEncoders =
      calculateAltAzFromEncoders(altEnc, azEnc);
  // assumes alt is zeroed to horizon at power on

  log("Calculated alt/az from encoders\t\talt: %lf\t\taz:%lf",
      calculatedAltAzFromEncoders.altInDegrees,
      calculatedAltAzFromEncoders.aziInDegrees);

  // check where model would say we are. Stored in currentEqPosition to use as
  // error calc
  calculateCurrentPosition(now);

  SynchPoint thisSyncPoint = SynchPoint(
      lastSyncedEq, calculatedAltAzFromEncoders, now, currentEqPosition);

  thisSyncPoint.altEncoder = altEnc;
  thisSyncPoint.azEncoder = azEnc;
  // compare to last

  if (lastSyncPoint.isValid) {
    calculatedAltEncoderRes =
        calculateAltEncoderStepsPerRevolution(thisSyncPoint, lastSyncPoint);
    calculatedAziEncoderRes =
        calculateAzEncoderStepsPerRevolution(thisSyncPoint, lastSyncPoint);
  }
  lastSyncPoint = thisSyncPoint;

  if (baseAlignmentSynchPoints.size() == 3) {
    std::vector<SynchPoint> furthest =
        findFarthest( lastSyncPoint, baseAlignmentSynchPoints);
    addReferencePoints(furthest);
  } else {
    baseAlignmentSynchPoints.push_back(lastSyncPoint);
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
long TelescopeModel::calculateAzEncoderStepsPerRevolution(SynchPoint &startPoint,
                                                          SynchPoint &endPoint) {

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
