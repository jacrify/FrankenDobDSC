#ifndef EQPLATFORM
#define EQPLATFORM
#include "AsyncUDP.h"
#include "TimePoint.h"

class EQPlatform {
public:
  EQPlatform();
  void sendEQCommand(String command, double parm);
  void setupEQListener();
  void checkConnectionStatus();
  TimePoint calculateAdjustedTime();
  

  double runtimeFromCenterSeconds;
  double platformResetOffsetSeconds;

      String eqPlatformIP;
  double timeToEnd;
  bool currentlyRunning;
  bool platformConnected;

private:
  AsyncUDP eqUDPOut;
  AsyncUDP eqUdpIn;
  void processPacket(AsyncUDPPacket &packet);
};

#endif
