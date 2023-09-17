#ifndef TELESCOPE_MODEL_E_COORD_H
#define TELESCOPE_MODEL_E_COORD_H

#include "../../Ephemeris/src/Ephemeris.h"

#include "TimePoint.h"
#include <cmath>
class HorizCoord;

class EqCoord {
public:
  EquatorialCoordinates eq;
  EqCoord();
  EqCoord(float raInDegrees, float decInDegrees);
  EqCoord(EquatorialCoordinates e);

  EqCoord(HorizCoord h, TimePoint tp);
  double calculateDistanceInDegrees(EqCoord delta) const;
  double getRAInHours() const;
  double getRAInDegrees() const;
  double getDecInDegrees() const;
  void setDecInDegrees(float dec);
  void setDecInDegrees(int degrees, int minutes, float seconds);
  void setRAInDegrees(int degrees, int minutes, float seconds);
  void setRAInDegrees(float ra);
  void setRAInHours(float ra);
  void setRAInHours(int hours, int minutes, float seconds);
  EqCoord addRAInDegrees(float raToAdd);

private:
  static constexpr double DEG_TO_RAD = M_PI / 180.0;
};
#endif
