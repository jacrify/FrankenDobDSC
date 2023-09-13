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
void EqCoord::setDecInDegrees(float dec) { eq.dec = dec; }
void EqCoord::setRAInDegrees(float ra) { eq.ra = ra / 15.0; }
void EqCoord::setRAInHours(float raHours) { eq.ra = raHours; }