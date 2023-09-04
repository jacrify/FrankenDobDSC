#ifndef TELESCOPE_MODEL_H
#define TELESCOPE_MODEL_H
#include <Ephemeris.h>

class TelescopeModel {
public:
  TelescopeModel();
   void setEncoderValues(long encAlt, long encAz);
  void setAzEncoderStepsPerRevolution(long altResolution);
  void setAltEncoderStepsPerRevolution(long altResolution);

  void setLatitude(float lat);
  void setLongitude(float lng);

  float getLatitude();
  float getLongitude();

  float getAltCoord();
  float getAzCoord();

  float getDecCoord();
  float getRACoord();

  void setPositionRaDec(float ra, float dec);

  void setUTCYear(int year);
  void setUTCMonth(int month);
  void setUTCDay(int day);
  void setUTCHour(int hour);
  void setUTCMinute(int min);
  void setUTCSecond(int sec);
  void calculateCurrentPosition();

  //Calculate in the future (if positive) or into past (if negative)
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
  //known encoder values at same
  long altEncBaseValue;
  long azEncBaseValue;
  float ra;
  float dec;

  double deltaSeconds;

};

#endif
