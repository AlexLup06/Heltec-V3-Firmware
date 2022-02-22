#pragma once
/**
 * Incoming Packets are Processed here
 **/

#include "../../.pio/libdeps/heltec_wifi_lora_32_V2/LinkedList/LinkedList.h"

#define LORA_MAX_PAKET_SIZE 255

/**
 * Paket Typen
 */
#define MESSAGE_TYPE_UNICAST_DATA_PAKET 0
#define MESSAGE_TYPE_FLOOD_BROADCAST_HEADER 1
#define MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT 2
#define MESSAGE_TYPE_NODE_ANNOUNCE 3
#define MESSAGE_TYPE_NODE_ID_DISAGREEMENT 4

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
    uint8_t messageType;
    uint16_t id;
    uint8_t fragment;
    uint8_t payload[LORA_MAX_PAKET_SIZE - 4];
} FloodBroadcastFragmentPaket_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t messageType;
    uint16_t id;
    uint8_t lastHop;
    uint8_t source;
    uint16_t size;
    uint8_t checksum;
} FloodBroadcastHeaderPaket_t;
#pragma pack()

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
     * 
    * 
    * dest: ziel node
    * source: quelle des pakets
    * lastHop: letzte Node, über die das Paket geleitet worden ist
    * id: generierte ID des pakets
    **/
#pragma pack(1)
typedef struct {
    uint8_t messageType;
    uint8_t nextHop;
    uint8_t lastHop;
    uint8_t dest;
    uint8_t source;
    uint8_t flags;
    uint16_t id;
    uint8_t payload[LORA_MAX_PAKET_SIZE - 8];
} UnicastMeshPaket_t;
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

#pragma pack(1)
typedef struct {
    uint8_t messageType;
    uint8_t deviceMac[6];
    uint8_t minNodeId;
    uint8_t lastHop;
} NodeIdDisagreement_t;
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
} QueuedPaket_t;

#define SERIAL_PAKET_TYPE_FLOOD_PAKET 1

#pragma pack(1)
typedef struct {
    uint8_t messageType;
    uint16_t size;
    uint8_t source;
    uint8_t *payload;
} SerialPayloadFloodPaket_t;
#pragma pack()

#pragma pack(1)
typedef struct {
    uint8_t serialPaketType;
    uint16_t size;
} SerialPaketHeader_t;
#pragma pack()

class MeshRouter {
public:
    uint8_t NodeID;
    uint8_t macAdress[6];
    RoutingTable_t **routingTable;
    uint8_t totalRoutes = 0;
    uint8_t OPERATING_MODE = 0;

    // Cad Detected
    // Modem hat den Anfang eines LoRa Signals empfangen
    bool cad = false;

    String *debugString;
    int *displayQueueLength;
    uint8_t receiveState = RECEIVE_STATE_IDLE;
    int readyPaketSize = 0;

    unsigned long blockSendUntil = 0;

    LinkedList<QueuedPaket_t> sendQueue;

    void OnReceivePacket(uint8_t messageType, uint8_t *rawPaket, uint8_t paketSize, int rssi);

    void OnFloodHeaderPaket(FloodBroadcastHeaderPaket_t *paket, int rssi);

    void OnFloodFragmentPaket(FloodBroadcastFragmentPaket_t *paket);

    void retransmitPaket(uint8_t *overrideBuffer, uint8_t overrideSize, uint8_t totalSize);

    FragmentedPaket_t *getIncompletePaketById(uint16_t transmissionid);

    void ProcessFloodSerialPaket(SerialPayloadFloodPaket_t *serialPayloadFloodPaket);

    void initNode();

    void SendRaw(uint8_t *rawPaket, uint8_t size);

    void QueuePaket(uint8_t *rawPaket, uint8_t size);

    void ProcessQueue();

    void UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[6], int rssi);

    void UpdateRSSI(uint8_t nodeId, int rssi);

    void UpdateHOP(uint8_t nodeId, uint8_t hop);

    void handle();

    void OnReceiveIR(int size);

    void _debugDumpSRAM();

    void OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket, int rssi);

    void OnUnicastPaket(UnicastMeshPaket_t *paket, uint8_t size, int rssi);

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

    unsigned long predictPacketSendTime(uint8_t size);

    void SenderWait(unsigned long delay);
};