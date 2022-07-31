#pragma once
/**
 * Incoming Packets are Processed here
 **/

#include "../../.pio/libdeps/heltec_wifi_lora_32_V2/LinkedList/LinkedList.h"
#include <mutex>

std::mutex serial_mtx;
#define LORA_MAX_PAKET_SIZE 255

/**
 * Paket Typen
 */
#define MESSAGE_TYPE_NODE_ANNOUNCE 0x00
#define MESSAGE_TYPE_FLOOD_BROADCAST_HEADER 0x01
#define MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT 0x02
#define MESSAGE_TYPE_FLOOD_BROADCAST_ACK 0x03

/**
 * Operating Modes
 */
#define OPERATING_MODE_INIT 0
#define OPERATING_MODE_RECEIVE 1
#define OPERATING_MODE_SEND 2
#define OPERATING_MODE_UPDATE_IDLE 3

/**
 * Receive States
 */
#define RECEIVE_STATE_IDLE 0
#define RECEIVE_STATE_PAKET_READY 1

/**
 * Dieses Flood Paket wird von jeder Node, die es empfängt, propagiert
 **/
#pragma pack(1)
typedef struct {
    uint8_t messageType = MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT;
    uint16_t id = 0;
    uint8_t fragment = 0;
    uint8_t payload[LORA_MAX_PAKET_SIZE - 4];
} FloodBroadcastFragmentPaket_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t messageType = MESSAGE_TYPE_FLOOD_BROADCAST_HEADER;
    uint16_t id = 0;
    uint8_t lastHop = 0;
    uint8_t source = 0;
    uint16_t size = 0;
    uint8_t checksum = 0;
} FloodBroadcastHeaderPaket_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t messageType = MESSAGE_TYPE_FLOOD_BROADCAST_ACK;
    uint8_t source = 0; // Source of the initial FloodBroadcastHeaderPaket_t
    uint16_t id = 0;
    uint16_t totalTransmissionSize = 0;
} FloodBroadcastAck_t;
#pragma pack()

/**
 * Diese Struct stellt ein Angekündigtes Paket dar. Dieses wird nach empfang eines FloodBroadcastHeaderPaket_t angelegt.
 */
#pragma pack(1)
typedef struct {
    uint16_t id;
    uint16_t size;
    uint16_t received = 0;
    uint8_t lastFragment = 0;
    uint8_t source;
    uint8_t lastHop;
    bool corrupted = false;
    uint8_t checksum;
    uint8_t *payload;
} FragmentedPaket_t;
#pragma pack()

/**
    * Node ID Announce Paket
    * Dieses Paket wird zu beginn an alle Nodes verschickt.
    *  
    **/
#pragma pack(1)
typedef struct {
    uint8_t messageType;
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t lastHop;
    uint8_t respond;

} NodeIdAnnounce_t;
#pragma pack()

typedef struct {
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t hop;
    int rssi;
    unsigned long lastSeen = 0;
} RoutingTable_t;

typedef struct {
    uint8_t *paketPointer;
    uint8_t paketSize;
    uint8_t source;
    uint16_t id;
    long waitTimeAfter;
    int64_t hash = 0;
} QueuedPaket_t;

#define SERIAL_PACKET_TYPE_STATUS_REQUEST 0
#define SERIAL_PAKET_TYPE_FLOOD_PAKET 1
#define SERIAL_PACKET_TYPE_STATUS_RESPONSE 2
#define SERIAL_PACKET_TYPE_CONFIGURATION_REQUEST 3
#define SERIAL_PACKET_TYPE_CONFIGURATION_RESPONSE 4
#define SERIAL_PACKET_TYPE_SNR_REQUEST 5
#define SERIAL_PACKET_TYPE_SNR_RESPONSE 6
#define SERIAL_PACKET_TYPE_RSSI_REQUEST 7
#define SERIAL_PACKET_TYPE_RSSI_RESPONSE 8

#pragma pack(1)
typedef struct {
    uint8_t messageType;
    uint16_t size;
    uint8_t source;
    int64_t hash; // Typ des Pakets
    uint8_t *payload;
} SerialPayloadFloodPaket_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t serialPaketType;
    uint16_t size;
} SerialPaketHeader_t;
#pragma pack()


#pragma pack(1)
typedef struct {
    uint8_t nodeID;
} SerialPacketRSSI_Request_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t nodeRSSI;
} SerialPacketRSSI_Response_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    int8_t nodeSNR;
} SerialPacketSNR_Response_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t newNodeID;
} SerialPacketConfig_Request_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t newNodeID;
} SerialPacketConfig_Response_t;
#pragma pack()

class MeshRouter {
public:
    uint8_t NodeID;
    uint8_t macAdress[6];
    RoutingTable_t **routingTable;
    uint8_t totalRoutes = 0;
    uint8_t OPERATING_MODE = 0;

    uint16_t ID_COUNTER = 0;

    uint16_t NODE_ID_COUNTERS[255];

    uint32_t receivedBytes = 0;

    uint8_t lastBroadcastSourceId = 0;

    // Cad Detected
    // Modem hat den Anfang eines LoRa Signals empfangen
    bool cad = false;

    String *debugString;
    int *displayQueueLength;
    uint8_t receiveState = RECEIVE_STATE_IDLE;
    int readyPaketSize = 0;

    unsigned long blockSendUntil = 0;
    unsigned long preambleAdd = 0;

    LinkedList<QueuedPaket_t *> sendQueue;

    uint8_t setNodeID();
    int8_t getSNR();
    uint8_t getRSSI(uint8_t nodeID);

    void OnReceivePacket(uint8_t messageType, uint8_t *rawPaket, uint8_t paketSize, int rssi);

    void OnFloodHeaderPaket(FloodBroadcastHeaderPaket_t *paket, int rssi);

    void OnFloodBroadcastAck(FloodBroadcastAck_t *paket, int rssi);

    void OnFloodFragmentPaket(FloodBroadcastFragmentPaket_t *paket);

    FragmentedPaket_t *getIncompletePaketById(uint16_t transmissionid, uint8_t source);

    void removeIncompletePaketById(uint16_t transmissionid, uint8_t source);

    void ProcessFloodSerialPaket(SerialPayloadFloodPaket_t *serialPayloadFloodPaket);

    void CreateBroadcastPacket(uint8_t *payload, uint8_t source, uint16_t size, uint16_t id,int64_t hash);

    void AddToSendQueueReplaceSameHashedPackets(LinkedList<QueuedPaket_t *> *newPackts, int64_t hash, uint8_t messageSource);

    void initNode();

    void SendRaw(uint8_t *rawPaket, uint8_t size);

    void QueuePaket(LinkedList<QueuedPaket_t *> *listPointer, uint8_t *rawPaket, uint8_t size, uint8_t source, uint16_t id, long waitTimeAfter = 0, int64_t hash = 0);

    void ProcessQueue();

    void UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[6], int rssi);

    void UpdateRSSI(uint8_t nodeId, int rssi);

    void UpdateHOP(uint8_t nodeId, uint8_t hop);

    void handle();

    void OnReceiveIR(int size);

    void _debugDumpSRAM();

    void OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket, int rssi);

    uint8_t findNextFreeNodeId(uint8_t nodeId, uint8_t deviceMac[6]);

    void announceNodeId(uint8_t respond);

    void debugPrintRoutingTable();

    void OnPaketForHost(FragmentedPaket_t *paket);

    void measurePaketTime(uint8_t size, unsigned long time);

private:
    LinkedList<FragmentedPaket_t *> incompletePaketList;
    unsigned long packetTime = 0;
    unsigned long lastAnounceTime = millis();

    // Measured Value for Sending Data
    double timePerByte = 0;
    double sendOverhead = 0;

    double getSendTimeByPaketSizeInMS(int size);

    void readPaketFromSRAM(uint8_t *receiveBuffer, uint8_t start, uint8_t size);

    void writePaketToSRAM(uint8_t *paket, uint8_t start, uint8_t end);

    uint8_t findNextHopForDestination(uint8_t dest);

    long predictPacketSendTime(uint8_t size);

    void SenderWait(unsigned long delay);
};