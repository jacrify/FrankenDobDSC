#include "telescopeModel.h"
#include <cmath>

TelescopeModel::TelescopeModel() {

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

  // known eq position at sync
  raBasePos = 0;
  decBasePos = 0;
  // known encoder values at same
  altEncBaseValue = 0;
  azEncBaseValue = 0;
  ra = 0;
  dec = 0;
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
  // Set location on earth for horizontal coordinates transformations
  Ephemeris::setLocationOnEarth(latitude, longitude);
}
void TelescopeModel::setLongitude(float lng) {
  longitude = lng;
  Ephemeris::setLocationOnEarth(latitude, longitude);
}

float TelescopeModel::getLatitude() { return latitude; }
float TelescopeModel::getLongitude() { return longitude; }

float TelescopeModel::getAltCoord() {}
float TelescopeModel::getAzCoord() {}

void TelescopeModel::calculateCurrentPosition() {

  // start by converting calibrated base eq coords to alt az
  EquatorialCoordinates baseEqCoord;
  // should be arcturus
  baseEqCoord.ra = raBasePos;
  baseEqCoord.dec = decBasePos;

  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          baseEqCoord, day, month, year, hour, min, sec);
  // add endcoder values since then
  float a = 360.0 * ((float)(altEnc - altEncBaseValue)) /
            (float)altEncoderStepsPerRevolution;
  altAzCoord.alt += a;

  float z = 360.0 * ((float)(azEnc - azEncBaseValue)) /
            (float)azEncoderStepsPerRevolution;
  altAzCoord.azi += z;

  // convert back to eq
  EquatorialCoordinates adjustedEqCoord =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
          altAzCoord, day, month, year, hour, min, sec);
  ra = fmod(adjustedEqCoord.ra, 24.0);

  dec = adjustedEqCoord.dec;
}
float TelescopeModel::getDecCoord() { return dec; }
float TelescopeModel::getRACoord() { return ra; }

void TelescopeModel::setPositionRaDec(float ra, float dec) {
  raBasePos = ra;
  decBasePos = dec;
  altEncBaseValue = altEnc;
  azEncBaseValue = azEnc;
}

void TelescopeModel::setUTCYear(int y) { year = y; }
void TelescopeModel::setUTCMonth(int m) { month = m; }
void TelescopeModel::setUTCDay(int d) { day = d; }
void TelescopeModel::setUTCHour(int h) { hour = h; }
void TelescopeModel::setUTCMinute(int m) { min = m; }
void TelescopeModel::setUTCSecond(int s) { sec = s; }