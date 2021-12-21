#include <Arduino.h>
#include <WiFi.h>
#include <mesh/MeshRouter.h>
#include <lib/LoRa.h>
/**
 * Incoming Packets are Processed here
 **/

void MeshRouter::initNode()
{
    // Read MAC Adress
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    // 255 = keine Zugewiesene ID
    NodeID = macAdress[5];

    routingTable = nullptr;

    // Announce Local NodeID to the Network to
    MeshRouter::announceNodeId(1);
}

void MeshRouter::UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[], int16_t rssi)
{
    // Malloc pointer array, when not allocated yet
    if (routingTable == nullptr)
    {
        routingTable = (RoutingTable_t **)malloc(sizeof(RoutingTable_t *) * 255);
        totalRoutes = 0;
    }

    Serial.println("nodeId: " + String(nodeId));
    // Suche Node in vorhandenem RoutingTable
    uint8_t foundIndex = 255;
    for (int i = 0; i < totalRoutes; i++)
    {
        if (memcmp(routingTable[i]->deviceMac, deviceMac, 6) == 0)
        {
            foundIndex = i;
        }
    }

    // Node noch nicht im Routing Table
    if (foundIndex == 255)
    {
        totalRoutes++;

        foundIndex = totalRoutes - 1;
        // Alloziiere eigentliche Datenstruktur
        routingTable[foundIndex] = (RoutingTable_t *)malloc(sizeof(RoutingTable_t));
    }

    memcpy(routingTable[foundIndex]->deviceMac, deviceMac, 6);
    routingTable[foundIndex]->nodeId = nodeId;
    routingTable[foundIndex]->hop = hop;
    routingTable[foundIndex]->rssi = rssi;
}

uint8_t MeshRouter::findNextFreeNodeId(uint8_t currentLeastKnownFreeNodeId, uint8_t deviceMac[6])
{
    int nextFreeNodeId = currentLeastKnownFreeNodeId;
    for (int i = 0; i < totalRoutes; i++)
    {
        if (memcmp(routingTable[i]->deviceMac, deviceMac, 6) == 0)
        {
            // Node is already Known and in Routing Table
            return routingTable[i]->nodeId;
        }
        if (routingTable[i]->nodeId >= nextFreeNodeId)
        {
            nextFreeNodeId = routingTable[i]->nodeId + 1;
            if (nextFreeNodeId == 255)
            {
                return 255;
            }
        }
    }
    return nextFreeNodeId;
}

void MeshRouter::debugPrintRoutingTable()
{
    Serial.println("--- ROUTING_TABLE ---");
    for (int i = 0; i < totalRoutes; i++)
    {
        char macHexStr[18] = {0};
        sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", routingTable[i]->deviceMac[0], routingTable[i]->deviceMac[1], routingTable[i]->deviceMac[2], routingTable[i]->deviceMac[3], routingTable[i]->deviceMac[4], routingTable[i]->deviceMac[5]);
        Serial.println("" + String(routingTable[i]->nodeId) + " - " + String(routingTable[i]->hop) + " - " + String(macHexStr));
    }
    Serial.println("---------------------");
}

/**
 * Main Loop
 * 
 */
void MeshRouter::handle()
{
    switch (OPERATING_MODE)
    {
    case OPERATING_MODE_INIT:
        LoRa.receive();
        OPERATING_MODE = OPERATING_MODE_RECEIVE;
        break;
    case OPERATING_MODE_SEND:
        // ToDo: Process Paket Queue

        break;
    case OPERATING_MODE_RECEIVE:
        if (receiveState == RECEIVE_STATE_PAKET_READY)
        {
            uint8_t *receiveBuffer = (uint8_t *)malloc(readyPaketSize);

            LoRa.readBytes(receiveBuffer, 255);

            int16_t rssi = LoRa.packetRssi();
            OnReceivePacket(receiveBuffer, rssi);
            receiveState = RECEIVE_STATE_IDLE;

            LoRa.receive();
        }

        if (PAKET_QUEUE_SIZE > 0 && blockSendUntil < millis())
        {
            ProcessQueue();
        }

        break;
    case OPERATING_MODE_UPDATE_IDLE:
        break;
    }
}

void MeshRouter::OnReceiveIR(int size)
{
    receiveState = RECEIVE_STATE_PAKET_READY;
    readyPaketSize = size;
}

void MeshRouter::OnReceivePacket(uint8_t *rawPaket, int16_t rssi)
{
    // First byte always MessageType
    uint8_t messageType = rawPaket[0];

    Serial.println("MessageType: " + String(messageType));
    switch (messageType)
    {
    case MESSAGE_TYPE_UNICAST_DATA_PAKET:
        // ToDo
        break;
    case MESSAGE_TYPE_FLOOD_BROADCAST:
        // ToDo
        break;
    case MESSAGE_TYPE_NODE_ANNOUNCE:
        NodeIdAnnounce_t *paket = (NodeIdAnnounce_t *)rawPaket;
        MeshRouter::OnNodeIdAnnouncePaket(paket, rssi);
        break;
    }

    // Paket Processed - Free Ram
    free(rawPaket);
}

void MeshRouter::OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket, int16_t rssi)
{
    char macHexStr[18] = {0};
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", paket->deviceMac[0], paket->deviceMac[1], paket->deviceMac[2], paket->deviceMac[3], paket->deviceMac[4], paket->deviceMac[5]);
    Serial.println("NodeIDDiscorveryFrom: " + String(macHexStr));

    if (NodeID != paket->nodeId)
    {
        MeshRouter::UpdateRoute(paket->nodeId, paket->lastHop, paket->deviceMac, rssi);

        if (paket->respond == 1)
        {
            // Unknown node! Delay and send own Pakage
            delay(NodeID * 15);
            MeshRouter::announceNodeId(0);
        }
    }
    else
    {
        // ToDo: Route Dispute
    }
}

/**
 * Announces the current Node ID to the Network
 */
void MeshRouter::announceNodeId(uint8_t respond)
{
    // Create Paket
    NodeIdAnnounce_t *paket = (NodeIdAnnounce_t *)malloc(sizeof(NodeIdAnnounce_t));

    paket->messageType = MESSAGE_TYPE_NODE_ANNOUNCE;
    paket->nodeId = NodeID;
    paket->lastHop = NodeID;
    paket->respond = respond;
    memcpy(paket->deviceMac, macAdress, 6);

    // Send
    MeshRouter::SendRaw((uint8_t *)paket, sizeof(NodeIdAnnounce_t));

    free(paket);
}

/**
 * Directly sends Paket with Hardware
 * 
 * @param rawPaket 
 * @param size 
 */
void MeshRouter::SendRaw(uint8_t *rawPaket, uint8_t size)
{
    LoRa.beginPacket();
    LoRa.write((uint8_t *)rawPaket, size);
    LoRa.endPacket();
    LoRa.receive();
}

void MeshRouter::QueuePaket(uint8_t *rawPaket, uint8_t size)
{
    PAKET_QUEUE[PAKET_QUEUE_SIZE].paketPointer = rawPaket;
    PAKET_QUEUE[PAKET_QUEUE_SIZE].paketSize = size;
    PAKET_QUEUE_SIZE++;
}

void MeshRouter::ProcessQueue()
{
}