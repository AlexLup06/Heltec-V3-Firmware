#include <Arduino.h>
#include <mesh/MeshRouter.h>
#include <lib/LoRa.h>

/**
 * Incoming Packets are Processed here
 **/

void MeshRouter::initNode() {
    // Read MAC Adress
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    // 255 = keine Zugewiesene ID
    NodeID = macAdress[5];
    routingTable = nullptr;

    // Announce Local NodeID to the Network to
    MeshRouter::announceNodeId(1);
}

void MeshRouter::UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[], int16_t rssi) {
    // Malloc pointer array, when not allocated yet
    if (routingTable == nullptr) {
        routingTable = (RoutingTable_t **) malloc(sizeof(RoutingTable_t *) * 255);
        totalRoutes = 0;
    }

#ifdef DEBUG_LORA_SERIAL
    Serial.println("nodeId: " + String(nodeId));
#endif
    // Suche Node in vorhandenem RoutingTable
    uint8_t foundIndex = 255;
    for (int i = 0; i < totalRoutes; i++) {
        if (memcmp(routingTable[i]->deviceMac, deviceMac, 6) == 0) {
            foundIndex = i;
        }
    }

    // Node noch nicht im Routing Table
    if (foundIndex == 255) {
        totalRoutes++;

        foundIndex = totalRoutes - 1;
        // Alloziiere eigentliche Datenstruktur
        routingTable[foundIndex] = (RoutingTable_t *) malloc(sizeof(RoutingTable_t));
    }

    memcpy(routingTable[foundIndex]->deviceMac, deviceMac, 6);
    routingTable[foundIndex]->nodeId = nodeId;
    routingTable[foundIndex]->hop = hop;
    routingTable[foundIndex]->rssi = rssi;
}

uint8_t MeshRouter::findNextFreeNodeId(uint8_t currentLeastKnownFreeNodeId, uint8_t deviceMac[6]) {
    int nextFreeNodeId = currentLeastKnownFreeNodeId;
    for (int i = 0; i < totalRoutes; i++) {
        if (memcmp(routingTable[i]->deviceMac, deviceMac, 6) == 0) {
            // Node is already Known and in Routing Table
            return routingTable[i]->nodeId;
        }
        if (routingTable[i]->nodeId >= nextFreeNodeId) {
            nextFreeNodeId = routingTable[i]->nodeId + 1;
            if (nextFreeNodeId == 255) {
                return 255;
            }
        }
    }
    return nextFreeNodeId;
}

void MeshRouter::debugPrintRoutingTable() {
    Serial.println("--- ROUTING_TABLE ---");
    for (int i = 0; i < totalRoutes; i++) {
        char macHexStr[18] = {0};
        sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", routingTable[i]->deviceMac[0],
                routingTable[i]->deviceMac[1], routingTable[i]->deviceMac[2], routingTable[i]->deviceMac[3],
                routingTable[i]->deviceMac[4], routingTable[i]->deviceMac[5]);
        Serial.println("" + String(routingTable[i]->nodeId) + " - " + String(routingTable[i]->hop) + " - " +
                       String(macHexStr));
    }
    Serial.println("---------------------");
}

/**
 * Dumps the whole SRAM of the SX127X Chip
 */
void MeshRouter::_debugDumpSRAM() {
    auto *ram = (uint8_t *) malloc(255);

    // Set SRAM Pointer to 0x00
    LoRa.writeRegister(REG_FIFO_ADDR_PTR, 0);

    for (int i = 0; i < 255; i++) {
        ram[i] = LoRa.readRegister(REG_FIFO);
    }

    char macHexStr[3];
    char parsedBuffer[160];
    for (int i = 0; i < 160; i++) {
        parsedBuffer[i] = ' ';
    }
    for (int i = 0; i < 50; i++) {
        sprintf(macHexStr, "%02X", ram[i]);

        parsedBuffer[i * 3] = macHexStr[0];
        parsedBuffer[i * 3 + 1] = macHexStr[1];
        parsedBuffer[i * 3 + 2] = ' ';
    }
    uint8_t address = LoRa.readRegister(REG_FIFO_RX_CURRENT_ADDR);

    Serial.println("SRAM P(" + String(address) + "): " + String(parsedBuffer));
}

/**
 * Main Loop
 * 
 */
void MeshRouter::handle() {
    switch (OPERATING_MODE) {
        case OPERATING_MODE_INIT:
            LoRa.receive();
            OPERATING_MODE = OPERATING_MODE_RECEIVE;
            break;
        case OPERATING_MODE_SEND:
            // ToDo: Process Paket Queue

            break;
        case OPERATING_MODE_RECEIVE:
            if (receiveState == RECEIVE_STATE_PAKET_READY) {
                // We need to set the Module to idle, to prevent the module from overwriting the SRAM with a new Paket and to be able to modify the Modem SRAM Pointer
                LoRa.idle();

                auto *receiveBuffer = (uint8_t *) malloc(readyPaketSize);
                int16_t rssi = LoRa.packetRssi();

                // set FIFO address to current RX address
                LoRa.writeRegister(REG_FIFO_ADDR_PTR, LoRa.readRegister(REG_FIFO_RX_CURRENT_ADDR));

                // We only need the Paket Type at this point
                receiveBuffer[0] = LoRa.readRegister(REG_FIFO);

                OnReceivePacket(receiveBuffer[0], receiveBuffer, readyPaketSize, rssi);

                receiveState = RECEIVE_STATE_IDLE;

                // Set Modem SRAM Pointer to 0x00
                LoRa.writeRegister(REG_FIFO_RX_CURRENT_ADDR, 0);
                LoRa.receive();
            }


            ProcessQueue();


            break;

        case OPERATING_MODE_UPDATE_IDLE:
            break;
    }
}

void MeshRouter::OnReceiveIR(int size) {
    receiveState = RECEIVE_STATE_PAKET_READY;
    readyPaketSize = size;
}

/**
 * Reads the Rest of the Paket into the given receive Buffer
 *
 * @param receiveBuffer
 * @param size
 */
void MeshRouter::readPaketFromSRAM(uint8_t *receiveBuffer, uint8_t start, uint8_t end) {
    for (int i = start; i < end; i++) {
        receiveBuffer[i] = LoRa.readRegister(REG_FIFO);
    }
}

void MeshRouter::writePaketToSRAM(uint8_t *paket, uint8_t start, uint8_t end) {
    LoRa.writeRegister(REG_FIFO_ADDR_PTR, start);
    for (int i = start; i < end; i++) {
        LoRa.writeRegister(REG_FIFO, paket[i]);
    }
}

void MeshRouter::OnReceivePacket(uint8_t messageType, uint8_t *rawPaket, uint8_t paketSize, int16_t rssi) {
#ifdef DEBUG_LORA_SERIAL
    Serial.println("MessageType: " + String(messageType));
#endif
    switch (messageType) {
        case MESSAGE_TYPE_UNICAST_DATA_PAKET:
            MeshRouter::OnUnicastPaket((UnicastMeshPaket_t *) rawPaket, paketSize, rssi);
            break;
        case MESSAGE_TYPE_FLOOD_BROADCAST_HEADER:
            readPaketFromSRAM(rawPaket, 1, paketSize);
            MeshRouter::OnFloodHeaderPaket((FloodBroadcastHeaderPaket_t *) rawPaket);
            break;
        case MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT:
            readPaketFromSRAM(rawPaket, 1, paketSize);
            MeshRouter::OnFloodFragmentPaket((FloodBroadcastFragmentPaket_t *) rawPaket);
            break;
        case MESSAGE_TYPE_NODE_ANNOUNCE:
            readPaketFromSRAM(rawPaket, 1, paketSize);
            MeshRouter::OnNodeIdAnnouncePaket((NodeIdAnnounce_t *) rawPaket, rssi);
            break;
    }

    // Paket Processed - Free Ram
    free(rawPaket);
}

void MeshRouter::OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket, int16_t rssi) {
#ifdef DEBUG_LORA_SERIAL
    char macHexStr[18];
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", paket->deviceMac[0], paket->deviceMac[1], paket->deviceMac[2],
            paket->deviceMac[3], paket->deviceMac[4], paket->deviceMac[5]);
    Serial.println("NodeIDDiscorveryFrom: " + String(macHexStr));
#endif
    if (NodeID != paket->nodeId) {
        MeshRouter::UpdateRoute(paket->nodeId, paket->lastHop, paket->deviceMac, rssi);

        if (paket->respond == 1) {
            // Unknown node! Block Sending for a time window, to allow other Nodes to respond.
            MeshRouter::SenderWait(NodeID * 15);
            MeshRouter::announceNodeId(0);
        }
    } else {
        // ToDo: Route Dispute
    }
}

void MeshRouter::OnUnicastPaket(UnicastMeshPaket_t *paket, uint8_t size, int16_t rssi) {
    // Read lastHop, nextHop and dest from SRAM
    readPaketFromSRAM((uint8_t *) paket, 1, 4);
    if (paket->nextHop == NodeID) {
        if (paket->dest == NodeID) {
            // We are the destination, read whole paket from Lora Module
            readPaketFromSRAM((uint8_t *) paket, 2, size);
        } else {
            // We are just a hop on the Route
            // Adjust data in SRAM and resend
            paket->nextHop = findNextHopForDestination(paket->dest);
            paket->lastHop = NodeID;

            writePaketToSRAM((uint8_t *) paket, 1, 3);

            LoRa.writeRegister(REG_FIFO_ADDR_PTR, 0);
            LoRa.writeRegister(REG_PAYLOAD_LENGTH, size);

            LoRa.endPacket();
        }
    }
}

/**
 * Announces the current Node ID to the Network
 */
void MeshRouter::announceNodeId(uint8_t respond) {
    // Create Paket
    auto *paket = (NodeIdAnnounce_t *) malloc(sizeof(NodeIdAnnounce_t));

    paket->messageType = MESSAGE_TYPE_NODE_ANNOUNCE;
    paket->nodeId = NodeID;
    paket->lastHop = NodeID;
    paket->respond = respond;
    memcpy(paket->deviceMac, macAdress, 6);

    char macHexStr[18] = {0};
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", paket->deviceMac[0], paket->deviceMac[1], paket->deviceMac[2],
            paket->deviceMac[3], paket->deviceMac[4], paket->deviceMac[5]);
#ifdef DEBUG_LORA_SERIAL
    Serial.println("SendNodeDiscovery: " + String(macHexStr));
#endif

    QueuePaket((uint8_t *) paket, sizeof(NodeIdAnnounce_t));
}

/**
 * Directly sends Paket with Hardware, you should never use this directly, because this will not wait for the Receive window of other Devices
 * 
 * @param rawPaket 
 * @param size 
 */
void MeshRouter::SendRaw(uint8_t *rawPaket, uint8_t size) {
    LoRa.beginPacket();
    LoRa.write(rawPaket, size);
    LoRa.endPacket();

    LoRa.receive();
}

/**
 * Delays the Next paket in the Queue for a specific time
 * @param delay
 */
void MeshRouter::SenderWait(unsigned long waitTime) {
    MeshRouter::blockSendUntil = millis() + waitTime;
}

void MeshRouter::QueuePaket(uint8_t *rawPaket, uint8_t size) {
    QueuedPaket_t paket;
    paket.paketPointer = rawPaket;
    paket.paketSize = size;
    sendQueue.add(paket);
}

void MeshRouter::ProcessQueue() {
    if (sendQueue.size() == 0 || blockSendUntil > millis()) {
        return;
    }
    QueuedPaket_t paketQueueEntry = sendQueue.shift();
    MeshRouter::SendRaw(paketQueueEntry.paketPointer,
                        paketQueueEntry.paketSize);
    free(paketQueueEntry.paketPointer);
}

uint8_t MeshRouter::findNextHopForDestination(uint8_t dest) {
    if (routingTable == nullptr) {
        return 0;
    }

    // Suche Node in vorhandenem RoutingTable
    for (int i = 0; i < totalRoutes; i++) {
        if (routingTable[i]->nodeId == dest) {
            return routingTable[i]->hop;
        }
    }
    return 0;
}

/**
 * Diese Methode wird aufgerufen, wenn ein vollständiges Serielles Paket über die USB Schnittstelle eingegangen ist.
 * @param serialPaket
 */
void MeshRouter::ProcessSerialPaket(SerialPaket_t *serialPaket) {
    // Announce Transmission with Header Paket
    auto *headerPaket = (FloodBroadcastHeaderPaket_t *) malloc(sizeof(FloodBroadcastHeaderPaket_t));

    headerPaket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_HEADER;
    headerPaket->lastHop = NodeID;
    headerPaket->id = 0;
    headerPaket->source = NodeID;
    headerPaket->size = serialPaket->size;

    QueuePaket((uint8_t *) headerPaket, sizeof(FloodBroadcastHeaderPaket_t));

    uint8_t fragment = 0;

    // Send Payload as Fragments
    for (int i = 0; i < serialPaket->size;) {
        auto *fragPaket = (FloodBroadcastFragmentPaket_t *) malloc(sizeof(FloodBroadcastFragmentPaket_t));
        fragPaket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT;
        fragPaket->id = headerPaket->id;
        fragPaket->fragment = fragment++;

        memcpy(&fragPaket->payload, serialPaket->payload + i, sizeof(fragPaket->payload));
        i += sizeof(fragPaket->payload);

        QueuePaket((uint8_t *) fragPaket, sizeof(FloodBroadcastFragmentPaket_t));
    }

    // Free Serial Data and Buffer
    free(serialPaket->payload);
    free(serialPaket);
}

void MeshRouter::OnFloodHeaderPaket(FloodBroadcastHeaderPaket_t *paket) {
    auto *receivedFragmentedPaket = (ReceivedFragmentedPaket_t *) malloc(sizeof(ReceivedFragmentedPaket_t));
    receivedFragmentedPaket->id = paket->id;
    receivedFragmentedPaket->size = paket->size;
    receivedFragmentedPaket->lastHop = paket->lastHop;
    receivedFragmentedPaket->source = paket->source;
    receivedFragmentedPaket->lastFragment = 0;
    receivedFragmentedPaket->received = 0;
    receivedFragmentedPaket->corrupted = false;

    *debugString = "FH: " + String(receivedFragmentedPaket->size);
    receivedFragmentedPaket->payload = (uint8_t *) malloc(paket->size);
    receivedPakets.add(receivedFragmentedPaket);
}

void MeshRouter::OnFloodFragmentPaket(FloodBroadcastFragmentPaket_t *paket) {
    ReceivedFragmentedPaket_t *receivedFragmentedPaket = getLocalFragmentPaketBuffer(paket->id);
    if (receivedFragmentedPaket == nullptr) {
        *debugString = "Fragment: NULLPTR";
        return;
    }

    *debugString = "Fragment: " + String(paket->fragment);

    // Check for lost Fragments
    if (paket->fragment - receivedFragmentedPaket->lastFragment > 1) {
        // We lost a Paket! Doesn't matter, fill out bytes and continue

        uint8_t lostFragments = paket->fragment - receivedFragmentedPaket->lastFragment - 1;

        receivedFragmentedPaket->received += (sizeof paket->payload) * lostFragments;

        receivedFragmentedPaket->lastFragment += lostFragments;
        receivedFragmentedPaket->corrupted = true;

#ifdef DEBUG_LORA_SERIAL
        Serial.println("FragLost!");
#endif
    }
#ifdef DEBUG_LORA_SERIAL
    Serial.println("OnFragment: Frag: " + String(paket->fragment) + " " + String(receivedFragmentedPaket->received) + "/" + String(receivedFragmentedPaket->size));
#endif
    uint16_t bytesLeft = receivedFragmentedPaket->size - receivedFragmentedPaket->received > (sizeof paket->payload)
                        ? (sizeof paket->payload) : receivedFragmentedPaket->size - receivedFragmentedPaket->received;
    // Copy paket data to buffer
    memcpy(receivedFragmentedPaket->payload + receivedFragmentedPaket->received, paket->payload, bytesLeft);
    receivedFragmentedPaket->received += bytesLeft;

    if (receivedFragmentedPaket->received == receivedFragmentedPaket->size) {
        // End of Transmission
        OnPaketForHost(receivedFragmentedPaket);
    }
}

ReceivedFragmentedPaket_t *MeshRouter::getLocalFragmentPaketBuffer(uint8_t transmissionid) {
    for (int i = 0; i < receivedPakets.size(); i++) {
        if (receivedPakets.get(i)->id == transmissionid) {
            return receivedPakets.get(i);
        }
    }
    return nullptr;
}

void MeshRouter::OnPaketForHost(ReceivedFragmentedPaket_t *paket) {
    auto serialPaket = (SerialPaket_t *) malloc(sizeof(SerialPaket_t));
    serialPaket->size = paket->size;
    serialPaket->payload = paket->payload;

    free(paket);

    Serial.write((uint8_t *) serialPaket, 5);
    Serial.write(serialPaket->payload, serialPaket->size);

    free(serialPaket->payload);
    free(serialPaket);
}

