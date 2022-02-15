#include <Arduino.h>
#include <mesh/MeshRouter.h>
#include <lib/LoRa.h>
#include "config.h"

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

void MeshRouter::UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[], int rssi) {
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

void MeshRouter::UpdateRSSI(uint8_t nodeId, int rssi) {
    if (routingTable == nullptr) {
        return;
    }

    for (int i = 0; i < totalRoutes; i++) {
        if (routingTable[i]->nodeId == nodeId) {
            routingTable[i]->rssi = rssi;
            return;
        }
    }
}

void MeshRouter::UpdateHOP(uint8_t nodeId, uint8_t hop) {
    if (routingTable == nullptr) {
        return;
    }
    for (int i = 0; i < totalRoutes; i++) {
        if (routingTable[i]->nodeId == nodeId) {
            routingTable[i]->hop = hop;
            return;
        }
    }
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
    // Modem empfängt Nachricht. Warte bis Ready.
    if (cad) {
        SenderWait(400 + predictPacketSendTime(255));
        cad = false;
    }

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
                int rssi = LoRa.packetRssi();

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

void MeshRouter::OnReceivePacket(uint8_t messageType, uint8_t *rawPaket, uint8_t paketSize, int rssi) {
#ifdef DEBUG_LORA_SERIAL
    Serial.println("MessageType: " + String(messageType));
#endif
    switch (messageType) {
        case MESSAGE_TYPE_UNICAST_DATA_PAKET:
            MeshRouter::OnUnicastPaket((UnicastMeshPaket_t *) rawPaket, paketSize, rssi);
            break;
        case MESSAGE_TYPE_FLOOD_BROADCAST_HEADER:
            readPaketFromSRAM(rawPaket, 1, paketSize);
            MeshRouter::OnFloodHeaderPaket((FloodBroadcastHeaderPaket_t *) rawPaket, rssi);
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

void MeshRouter::OnNodeIdAnnouncePaket(NodeIdAnnounce_t *paket, int rssi) {
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
            MeshRouter::SenderWait(NodeID * 10);
            MeshRouter::announceNodeId(0);
        }
    } else {
        // ToDo: Route Dispute
    }
}

void MeshRouter::OnUnicastPaket(UnicastMeshPaket_t *paket, uint8_t size, int rssi) {
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
    unsigned long time = millis();
    LoRa.beginPacket();
    LoRa.write(rawPaket, size);
    LoRa.endPacket();
    packetTime = millis() - time;
    measurePaketTime(size, packetTime);

    LoRa.receive();
}

/**
 * Delays the Next paket in the Queue for a specific time
 * @param delay
 */
void MeshRouter::SenderWait(unsigned long waitTime) {
    if (blockSendUntil < millis() + waitTime) {
        blockSendUntil = millis() + waitTime;
    }
}

void MeshRouter::QueuePaket(uint8_t *rawPaket, uint8_t size) {
    QueuedPaket_t paket;
    paket.paketPointer = rawPaket;
    paket.paketSize = size;
    sendQueue.add(paket);
    if (displayQueueLength != nullptr) {
        *displayQueueLength = sendQueue.size();
    }
}

void MeshRouter::ProcessQueue() {
    if (sendQueue.size() == 0 || blockSendUntil > millis()) {
        return;
    }
    QueuedPaket_t paketQueueEntry = sendQueue.shift();

    MeshRouter::SendRaw(paketQueueEntry.paketPointer,
                        paketQueueEntry.paketSize);

    // Wait for Nodes to Receive and Retransmit Paket
    SenderWait(200 + predictPacketSendTime(paketQueueEntry.paketSize));

    free(paketQueueEntry.paketPointer);

    // Update Queue on Display
    if (displayQueueLength != nullptr) {
        *displayQueueLength = sendQueue.size();
    }
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
uint16_t ID_COUNTER = 0;

void MeshRouter::ProcessFloodSerialPaket(SerialPayloadFloodPaket_t *serialPayloadFloodPaket) {
    // Announce Transmission with Header Paket
    auto *headerLoraPaket = (FloodBroadcastHeaderPaket_t *) malloc(sizeof(FloodBroadcastHeaderPaket_t));
    headerLoraPaket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_HEADER;
    headerLoraPaket->lastHop = NodeID;
    headerLoraPaket->id = ID_COUNTER++;
    headerLoraPaket->source = NodeID;
    headerLoraPaket->size = serialPayloadFloodPaket->size;

    // Very simple Content Checksum
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < serialPayloadFloodPaket->size; i++) {
        checksum += serialPayloadFloodPaket->payload[i];
    }
    headerLoraPaket->checksum = checksum;

    QueuePaket((uint8_t *) headerLoraPaket, sizeof(FloodBroadcastHeaderPaket_t));

    uint8_t fragment = 0;

    // Send Payload as Fragments
    for (uint16_t i = 0; i < serialPayloadFloodPaket->size;) {
        auto *fragPaket = (FloodBroadcastFragmentPaket_t *) malloc(sizeof(FloodBroadcastFragmentPaket_t));
        fragPaket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT;
        fragPaket->id = headerLoraPaket->id;
        fragPaket->fragment = fragment++;

        uint16_t bytesToCopy =
                sizeof(fragPaket->payload) > serialPayloadFloodPaket->size - i ? serialPayloadFloodPaket->size - i
                                                                               : sizeof(fragPaket->payload);
        memcpy(&fragPaket->payload, serialPayloadFloodPaket->payload + i, bytesToCopy);
        i += bytesToCopy;
        QueuePaket((uint8_t *) fragPaket, sizeof(FloodBroadcastFragmentPaket_t));
    }

    // Free Serial Data and Buffer
    // free(serialPayloadFloodPaket->payload);
    // free(serialPayloadFloodPaket);
}

/**
 * Wird aufgerufen, wenn der Header einer anstehenden Übertragung empfangen wird.
 * @param paket
 */
void MeshRouter::OnFloodHeaderPaket(FloodBroadcastHeaderPaket_t *paket, int rssi) {
    if(getIncompletePaketById(paket->id) != nullptr){
        SenderWait((unsigned long) 300 + predictPacketSendTime(255));
        return;
    }

    auto *incompletePaket = (FragmentedPaket_t *) malloc(sizeof(FragmentedPaket_t));
    incompletePaket->id = paket->id;
    incompletePaket->size = paket->size;
    incompletePaket->lastHop = paket->lastHop;
    incompletePaket->source = paket->source;
    incompletePaket->lastFragment = 0;
    incompletePaket->received = 0;
    incompletePaket->corrupted = false;
    incompletePaket->checksum = paket->checksum;

    MeshRouter::UpdateRSSI(paket->lastHop, rssi);
    MeshRouter::UpdateHOP(paket->source, paket->lastHop);

    uint16_t nextFragLength = (uint16_t) paket->size > 255 ? 255 : paket->size;
    SenderWait((unsigned long) 300 + predictPacketSendTime(nextFragLength));

    *debugString = "FH: " + String(incompletePaket->size) + " ID: " + String(incompletePaket->id);
    incompletePaket->payload = (uint8_t *) malloc(paket->size);
    incompletePaketList.add(incompletePaket);

    // Redirect to next Node
    paket->lastHop = NodeID;
    retransmitPaket((uint8_t *)paket, 4, sizeof (FloodBroadcastHeaderPaket_t));
}

void MeshRouter::OnFloodFragmentPaket(FloodBroadcastFragmentPaket_t *paket) {
    FragmentedPaket_t *incompletePaket = getIncompletePaketById(paket->id);
    if (incompletePaket == nullptr) {
        // Keine Informationen über zukünftige Pakete. Gehe vom größten aus!
        SenderWait(400 + predictPacketSendTime(255));
        *debugString = "Fragment: ERR";
        return;
    }

    // Igore! We are receiving a repeated Paket!
    if (paket->fragment <= incompletePaket->lastFragment && paket->fragment > 0) {
        return;
    }

    // Check for lost Fragments
    if (paket->fragment - incompletePaket->lastFragment > 1) {
        // We lost a Paket! Doesn't matter, fill out bytes and continue

        *debugString = "LOSS: " + String(paket->fragment);
        uint8_t lostFragments = paket->fragment - incompletePaket->lastFragment - 1;

        incompletePaket->received += (sizeof paket->payload) * lostFragments;

        incompletePaket->lastFragment += lostFragments;
        incompletePaket->corrupted = true;
    }

    uint16_t bytesLeft = incompletePaket->size - incompletePaket->received > (sizeof paket->payload)
                         ? (sizeof paket->payload)
                         : incompletePaket->size - incompletePaket->received;

    // Delay Transmission of Pakets
    SenderWait(400 + predictPacketSendTime(bytesLeft > 255 ? 255 : bytesLeft));

    // Copy paket data to buffer
    memcpy(incompletePaket->payload + incompletePaket->received, paket->payload, bytesLeft);
    incompletePaket->received += bytesLeft;

    // Set last Received fragment
    incompletePaket->lastFragment = paket->fragment;

    *debugString = "F" + String(paket->fragment) + "R" + String(incompletePaket->received) + "S" +
                   String(incompletePaket->size) + "L" + String(bytesLeft);

    if (incompletePaket->received == incompletePaket->size) {
        // End of Transmission

        // Very simple Content Checksum
        uint8_t checksum = 0;
        for (uint16_t i = 0; i < incompletePaket->size; i++) {
            checksum += incompletePaket->payload[i]; // Intentional Overflow, Allowed and strictly defined in c standard for unsigned values
        }

        if (checksum != incompletePaket->checksum) {
            *debugString = "Invalid CS!" + String(checksum);
        } else if (incompletePaket->corrupted) {
            *debugString = "Paket Lost!";
        } else {
            *debugString = "Success: " + String(checksum);
        }
        OnPaketForHost(incompletePaket);

        // Wait
        SenderWait(400 + 2 * predictPacketSendTime(255));
        retransmitPaket((uint8_t *)paket, 0, sizeof (FloodBroadcastFragmentPaket_t));
    }
}

FragmentedPaket_t *MeshRouter::getIncompletePaketById(uint16_t transmissionid) {
    for (int i = 0; i < incompletePaketList.size(); i++) {
        if (incompletePaketList.get(i)->id == transmissionid) {
            return incompletePaketList.get(i);
        }
    }
    return nullptr;
}

void MeshRouter::OnPaketForHost(FragmentedPaket_t *paket) {
    auto payloadBuffer = (uint8_t *) malloc(sizeof(FragmentedPaket_t) + paket->size);
    memcpy(payloadBuffer, paket, sizeof(FragmentedPaket_t));
    memcpy(payloadBuffer + sizeof(FragmentedPaket_t), paket->payload, paket->size);

    auto serialPaketHeader = (SerialPaketHeader_t *) malloc(sizeof(SerialPaketHeader_t));
    serialPaketHeader->serialPaketType = SERIAL_PAKET_TYPE_FLOOD_PAKET;
    serialPaketHeader->size = sizeof(FragmentedPaket_t) + paket->size;

    Serial.write((uint8_t *) serialPaketHeader, 3);
    Serial.write(payloadBuffer, serialPaketHeader->size);

    *debugString = "HOST SEND: " + String(paket->size);

    free(paket->payload);
    free(paket);
    free(serialPaketHeader);
    free(payloadBuffer);
}

// Calculates the AirTime of a paket, based on its Payload size.
// Calculation is based of the LoRa Chip Datasheet.
// SX1276-7-8-9 Datasheet-> https://semtech--c.na98.content.force.com/sfc/dist/version/download/?oid=00DE0000000JelG&ids=0682R000006TQEPQA4&d=%2Fa%2F2R0000001Rbr%2F6EfVZUorrpoKFfvaF_Fkpgp5kzjiNyiAbqcpqh9qSjE&operationContext=DELIVERY&asPdf=true&viewId=05H2R000002POsoUAG&dpt=
double MeshRouter::getSendTimeByPaketSizeInMS(int PL) {
    double R_s = 1.0 * LORA_BANDWIDTH / pow(2, LORA_SPREADINGFACTOR);
    double T_s = 1.0 / R_s;

    double T_preamble = (LORA_PREAMBLE_LENGTH + 4.25) * T_s;

    int CR = 1; // 1 corresponding to 4/5, 4 to 4/8, see Datasheet page 31
    int CRC = 1;

    int n_payload =
            8 +
            max((int) ceil((8.0 * PL - 4 * LORA_SPREADINGFACTOR + 28 + 16 * CRC) / 4 * (LORA_SPREADINGFACTOR)) *
                (CR + 4),
                0);

    double t_payload = n_payload * T_s;
    double t_packet = t_payload + T_preamble;

    return t_packet;
}

unsigned long timeMap[256];
uint8_t totalTimeMapEntries = 0;

void MeshRouter::measurePaketTime(uint8_t size, unsigned long time) {
    if (timeMap[size] > 0) {
        return;
    }

    timeMap[size] = time;
    totalTimeMapEntries++;

    if (totalTimeMapEntries > 1) {
        uint8_t size_1 = 0;
        unsigned long time_1 = 0;
        for (int i = 0; i < 256; i++) {
            //1. value
            if (timeMap[i] > 0 && size_1 == 0) {
                size_1 = i;
                time_1 = timeMap[i];
            } else if (timeMap[i] > 0 && size_1 > 0) {
                // 2. value
                uint8_t sizeDiff = i - size_1;
                unsigned long timeDiff = timeMap[i] - time_1;
                timePerByte = 1.0 * timeDiff / sizeDiff;
                sendOverhead = timeMap[i] - 1.0 * i * timePerByte;
            }
        }
    }
}

unsigned long MeshRouter::predictPacketSendTime(uint8_t size) {
    unsigned long time = (unsigned long) (1.0 * timePerByte * size + sendOverhead);
    if (time < 10) {
        time = 500;
    }
    return time;
}

/**
 * Überschreibt die ersten n bytes im LoRa Modul RAM mit dem angegebenem Buffer Inhalt.
 * Wird verwendet, um Pakete neu zu Transferieren, ohne das Paket komplett neu Schreiben zu müssen.
 * @param overrideBuffer
 * @param overrideSize
 * @param totalSize
 */
void MeshRouter::retransmitPaket(uint8_t *overrideBuffer, uint8_t overrideSize, uint8_t totalSize) {
    if(overrideSize > 0){
        writePaketToSRAM(overrideBuffer, 0, overrideSize - 1);
    }

    LoRa.writeRegister(REG_FIFO_ADDR_PTR, 0);
    LoRa.writeRegister(REG_PAYLOAD_LENGTH, totalSize);

    LoRa.endPacket();
}
