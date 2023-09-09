#ifndef TELESCOPE_MODEL_H
#define TELESCOPE_MODEL_H
#include <Ephemeris.h>

class TelescopeModel {
public:
  TelescopeModel();
  void setEncoderValues(long encAlt, long encAz);
  void setAzEncoderStepsPerRevolution(long altResolution);
  void setAltEncoderStepsPerRevolution(long altResolution);
  long getAzEncoderStepsPerRevolution();
  long getAltEncoderStepsPerRevolution();

  void setLatitude(float lat);
  void setLongitude(float lng);

  float getLatitude();
  float getLongitude();

  float getAltCoord();
  float getAzCoord();

  float getDecCoord();
  float getRACoord();

  void setPositionRaDec(float ra, float dec);
//test
  void setUTCYear(int year);
  void setUTCMonth(int month);
  void setUTCDay(int day);
  void setUTCHour(int hour);
  void setUTCMinute(int min);
  void setUTCSecond(int sec);
  void calculateCurrentPosition();
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
  void setDeltaSeconds(double delta);

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
  float currentRA;
  float currentDec;
  float currentAlt;
  float currentAz;

  // when align is hit, store the values so we can use them to calibrate
  // encoders.
  //Values always get stored in AlignValue1, with existing values copied
  //to alignvalue2 beforehand

  long altEncoderAlignValue1;
  long altEncoderAlignValue2;
  long azEncoderAlignValue1;
  long azEncoderAlignValue2;

  float altAlignValue1;
  float altAlignValue2;
  float azAlignValue1;
  float azAlignValue2;

  double deltaSeconds;
};

#endif
