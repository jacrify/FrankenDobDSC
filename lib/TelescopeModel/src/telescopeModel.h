#ifndef TELESCOPE_MODEL_H
#define TELESCOPE_MODEL_H
#include "CoordConv.hpp"
#include <Ephemeris.h>

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
  void setEncoderValues(long encAlt, long encAz);
  void setAzEncoderStepsPerRevolution(long altResolution);
  void setAltEncoderStepsPerRevolution(long altResolution);
  long getAzEncoderStepsPerRevolution();
  long getAltEncoderStepsPerRevolution();

  void calculateAltAzFromEncoders(float &alt, float &az, long altEncVal,
                                  long azEncVal);

  void calculateEncoderOffsetFromAltAz(float alt, float az, long altEncVal,
                                       long azEncVal, long &altEncOffset,
                                       long &azEncOffset);

  void storeAltAzOffset(float actualAlt, float actualAz, float encoderBasedAlt,
                        float encoderBasedAz);

  void adjustAltAzBasedOnOffsets(float &alt, float &az);

  void addReferencePoint();

  void setLatitude(float lat);
  void setLongitude(float lng);

  float getLatitude();
  float getLongitude();

  float getAltCoord();
  float getAzCoord();

  float getDecCoord();
  float getRACoord();

  void syncPositionRaDec(float ra, float dec, unsigned long time);
  // test
  void setUTCYear(int year);
  void setUTCMonth(int month);
  void setUTCDay(int day);
  void setUTCHour(int hour);
  void setUTCMinute(int min);
  void setUTCSecond(int sec);
  void calculateCurrentPosition(unsigned long timeMillis);
  void saveEncoderCalibrationPoint();

  long getAltEncoderAlignValue1() const;
  long getAltEncoderAlignValue2() const;
  long getAzEncoderAlignValue1() const;
  long getAzEncoderAlignValue2() const;

  float getAltAlignValue1() const;
  float getAltAlignValue2() const;
  float getAzAlignValue1() const;
  float getAzAlignValue2() const;

  long calculateAzEncoderStepsPerRevolution();
  long calculateAltEncoderStepsPerRevolution();

  // Calculate in the future (if positive) or into past (if negative)
  void setRaOffset(double raOffset);

  int year;
  int month;
  int day;
  int hour;
  int min;
  int sec;

  float latitude;
  float longitude;

  long altEnc;
  long azEnc;
  long azEncoderStepsPerRevolution;
  long altEncoderStepsPerRevolution;

  // known eq position at sync
  float raBasePos;
  float decBasePos;
  // known encoder values at same
  long altEncBaseValue;
  long azEncBaseValue;
  unsigned long baseTime;
  double baseRaOffset;

  double altBaseValue;
  double azBaseValue;

  float currentRA;
  float currentDec;
  float currentAlt;
  float currentAz;

  // when align is hit, store the values so we can use them to calibrate
  // encoders.
  // Values always get stored in AlignValue1, with existing values copied
  // to alignvalue2 beforehand

  long altEncoderAlignValue1;
  long altEncoderAlignValue2;
  long azEncoderAlignValue1;
  long azEncoderAlignValue2;

  float altAlignValue1;
  float altAlignValue2;
  float azAlignValue1;
  float azAlignValue2;

  double raOffset;

  double lastSyncedRa;
  double lastSyncedDec;
  double lastSyncedAlt;
  double lastSyncedAz;
  unsigned long secondSyncTime; // timestamp in milliseconds  of second sync
  unsigned long firstSyncTime;  // timestamp in milliseconds  of first sync
  unsigned long alignmentModelSyncTime; // t=0 for post alignment. Based on time
                                        // of first sync

  float altOffsetToAddToEncoderResult;
  float azOffsetToAddToEncoderResult;

  bool isNorthernHemisphere();

  CoordConv alignment;
  double millisecondsToRADeltaInDegrees(unsigned long millisecondsDelta);
};

#endif
