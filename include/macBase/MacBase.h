#ifndef MACBASE_H
#define MACBASE_H

#include <Arduino.h>
#include "../helpers/IncompletePacketList.h"
#include "../helpers/CustomPacketQueue.h"
#include "../helpers/DataLogger.h"
#include "config.h"
#include "OperationalBase.h"
#include "RadioBase.h"

enum FSMState
{
    UNDEFINED,
    OFF,
    LISTENING,
    RECEIVING,
    TRANSMITTING,
};

// Abstract base class for all MAC protocols
class MacBase : public OperationalBase, public RadioBase
{
protected:
    // Common LoRa parameters and statistics
    uint8_t nodeId;
    uint16_t messageIdCounter;
    uint16_t missionIdCounter;

    DataLogger *dataLogger;

    QueuedPacket currentTransmission;

    FSMState fsmState = UNDEFINED;

    IncompletePacketList incompleteMissionPackets;
    IncompletePacketList incompleteNeighbourPackets;
    CustomPacketQueue customPacketQueue;

public:
    MacBase(uint8_t id) : nodeId(id), messageIdCounter(0), missionIdCounter(0) {}
    virtual ~MacBase() {}

    void finish();

    void sendPacket();
    void sendRaw();
    void finishCurrentTransmission();

    void handleUpperPacket();
    virtual void handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const uint8_t packetSize, const int rssi) = 0; // this is FSM
    void handleLowerPacket(const uint8_t messageType, const uint8_t *packet, const uint8_t packetSize, const int rssi);
    void onReceiveIR() override;

    void createNodeAnnouncePacket();
    void createBroadcastPacket(const uint8_t *payload, const uint8_t sourceId, const uint16_t messageSize, const bool withRTS, const bool isMission);
    void enqueuePacket(const uint8_t *packet, const uint8_t packetSize, const bool isHeader, const bool isNeighbour, const bool isNodeAnnounce);
    void encapsulate(const bool isMission, const uint8_t *packet);
    void decapsulate(const bool isMission, const uint8_t *packet);

    uint32_t getSendTimeByPacketSizeInUS(int packetBytes);
};

#endif // MACBASE_H
