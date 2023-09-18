#ifndef ENCODERS_H
#define ENCODERS_H

void setupEncoders();
void loopEncoders();
long volatile getEncoderAz();
long volatile getEncoderAl();
volatile long getEncoderSkipsAz();
volatile long getEncoderSkipsAl()
#endif
