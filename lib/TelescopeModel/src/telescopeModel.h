#ifndef TELESCOPE_MODEL_H
#define TELESCOPE_MODEL_H
#include "CoordConv.hpp"
#include "EqCoord.h"
#include "HorizCoord.h"
#include "TimePoint.h"
#include <Ephemeris.h>
#include <vector>

// when adding syncpoints, any existing points closer than this are deleted.
#define SYNCHPOINT_FILTER_DISTANCE_DEGREES 10

struct SynchPoint {
  EqCoord eqCoord;
  HorizCoord encoderAltAz;
  TimePoint timePoint;
  double errorInDegreesAtCreation;
  bool isValid;
  long altEncoder;
  long azEncoder;

  SynchPoint()
      : errorInDegreesAtCreation(0), isValid(false) {
  } // Initialize members as needed

  SynchPoint(const EqCoord &eq, const HorizCoord &encoderHorizontal,
             const TimePoint &tp, const EqCoord &calculatedeq)
      : eqCoord(eq), encoderAltAz(encoderHorizontal), timePoint(tp),
        isValid(true) {
    errorInDegreesAtCreation = eq.calculateDistanceInDegrees(calculatedeq);
  }
};

/**
 *  This class does the heavy lifting of modeling where the telescope
 * is pointing, and storing its internal state. It uses Taki Toshimi's
 * 2 star alignment method (code taken from teenastro), and models the
 * equatorial platform state as a simple offset to in and out ra values.
 *
 * General use cases are:
 * 0) Initialisation. Model is initialised with either saved transformation
 *    matrix, or default reference points.
 * 1) Basic ra/dec sync. Call:
 *    - setEncoderValues() with current encoder values, then
 *    - setRaOffset() with number of seconds in RA ahead/behind (EQ platform)
 *    - setPositionRADec() with where the scope is currently pointing.
 *    setPositionRADec takes current ra and dec, and current time.
 *    These are stored, along with current encoder values, and raoffset.
 *    These are used in calculateCurrentPosition to "zero" the encoders:
 *    all alt/az calculations are made as an offset from this current point.
 *    This allows fine adjustments (eg from platesolving) without recalculating
 *    transformation matrix.
 *
 *    Each time setPositionRADec() is called, the model saves two flavours
 *    of state
 *    a) The values of the last two syncs are always kept,
 *       and can be used to calibrate encoder resolution
 *    b) The last ra/dec/time value is kept, and can be added as a
 *       reference point for rebuilding transformation matrix,
 *       as per next section
 * 2) Rebuild matrix. Call:
 *    - setEncoderValues()
 *    - setPlatformRAOffset() with number of seconds in RA ahead/behind
 *    - setPositionRADec()
 *    - addReferencePoint()
 *    Twice. AddReferencePoint will take the last position calculated.
 *    Once two reference points are added, the new matrix will be calculated.
 *    If the another reference point is added when two exist, old ones
 *    will be cleared (though matrix will still be valid).
 *    TODO does addReferencePoint need to use time offset, or is it cooked
 *    in from setPosition?
 * 3) Find current position. Call:
 *    - setEncoderValues()
 *    - setPlatformRAOffset() with number of seconds in RA ahead/behind
 *    - calculateCurrentPosition() with current time
 *    This will invoke convert the encoder values to alt/az, then invoke the
 *    transformation matrix to derive ra/dec. ra will be adjust based on offset
 *
 *
 *
 *
 *    This method uses the current encoder values to derive al/az
 *

*/
class TelescopeModel {
public:
  TelescopeModel();

  std::vector<SynchPoint> synchPoints;
  EqCoord currentEqPosition;
  SynchPoint lastSyncPoint;

  void clearAlignment();
  void syncPositionRaDec(float ra, float dec, TimePoint tp);
  void calculateCurrentPosition(TimePoint tp);

  void setEncoderValues(long encAlt, long encAz);
  void setAzEncoderStepsPerRevolution(long altResolution);
  void setAltEncoderStepsPerRevolution(long altResolution);
  long getAzEncoderStepsPerRevolution();
  long getAltEncoderStepsPerRevolution();

  void performZeroedAlignment(TimePoint now);

  void setLatitude(float lat);
  void setLongitude(float lng);

  float getLatitude();
  float getLongitude();

  float getAltCoord();
  float getAzCoord();

  float getDecCoord();
  float getRACoord();

  /**
   * This does the following:
   * 1: Iterates through synchPoints and removes all items where the distance
   * (calculated using double calculateDistanceInDegrees(EqCoord delta) )
   * is less than trimRadius.
   * 2: Adds sp to synchPoints
   * 3: Returns the synchPoint where distance is greatests, found in step 1
   * Cannot return sp. Cannot return removed item. If no item return syncpoint
   * with "isvalid" set to false. New one is still added.
   */
  SynchPoint addSynchPointAndFindFarthest(SynchPoint sp, double trimRadius);

  void performOneStarAlignment(HorizCoord altaz, EqCoord eq, TimePoint tp);
  long calculatedAltEncoderRes;
  long calculatedAziEncoderRes;

private:
  float latitude;
  float longitude;

  long altEnc;
  long azEnc;
  long azEncoderStepsPerRevolution;
  long altEncoderStepsPerRevolution;

  CoordConv alignment;
  SynchPoint baseSyncPoint;

  bool defaultAlignment;
  float currentAlt;
  float currentAz;
  double secondsToRADeltaInDegrees(double secondsDelta);
  void performBaselineAlignment();
  void calculateEncoderOffsetFromAltAz(float alt, float az, long altEncVal,
                                       long azEncVal, long &altEncOffset,
                                       long &azEncOffset);
  HorizCoord calculateAltAzFromEncoders(long altEncVal, long azEncVal);

  void addReferencePoints(SynchPoint oldest, SynchPoint newest);
  long calculateAzEncoderStepsPerRevolution(SynchPoint startPoint,
                                            SynchPoint endPoint);
  long calculateAltEncoderStepsPerRevolution(SynchPoint startPoint,
                                             SynchPoint endPoint);
};

#endif
