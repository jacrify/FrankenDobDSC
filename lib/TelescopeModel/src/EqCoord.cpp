#include "EqCoord.h"
#include "HorizCoord.h"
#include <cmath>

EqCoord::EqCoord() {}
EqCoord::EqCoord(EquatorialCoordinates e) { eq = e; }

double EqCoord::calculateDistanceInDegrees(EqCoord delta) const {

  double ra1 = getRAInDegrees() * DEGREES_TO_RADIANS;
  double dec1 = getDecInDegrees() * DEGREES_TO_RADIANS;
  double ra2 = delta.getRAInDegrees() * DEGREES_TO_RADIANS;
  double dec2 = delta.getDecInDegrees() * DEGREES_TO_RADIANS;

  // Spherical law of cosines
  double distance_in_radians =
      acos(sin(dec1) * sin(dec2) + cos(dec1) * cos(dec2) * cos(ra2 - ra1));

  return distance_in_radians / DEGREES_TO_RADIANS; // Conversion back to degrees
}

/** 
 * Bug here! This give odd result when h.alt = exactly 90
 * */
EqCoord::EqCoord(HorizCoord h,TimePoint tp) {
  eq = Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
      h.toHorizontalCoordinates(), convertTimePointToEpochSeconds(tp));
}

EqCoord::EqCoord(float raInDegrees, float decInDegrees) {
  setRAInDegrees(raInDegrees);
  setDecInDegrees(decInDegrees);
}

double EqCoord::getRAInHours() const { return eq.ra; }
double EqCoord::getRAInDegrees() const { return eq.ra * 15; }
double EqCoord::getDecInDegrees() const { return eq.dec; }
void EqCoord::setDecInDegrees(float dec) { eq.dec = dec; }
void EqCoord::setRAInDegrees(float ra) { setRAInHours(ra / 15.0); }
void EqCoord::setRAInHours(float raHours) {
  // make positive
  raHours = fmod(fmod(raHours, 24) + 24, 24);
  eq.ra = raHours;
}

void EqCoord::setRAInHours(int hours, int minutes, float seconds) {
  eq.ra =
      Ephemeris::hoursMinutesSecondsToFloatingHours(hours, minutes, seconds);
}
void EqCoord::setDecInDegrees(int degrees, int minutes, float seconds) {
  eq.dec = Ephemeris::degreesMinutesSecondsToFloatingDegrees(degrees, minutes,
                                                             seconds);
}
void EqCoord::setRAInDegrees(int degrees, int minutes, float seconds) {
  setRAInDegrees(Ephemeris::degreesMinutesSecondsToFloatingDegrees(
      degrees, minutes, seconds));
}

EqCoord EqCoord::addRAInDegrees(float raToAdd) {
  float raHours = eq.ra + raToAdd / 15.0; //convert degrees to hours
  EqCoord out=EqCoord();
  out.setRAInHours(raHours);
  out.setDecInDegrees(eq.dec);
  return out;
   
}
