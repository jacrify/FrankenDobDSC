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

  deltaSeconds = 0;

  // known eq position at sync
  raBasePos = 0;
  decBasePos = 0;
  // known encoder values at same
  altEncBaseValue = 0;
  azEncBaseValue = 0;
  currentRA = 0;
  currentDec = 0;
  currentAlt = 0;
  currentAz = 0;
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

// TODO implement
float TelescopeModel::getAltCoord() { return currentAlt; }
float TelescopeModel::getAzCoord() { return currentAz; }

void TelescopeModel::setDeltaSeconds(double delta) { deltaSeconds = delta; }

void TelescopeModel::calculateCurrentPosition() {
  // When client syncs the position, we store the ra/dec of that
  // position as well as the encoder values.
  // So we start by converting that base position into known
  // alt/az coords
  EquatorialCoordinates baseEqCoord;

  baseEqCoord.ra = raBasePos;
  baseEqCoord.dec = decBasePos;

  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          baseEqCoord, day, month, year, hour, min, sec);
  // add encoder values since then: we stored the encoder values
  // at position set
  float a = 360.0 * ((float)(altEnc - altEncBaseValue)) /
            (float)altEncoderStepsPerRevolution;
  altAzCoord.alt += a;

  float z = 360.0 * ((float)(azEnc - azEncBaseValue)) /
            (float)azEncoderStepsPerRevolution;
  altAzCoord.azi += z;

  currentAlt = altAzCoord.alt;
  currentAz = altAzCoord.azi;

  // convert back to eq to get new ra/dec
  EquatorialCoordinates adjustedEqCoord =
      Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
          altAzCoord, day, month, year, hour, min, sec);
  currentRA = fmod(adjustedEqCoord.ra, 24.0);

  currentDec = adjustedEqCoord.dec;
}
float TelescopeModel::getDecCoord() { return currentDec; }
float TelescopeModel::getRACoord() { return currentRA; }

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

void TelescopeModel::saveEncoderCalibrationPoint() {
  // use current ra/dec, without encoder values supplied, to get alt/az
  EquatorialCoordinates baseEqCoord;

  baseEqCoord.ra = raBasePos;
  baseEqCoord.dec = decBasePos;

  HorizontalCoordinates altAzCoord =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(
          baseEqCoord, day, month, year, hour, min, sec);

  altEncoderAlignValue2 = altEncoderAlignValue1;
  azEncoderAlignValue2 = azEncoderAlignValue1;
  azAlignValue2 = azAlignValue1;
  altAlignValue2 = altAlignValue1;

  azAlignValue1 = altAzCoord.azi;
  altAlignValue1 = altAzCoord.alt;
  altEncoderAlignValue1 = altEnc;
  azEncoderAlignValue1 = azEnc;
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

long TelescopeModel::calculateAzEncoderStepsPerRevolution() {
  long encoderMove = azEncoderAlignValue2 - azEncoderAlignValue1;
  float coordMove = azAlignValue2 - azAlignValue1;

  // Handle wraparound for azimuth
  if (coordMove < -180) {
    coordMove += 360;
  } else if (coordMove > 180) {
    coordMove -= 360;
  }

  float stepsPerDegree = (float)encoderMove / coordMove;
  return stepsPerDegree * 360.0; // Extrapolating for a full 360° rotation
}
long TelescopeModel::calculateAltEncoderStepsPerRevolution() {
  long encoderMove = altEncoderAlignValue2 - altEncoderAlignValue1;
  float coordMove = altAlignValue2 - altAlignValue1;

  // Handle potential wraparound for altitude
  // If your scope can move from slightly below the horizon to slightly past
  // zenith
  if (coordMove < -45) {
    coordMove += 90;
  } else if (coordMove > 45) {
    coordMove -= 90;
  }

  float stepsPerDegree = (float)encoderMove / coordMove;
  return stepsPerDegree *
         360.0; // Extrapolating for a hypothetical full 360° rotation
}
