#include "AlpacaDiscovery.h"
#include "AsyncUDP.h"
#include "Logging.h"


#define IPBROADCASTPORT 50375 // set up alpaca discovery udp
AsyncUDP alpacaUdp;

/**
 * Set up alpaca udp discovery. 
*/
void setupAlpacaDiscovery(int alpacaHTTPPort) {
  if (alpacaUdp.listen(32227)) {
    log("Listening for alpaca discovery requests...");
    alpacaUdp.onPacket([alpacaHTTPPort](AsyncUDPPacket packet) {
      log("Received alpaca UDP Discovery packet ");
      if ((packet.length() >= 16) &&
          (strncmp("alpacadiscovery1", (char *)packet.data(), 16) == 0)) {
        log("Responding to alpaca UDP Discovery packet with my port number "
            "%d",
            alpacaHTTPPort);

        packet.printf("{\"AlpacaPort\": %d}", alpacaHTTPPort);
      }
    });
  }
}
