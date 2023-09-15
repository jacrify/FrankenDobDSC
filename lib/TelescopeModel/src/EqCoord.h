#ifndef TELESCOPE_MODEL_E_COORD_H
#define TELESCOPE_MODEL_E_COORD_H

#include "../../Ephemeris/src/Ephemeris.h"

#include <cmath>
class HorizCoord;

class EqCoord {
public:
  EquatorialCoordinates eq;
  EqCoord();
  EqCoord(float raInDegrees,float decInDegrees);
  EqCoord(EquatorialCoordinates e);

  EqCoord(HorizCoord h, unsigned long epochMillis);
  double getRAInHours();
  double getRAInDegrees();
  double getDecInDegrees();
  void setDecInDegrees(float dec);
  void setDecInDegrees(int degrees, int minutes, float seconds);
  void setRAInDegrees(int degrees, int minutes, float seconds);
  void setRAInDegrees(float ra);
  void setRAInHours(float ra);
  void setRAInHours(int hours, int minutes, float seconds);
  void addRAInDegrees(float raToAdd);
  
  
};
#endif
