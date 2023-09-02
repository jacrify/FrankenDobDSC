#include "telescopeModel.h"

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

EquatorialCoordinates TelescopeModel::calculateCurrentPosition() {
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

  float z = 360.0 * ((float)(azEnc - azEncBaseValue )) /
            (float)azEncoderStepsPerRevolution;
  altAzCoord.azi += z;
  // convert back to eq
  EquatorialCoordinates adjustedEqCoord =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
          altAzCoord, day, month, year, hour, min, sec);
  return adjustedEqCoord;
}
float TelescopeModel::getDecCoord() {
  // todo optimise to not recalc every call
  EquatorialCoordinates eqCoord = calculateCurrentPosition();
  return eqCoord.dec;
}
float TelescopeModel::getRACoord() {
  EquatorialCoordinates eqCoord = calculateCurrentPosition();
  return eqCoord.ra;
}

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