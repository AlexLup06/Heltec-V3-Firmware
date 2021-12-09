
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
     * Dieses Flood Paket wird von jeder Node, die es empfängt, propagiert
     **/
typedef struct
{
    uint8_t messageType;
    uint16_t id;
    uint8_t lastHop;
    uint8_t payload[LORA_MAX_PAKET_SIZE - 3];
} FloodBroadcastPaket;

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
    uint8_t payload[LORA_MAX_PAKET_SIZE - 6];
} UnicastMeshPaket;
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
} NodeIdAnnounce_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    uint8_t messageType;
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t lastHop;
} NodeIdDisagreement_t;
#pragma pack()

typedef struct
{
    uint8_t deviceMac[6];
    uint8_t nodeId;
    uint8_t hop;
} RoutingTable_t;

class MeshRouter
{
public:
    uint8_t NodeID;
    uint8_t macAdress[6];
    RoutingTable_t **routingTable;
    uint8_t totalRoutes = 0;

    void OnReceivePacket(uint8_t *rawPaket);
    void initNode();
    void SendRaw(uint8_t *rawPaket, uint8_t size);
    void OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket);
    void UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[6]);
    uint8_t findNextFreeNodeId(uint8_t nodeId, uint8_t deviceMac[6]);

    void announceNodeId();

    void debugPrintRoutingTable();
};