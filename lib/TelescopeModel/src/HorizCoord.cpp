#include "HorizCoord.h"
#include "EqCoord.h"
void HorizCoord::normalise() {
  if (altInDegrees > 90) {
    altInDegrees = 180 - altInDegrees;
    aziInDegrees += 180;
  }
  if (altInDegrees < -90) {
    altInDegrees = 180 - altInDegrees;
    aziInDegrees += 180;
  }
  aziInDegrees = fmod(fmod(aziInDegrees, 360) + 360, 360);
}
HorizCoord::HorizCoord() {
  altInDegrees = 0.0;
  aziInDegrees = 0.0;
}

HorizCoord::HorizCoord(float altitude, float azimuth) {
  altInDegrees = altitude;
  aziInDegrees = azimuth;
  // normalise();?
}

HorizCoord::HorizCoord(EqCoord e, unsigned long time) {
  HorizontalCoordinates h =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(e.eq, time);
  altInDegrees = h.alt;
  aziInDegrees = h.azi;
  // normalise();?
}

HorizCoord::HorizCoord(HorizontalCoordinates ephHCoord) {
  altInDegrees = ephHCoord.alt;
  aziInDegrees = ephHCoord.azi;
  // normalise();
}
HorizontalCoordinates HorizCoord::toHorizontalCoordinates() {
  HorizontalCoordinates out;
  out.alt = altInDegrees;
  out.azi = aziInDegrees;
  return out;
}
void HorizCoord::setAlt(int degrees, int minutes, float seconds) {
  altInDegrees = Ephemeris::degreesMinutesSecondsToFloatingDegrees(
      degrees, minutes, seconds);
  // normalise();
}
void HorizCoord::setAzi(int degrees, int minutes, float seconds) {
  aziInDegrees = Ephemeris::degreesMinutesSecondsToFloatingDegrees(
      degrees, minutes, seconds);
  // normalise();
}
HorizCoord HorizCoord::addOffset(float altOffset, float aziOffset) {
  HorizCoord out(altInDegrees, aziInDegrees);
  out.altInDegrees += altOffset;
  out.aziInDegrees += aziOffset;
  out.normalise();
  return out;
}