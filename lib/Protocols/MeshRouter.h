#pragma once
/**
 * Incoming Packets are Processed here
 **/

#include <RadioLib.h>
#include <mutex>
#include <Arduino.h>
#include "../../.pio/libdeps/heltec_wifi_lora_32_V2/LinkedList/LinkedList.h"
#include "config.h"
#include "RadioHandler.h"
#include "MacBase.h"
#include "DataLogger.h"
#include "Messages.h"

std::mutex *getSerialMutex();

// The Modem can only transmit 255 Byte large LoRa Packets.
#define LORA_MAX_PACKET_SIZE 255

/**
 * Receive States
 */
#define RECEIVE_STATE_IDLE 0
#define RECEIVE_STATE_PACKET_READY 1

#define SERIAL_PACKET_TYPE_STATUS_REQUEST 0
#define SERIAL_PACKET_TYPE_FLOOD_PACKET 1
#define SERIAL_PACKET_TYPE_STATUS_RESPONSE 2
#define SERIAL_PACKET_TYPE_CONFIGURATION_REQUEST 3
#define SERIAL_PACKET_TYPE_CONFIGURATION_RESPONSE 4
#define SERIAL_PACKET_TYPE_SNR_REQUEST 5
#define SERIAL_PACKET_TYPE_SNR_RESPONSE 6
#define SERIAL_PACKET_TYPE_RSSI_REQUEST 7
#define SERIAL_PACKET_TYPE_RSSI_RESPONSE 8
#define SERIAL_PACKET_TYPE_APPLY_MODEM_CONFIG 9

// This struct contains information on the serially transmitted payload packet.
#pragma pack(1)
typedef struct
{
    uint8_t messageType;
    uint16_t size;
    uint8_t source;
    int64_t hash; // Type of packet
    uint8_t *payload;
} SerialPayloadFloodPacket_t;
#pragma pack()

// Struct to determine the type and size of serial packet from header.
#pragma pack(1)
typedef struct
{
    uint8_t serialPacketType;
    uint16_t size;
} SerialPacketHeader_t;
#pragma pack()

// Struct containing new node ID to access configuration of a new serial request packet.
#pragma pack(1)
typedef struct
{
    uint8_t newNodeID;
} SerialPacketConfig_Request_t;
#pragma pack()

// Struct containing new node ID to access configuration of a new serial response packet.
#pragma pack(1)
typedef struct
{
    uint8_t newNodeID;
} SerialPacketConfig_Response_t;
#pragma pack()

// Model configuration parameters.
#pragma pack(1)
typedef struct ModemConfig
{
    uint8_t sf;
    uint8_t transmissionPower;
    uint32_t frequency;
    uint32_t bandwidth;
} ModemConfig_t;
#pragma pack()

/**
 * This struct represents an announced packet. This is created after receiving a FloodBroadcastHeaderPacket_t.
 * Diese Struct stellt ein Angekündigtes Paket dar. Dieses wird nach empfang eines FloodBroadcastHeaderPacket_t angelegt.
 */
#pragma pack(1)
typedef struct
{
    uint16_t id;
    uint16_t size;
    uint16_t received = 0;
    uint8_t lastFragment = 0;
    uint8_t source;
    bool corrupted = false;
    uint8_t checksum;
    uint8_t *payload;
} FragmentedPacketMeshRouter_t;
#pragma pack()

// This struct contains information of a queued packet.
typedef struct
{
    uint8_t *packetPointer;
    uint8_t packetSize;
    uint8_t source;
    uint16_t id;
    long waitTimeAfter;
    bool isHeader;
    bool isMission;
} QueuedPacket_t;

class MeshRouter : public MacBase
{
public:
    uint16_t ID_COUNTER = 0;
    uint16_t NODE_ID_COUNTERS[255];
    uint32_t receivedBytes = 0;
    uint8_t lastBroadcastSourceId = 0;

    String *debugString;
    int *displayQueueLength;

    unsigned long blockSendUntil = 0; // Blocks senders until previous message sent
    unsigned long preambleAdd = 0;    // Increases blocking time causing the other senders to wait

    LinkedList<QueuedPacket_t *> sendQueue; // List of queued packets to be sent

    void initProtocol() override;
    void handleWithFSM() override;
    void clearMacData() override;

    void handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const size_t packetSize, const int rssi, bool isMission) override;
    void OnFloodHeaderPacket(BroadcastRTSPacket_t *packet, int rssi);
    void OnFloodFragmentPacket(BroadcastFragmentPacket_t *packet);
    void OnNodeIdAnnouncePacket(NodeIdAnnounce_t *packet, int rssi);
    void OnPacketForHost(FragmentedPacketMeshRouter_t *packet);

    FragmentedPacketMeshRouter_t *getIncompletePacketById(uint16_t transmissionid, uint8_t source);
    void removeIncompletePacketById(uint16_t transmissionid, uint8_t source);

    void CreateBroadcastPacket(uint8_t *payload, uint8_t source, size_t size, uint16_t id, bool isMission);
    void AddToSendQueue(LinkedList<QueuedPacket_t *> *newPackets, bool isMission);
    void QueuePacket(LinkedList<QueuedPacket_t *> *listPointer, uint8_t *rawPacket, size_t size, uint8_t source, uint16_t id, bool isHeader, bool isMission, long waitTimeAfter = 0);
    void ProcessQueue();

    void handleUpperPacket(MessageToSend_t *serialPayloadFloodPacket) override;
    void announceNodeId(uint8_t respond);
    String getProtocolName() override;

protected:
    void onPreambleDetectedIR() override;
    void onCRCerrorIR() override;

private:
    LinkedList<FragmentedPacketMeshRouter_t *> incompletePacketList;
    // Time to send entire packet till last bit
    unsigned long packetTime = 0;
    // Returns total time passed since last announce
    unsigned long lastAnnounceTime = millis();

    // Measured Value for Sending Data
    double timePerByte = 0;
    double sendOverhead = 0;

    long predictPacketSendTime(uint8_t size);
    void SenderWait(unsigned long delay);
};