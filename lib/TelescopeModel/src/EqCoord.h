#ifndef TELESCOPE_MODEL_E_COORD_H
#define TELESCOPE_MODEL_E_COORD_H
#include "Ephemeris.h"

#include <cmath>
class HorizCoord;

class EqCoord {
public:
  EquatorialCoordinates eq;
  EqCoord();
  EqCoord(EquatorialCoordinates e);

  EqCoord(HorizCoord h, unsigned long millis);
  double getRAInHours() ;
  double getRAInDegrees() ;
  double getDecInDegrees() ;
  void setDecInDegrees(float dec);
  void setRAInDegrees(float ra) ;
};
#endif
