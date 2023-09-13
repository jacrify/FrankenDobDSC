#ifndef TELESCOPE_MODEL_H_COORD_H
#define TELESCOPE_MODEL_H_COORD_H
#include "Ephemeris.h"
#include <cmath>

/**
 * Represents an alt / az. Largely a wrapper for Ephermeris HorizontalCoord,
 * but exposes add/substact operations.
 */
class HorizCoord {

  // HorizCoord() {
  //   alt=0;
  //   azi=0;
  // }

public:
  float alt;
  float azi;

  void normalise() {
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
  HorizCoord() {
    alt = 0.0;
    azi = 0.0;
  }

  HorizCoord(float altitude, float azimuth) {
    alt = altitude;
    azi = azimuth;
    // normalise();?
  }

  HorizCoord(HorizontalCoordinates ephHCoord) {
    alt = ephHCoord.alt;
    azi = ephHCoord.azi;
    // normalise();
  }
  
  /**
   * Convert to ephemeris
  */
  HorizontalCoordinates toHorizontalCoordinates() {
    HorizontalCoordinates out;
    out.alt=alt;
    out.azi=azi;
  }
  void
  setAlt(int degrees, int minutes, float seconds) {
    alt = Ephemeris::degreesMinutesSecondsToFloatingDegrees(degrees, minutes,
                                                            seconds);
    // normalise();
  }
  void setAzi(int degrees, int minutes, float seconds) {
    azi = Ephemeris::degreesMinutesSecondsToFloatingDegrees(degrees, minutes,
                                                            seconds);
    // normalise();
  }
  HorizCoord addOffset(float altOffset,float aziOffset) {
    HorizCoord out(alt, azi);
    out.alt += altOffset;
    out.azi += aziOffset;
    out.normalise();
    return out;
  }
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
    altAngle = altAz.alt;
    aziAngle = altAz.azi;

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