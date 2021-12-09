#include <Arduino.h>
#include <WiFi.h>
#include <mesh/MeshRouter.h>
#include <lib/LoRa.h>

/**
 * Incoming Packets are Processed here
 **/

void MeshRouter::initNode()
{
    // 255 = keine Zugewiesene ID
    NodeID = 255;

    // Read MAC Adress
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    routingTable = nullptr;

    // Announce Local NodeID to the Network to
    MeshRouter::announceNodeId();
}

void MeshRouter::UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[])
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

    MeshRouter::debugPrintRoutingTable();
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

void MeshRouter::OnReceivePacket(uint8_t *rawPaket)
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
        MeshRouter::OnNodeIdAnnouncePaket(paket);
        break;
    }
}

void MeshRouter::OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket)
{
    char macHexStr[18] = {0};
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", paket->deviceMac[0], paket->deviceMac[1], paket->deviceMac[2], paket->deviceMac[3], paket->deviceMac[4], paket->deviceMac[5]);
    Serial.println("NodeIDDiscorveryFrom: " + String(macHexStr));

    if (NodeID != paket->nodeId)
    {
        MeshRouter::UpdateRoute(paket->nodeId, paket->lastHop, paket->deviceMac);
    }
    else
    {
        // ToDo: Route Dispute
    }

    //uint8_t freeNodeID = MeshRouter::findNextFreeNodeId();
}

/**
 * Announces the current Node ID to the Network
 */
void MeshRouter::announceNodeId()
{
    // Create Paket
    NodeIdAnnounce_t *paket = (NodeIdAnnounce_t *)malloc(sizeof(NodeIdAnnounce_t));

    paket->messageType = MESSAGE_TYPE_NODE_ANNOUNCE;
    paket->nodeId = macAdress[5]; // Pick last byte of WiFi Mac adress as starting NodeID
    memcpy(paket->deviceMac, macAdress, 6);

    // Send
    MeshRouter::SendRaw((uint8_t *)paket, sizeof(NodeIdAnnounce_t));
}

void MeshRouter::SendRaw(uint8_t *rawPaket, uint8_t size)
{
    LoRa.beginPacket();
    LoRa.write((uint8_t *)rawPaket, size);
    LoRa.endPacket();
}