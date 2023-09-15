#ifndef TELESCOPE_MODEL_H_COORD_H
#define TELESCOPE_MODEL_H_COORD_H
#include "../../Ephemeris/src/Ephemeris.h"
#include <cmath>


class EqCoord;
/**
 * Represents an alt / az. Largely a wrapper for Ephermeris HorizontalCoord,
 * but exposes add/substact operations.
 */
class HorizCoord {


public:
  float altInDegrees;
  float aziInDegrees;

  void normalise();
  HorizCoord() ;

  HorizCoord(float altitude, float azimuth);
  HorizCoord(EqCoord e,unsigned  long epochMillis) ;
  HorizCoord(HorizontalCoordinates ephHCoord) ;
  HorizontalCoordinates toHorizontalCoordinates();
  void setAlt(int degrees, int minutes, float seconds);
  void setAzi(int degrees, int minutes, float seconds);
  HorizCoord addOffset(float altOffset, float aziOffset);
};


/** This class represents the way taki measures angle coords.
 * This is: aziangle is anti clockwise when viewed from north pole
 * (so if viewer location is southern hemisphere azi direction changes)
 * and alt angle is negative if in southern hemisphere.
 *
 * Representing as seperate data structure here as it's doing my head
 * in trying to flip back and forth in code
 *
 */
class TakiHorizCoord {
public:
  float aziAngle;
  float altAngle;

  TakiHorizCoord(HorizCoord altAz, bool northernHemisphere) {
    altAngle = altAz.altInDegrees;
    aziAngle = altAz.aziInDegrees;

    if (northernHemisphere) {
      // when in northern hemisphere, alt is positive, and azi is counter
      // clockwise for model
      //  check for "over the top"
      aziAngle = 360.0 - aziAngle;
    } else {
      // when in southern hemisphere, alt is negatice, and azi is
      // clockwise for model
      altAngle = -altAngle;
    }

    if (altAngle > 90) {
      altAngle = 180 - altAngle; // Flip altAngle
      aziAngle += 180;
    }
    if (altAngle < -90) {
      altAngle = -180 - altAngle; // Flip altAngle
      aziAngle += 180;
    }
    // normalise to 0-360
    aziAngle = fmod(fmod(aziAngle, 360) + 360, 360);
  }
};

#endif