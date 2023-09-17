#include "EqCoord.h"
#include "HorizCoord.h"

EqCoord::EqCoord() {}
EqCoord::EqCoord(EquatorialCoordinates e) { eq = e; }

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

double EqCoord::getRAInHours() { return eq.ra; }
double EqCoord::getRAInDegrees() { return eq.ra * 15; }
double EqCoord::getDecInDegrees() { return eq.dec; }
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
  float raHours = eq.ra + raToAdd / 15.0; //convert minutes to hours
  EqCoord out=EqCoord();
  out.setRAInHours(raHours);
  out.setDecInDegrees(eq.dec);
  return out;
   
}
