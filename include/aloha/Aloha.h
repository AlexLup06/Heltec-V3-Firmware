#ifndef ALOHA_H
#define ALOHA_H

#include <Arduino.h>

class Aloha
{
public:
    void handleWithFSM();
    void handlePacket(
        const uint8_t messageType,
        const uint8_t *packet,
        const uint8_t packetSize);
};

#endif // ALOHA_H