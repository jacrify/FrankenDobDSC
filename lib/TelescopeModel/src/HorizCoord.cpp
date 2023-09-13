#include "HorizCoord.h"
#include "EqCoord.h"
void HorizCoord::normalise() {
  if (alt > 90) {
    alt = 180 - alt;
    azi += 180;
  }
  if (alt < -90) {
    alt = 180 - alt;
    azi += 180;
  }
  azi = fmod(fmod(azi, 360) + 360, 360);
}
HorizCoord::HorizCoord() {
  alt = 0.0;
  azi = 0.0;
}

HorizCoord::HorizCoord(float altitude, float azimuth) {
  alt = altitude;
  azi = azimuth;
  // normalise();?
}

HorizCoord::HorizCoord(EqCoord e, unsigned long time) {
  HorizontalCoordinates h =
      Ephemeris::equatorialToHorizontalCoordinatesAtDateAndTime(e.eq, time);
  alt = h.alt;
  azi = h.azi;
  // normalise();?
}

HorizCoord::HorizCoord(HorizontalCoordinates ephHCoord) {
  alt = ephHCoord.alt;
  azi = ephHCoord.azi;
  // normalise();
}
HorizontalCoordinates HorizCoord::toHorizontalCoordinates() {
  HorizontalCoordinates out;
  out.alt = alt;
  out.azi = azi;
  return out;
}
void HorizCoord::setAlt(int degrees, int minutes, float seconds) {
  alt = Ephemeris::degreesMinutesSecondsToFloatingDegrees(degrees, minutes,
                                                          seconds);
  // normalise();
}
void HorizCoord::setAzi(int degrees, int minutes, float seconds) {
  azi = Ephemeris::degreesMinutesSecondsToFloatingDegrees(degrees, minutes,
                                                          seconds);
  // normalise();
}
HorizCoord HorizCoord::addOffset(float altOffset, float aziOffset) {
  HorizCoord out(alt, azi);
  out.alt += altOffset;
  out.azi += aziOffset;
  out.normalise();
  return out;
}