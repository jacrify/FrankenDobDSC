#ifndef EQPLATFORM
#define EQPLATFORM
#include "AsyncUDP.h"
#include "TimePoint.h"

class EQPlatform {
public:
  EQPlatform();

  void setupEQListener();
  void checkConnectionStatus();
  TimePoint calculateAdjustedTime();

  void park();
  void findHome();
  void moveAxis(double rate);
  void setTracking(int tracking);
  void pulseGuide(int direction, long duration);
  void zeroOffsetTime();

  double runtimeFromCenterSeconds;


  String eqPlatformIP;
  double timeToEnd;
  bool currentlyRunning;
  bool platformConnected;
  double pulseGuideRate;//degrees/sec
  double axisMoveRateMax;
  double axisMoveRateMin;
   double trackingRate;
   


private:
  AsyncUDP eqUDPOut;
  AsyncUDP eqUdpIn;
  void processPacket(AsyncUDPPacket &packet);
  void sendEQCommand(String command, double parm1, double parm2);
};

#endif
