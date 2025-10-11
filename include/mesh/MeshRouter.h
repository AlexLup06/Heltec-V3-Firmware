#pragma once
/**
 * Incoming Packets are Processed here
 **/

#include <RadioLib.h>
#include <mutex>
#include <Arduino.h>
#include "../../.pio/libdeps/heltec_wifi_lora_32_V2/LinkedList/LinkedList.h"
#include "config.h"
#include "radioHandler/RadioHandler.h"
#include "macBase/OperationalBase.h"
#include "../helpers/DataLogger.h"

std::mutex *getSerialMutex();

// The Modem can only transmit 255 Byte large LoRa Packets.
#define LORA_MAX_PACKET_SIZE 255

/**
 * Packet Types
 */
#define MESSAGE_TYPE_NODE_ANNOUNCE 0x00
#define MESSAGE_TYPE_FLOOD_BROADCAST_HEADER 0x01
#define MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT 0x02
#define MESSAGE_TYPE_FLOOD_BROADCAST_ACK 0x03

/**
 * Operating Modes
 */
#define OPERATING_MODE_INIT 0        // Node is initialized
#define OPERATING_MODE_RECEIVE 1     // Node is ready to receive messages
#define OPERATING_MODE_SEND 2        // Node is ready to send messages
#define OPERATING_MODE_UPDATE_IDLE 3 // Idle mode

/**
 * Receive States
 */
#define RECEIVE_STATE_IDLE 0
#define RECEIVE_STATE_PACKET_READY 1

/**
 * This flood packet is propagated by every node that receives it.
 * Dieses Flood Paket wird von jeder Node, die es empfängt, propagiert
 **/
// Structure for message of type Broadcast Fragment.
#pragma pack(1)
typedef struct
{
    uint8_t messageType = MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT;
    uint16_t id = 0;
    uint8_t fragment = 0;
    uint8_t payload[LORA_MAX_PACKET_SIZE - 4];
} FloodBroadcastFragmentPacket_t;
#pragma pack()

// Broadcast header packet
#pragma pack(1)
typedef struct
{
    uint8_t messageType = MESSAGE_TYPE_FLOOD_BROADCAST_HEADER;
    uint16_t id = 0;
    uint8_t lastHop = 0;
    uint8_t source = 0;
    uint16_t size = 0;
    uint8_t checksum = 0;
} FloodBroadcastHeaderPacket_t;
#pragma pack()

// Broadcast acknowledgement packet
#pragma pack(1)
typedef struct
{
    uint8_t messageType = MESSAGE_TYPE_FLOOD_BROADCAST_ACK;
    uint8_t source = 0; // Source of the initial FloodBroadcastHeaderPacket_t
    uint16_t id = 0;
    uint16_t totalTransmissionSize = 0;
} FloodBroadcastAck_t;
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
    uint8_t lastHop;
    bool corrupted = false;
    uint8_t checksum;
    uint8_t *payload;
} FragmentedPacket_t;
#pragma pack()

/**
 * Node ID Announce Packet
 * This packet is initially sent to all nodes.
 * Dieses Paket wird zu beginn an alle Nodes verschickt.
 *
 **/
#pragma pack(1)
typedef struct
{
    uint8_t messageType;
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t lastHop;
    uint8_t respond;

} NodeIdAnnounce_t;
#pragma pack()

// Routing table packet. This is created after receiving a NodeIdAnnounce_t. It contains the node ID, RSSI, Hop and last seen concerning the new/updated route.
typedef struct
{
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t hop;
    int rssi;
    unsigned long lastSeen = 0;
} RoutingTable_t;

// This struct contains information of a queued packet.
typedef struct
{
    uint8_t *packetPointer;
    uint8_t packetSize;
    uint8_t source;
    uint16_t id;
    long waitTimeAfter;
    int64_t hash = 0;
} QueuedPacket_t;

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

class MeshRouter : public OperationalBase
{
public:
    uint8_t NodeID;
    uint8_t macAdress[6];
    uint8_t OPERATING_MODE = 0; // initialize node by default.
    uint16_t ID_COUNTER = 0;
    uint16_t NODE_ID_COUNTERS[255];
    uint32_t receivedBytes = 0;
    uint8_t lastBroadcastSourceId = 0;

    String *debugString;
    int *displayQueueLength;
    uint8_t receiveState = RECEIVE_STATE_IDLE; // Node is in standby default receive state
    bool cad = false;                          // bool variable to determine if the modem has received the beginning of a LoRa signal

    DataLogger *dataLogger;

    unsigned long blockSendUntil = 0; // Blocks senders until previous message sent
    unsigned long preambleAdd = 0;    // Increases blocking time causing the other senders to wait

    LinkedList<QueuedPacket_t *> sendQueue; // List of queued packets to be sent

    uint8_t setNodeID(uint8_t newNodeID);

    void applyModemConfig(uint8_t spreading_factor, uint8_t transmission_power, uint32_t frequency, uint32_t bandwidth);

    void init() override;
    void handle() override;
    void finish() override;

    void OnReceivePacket(uint8_t messageType, uint8_t *rawPacket, uint8_t packetSize, int rssi);
    void OnFloodHeaderPacket(FloodBroadcastHeaderPacket_t *packet, int rssi);
    void OnFloodBroadcastAck(FloodBroadcastAck_t *packet, int rssi);
    void OnFloodFragmentPacket(FloodBroadcastFragmentPacket_t *packet);
    void OnNodeIdAnnouncePacket(NodeIdAnnounce_t *packet, int rssi);
    void OnPacketForHost(FragmentedPacket_t *packet);

    FragmentedPacket_t *getIncompletePacketById(uint16_t transmissionid, uint8_t source);
    void removeIncompletePacketById(uint16_t transmissionid, uint8_t source);

    void CreateBroadcastPacket(uint8_t *payload, uint8_t source, uint16_t size, uint16_t id, int64_t hash);
    void AddToSendQueueReplaceSameHashedPackets(LinkedList<QueuedPacket_t *> *newPackts, int64_t hash, uint8_t messageSource);
    void QueuePacket(LinkedList<QueuedPacket_t *> *listPointer, uint8_t *rawPacket, uint8_t size, uint8_t source, uint16_t id, long waitTimeAfter = 0, int64_t hash = 0);
    void ProcessQueue();

    void ProcessFloodSerialPacket(SerialPayloadFloodPacket_t *serialPayloadFloodPacket);
    void announceNodeId(uint8_t respond);
    void SendRaw(uint8_t *rawPacket, uint8_t size);
    String getProtocolName() override;

protected:
    void onReceiveIR() override;
    void onPreambleDetectedIR() override;
    void onCRCerrorIR() override;

private:
    LinkedList<FragmentedPacket_t *> incompletePacketList;
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