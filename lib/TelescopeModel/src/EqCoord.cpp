#include "EqCoord.h"
#include "HorizCoord.h"

EqCoord::EqCoord() {}
EqCoord::EqCoord(EquatorialCoordinates e) { eq = e; }

EqCoord::EqCoord(HorizCoord h, unsigned long millis) {
  eq = Ephemeris::horizontalToEquatorialCoordinatesAtDateAndTime(
      h.toHorizontalCoordinates(), millis);
}
double EqCoord::getRAInHours() { return eq.ra; }
double EqCoord::getRAInDegrees() { return eq.ra * 15; }
double EqCoord::getDecInDegrees() { return eq.dec; }
double EqCoord::setDecInDegrees(float dec) { eq.dec = dec; }
double EqCoord::setRAInDegrees(float ra) { eq.ra = ra / 15.0; }