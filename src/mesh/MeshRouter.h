#pragma once
/**
 * Incoming Packets are Processed here
 **/

#define LORA_MAX_PAKET_SIZE 255

/**
 * Paket Typen
 */
#define MESSAGE_TYPE_UNICAST_DATA_PAKET 0
#define MESSAGE_TYPE_FLOOD_BROADCAST 1
#define MESSAGE_TYPE_NODE_ANNOUNCE 2
#define MESSAGE_TYPE_NODE_ID_DISAGREEMENT 2

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
typedef struct
{
    uint8_t messageType;
    uint16_t id;
    uint8_t lastHop;
    uint8_t payload[LORA_MAX_PAKET_SIZE - 3];
} FloodBroadcastPaket_t;

/**
     * 
    * 
    * dest: ziel node
    * source: quelle des pakets
    * lastHop: letzte Node, über die das Paket geleitet worden ist
    * id: generierte ID des pakets
    **/
#pragma pack(1)
typedef struct
{
    uint8_t messageType;
    uint16_t id;
    uint8_t dest;
    uint8_t source;
    uint8_t lastHop;
    uint8_t flags;
    uint8_t payload[LORA_MAX_PAKET_SIZE - 7];
} UnicastMeshPaket_t;
#pragma pack()

/**
    * Node ID Announce Paket
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

#pragma pack(1)
typedef struct
{
    uint8_t messageType;
    uint8_t deviceMac[6];
    uint8_t minNodeId;
    uint8_t lastHop;
} NodeIdDisagreement_t;
#pragma pack()

typedef struct
{
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t hop;
    uint8_t rssi;
} RoutingTable_t;

typedef struct
{
    uint8_t *paketPointer;
    uint8_t paketSize;
} QueuedPaket_t;

class MeshRouter
{
public:
    uint8_t NodeID;
    uint8_t macAdress[6];
    RoutingTable_t **routingTable;
    uint8_t totalRoutes = 0;
    uint8_t OPERATING_MODE = 0;

    uint8_t receiveState = RECEIVE_STATE_IDLE;
    int readyPaketSize = 0;

    QueuedPaket_t PAKET_QUEUE[32];
    //Vector<QueuedPaket_t> vector(QueuedPaket_t[], 0);

    uint8_t PAKET_QUEUE_SIZE = 0;

    uint32_t blockSendUntil = 0;

    void OnReceivePacket(uint8_t *rawPaket, int16_t rssi);
    void initNode();
    void SendRaw(uint8_t *rawPaket, uint8_t size);
    void QueuePaket(uint8_t *rawPaket, uint8_t size);
    void ProcessQueue();
    void OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket, int16_t rssi);
    void UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[6], int16_t rssi);
    void handle();
    void OnReceiveIR(int size);

    uint8_t findNextFreeNodeId(uint8_t nodeId, uint8_t deviceMac[6]);

    void announceNodeId(uint8_t respond);

    void debugPrintRoutingTable();
};