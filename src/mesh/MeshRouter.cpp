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

    for (int i = 0; i < 255; i++) {
        NODE_ID_COUNTERS[i] = 0;
    }
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
    routingTable[foundIndex]->lastSeen = millis();
}

void MeshRouter::UpdateRSSI(uint8_t nodeId, int rssi) {
    if (routingTable == nullptr) {
        return;
    }

    for (int i = 0; i < totalRoutes; i++) {
        if (routingTable[i]->nodeId == nodeId) {
            routingTable[i]->rssi = rssi;
            routingTable[i]->lastSeen = millis();
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
    // Modem hat preable Empfangen
    if (cad) {
        SenderWait(0);
        preambleAdd = 1000 + predictPacketSendTime(255);
        cad = false;
    }

#ifdef TEST_MODE
    if (sendQueue.size() < 20 && millis() < 900000) {
        uint16_t size = 1730;
        auto dummyPayload = (uint8_t *) malloc(size);
        memset(dummyPayload, (uint8_t) 'M', size);
        CreateBroadcastPacket(dummyPayload, NodeID, size, ID_COUNTER++, 41234231432);

        free(dummyPayload);
    }
#endif



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

#ifdef ANNOUNCE_IN_INTERVAL
            if (millis() - lastAnounceTime > 3000) {
                MeshRouter::announceNodeId(0);
            }
#endif
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
    readPaketFromSRAM(rawPaket, 1, paketSize);

    // remove preamble wait value
    preambleAdd = 0;
    switch (messageType) {
        case MESSAGE_TYPE_FLOOD_BROADCAST_HEADER:
            MeshRouter::OnFloodHeaderPaket((FloodBroadcastHeaderPaket_t *) rawPaket, rssi);
            SenderWait(random(0, 150));
            break;
        case MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT:
            MeshRouter::OnFloodFragmentPaket((FloodBroadcastFragmentPaket_t *) rawPaket);
            SenderWait(random(0, 150));
            break;
        case MESSAGE_TYPE_NODE_ANNOUNCE:
            MeshRouter::OnNodeIdAnnouncePaket((NodeIdAnnounce_t *) rawPaket, rssi);
            break;
        case MESSAGE_TYPE_FLOOD_BROADCAST_ACK:
            MeshRouter::OnFloodBroadcastAck((FloodBroadcastAck_t *) rawPaket, rssi);
            break;
        default:
            break;
    }


    // Paket Processed - Free Ram
    free(rawPaket);
}

void MeshRouter::OnFloodBroadcastAck(FloodBroadcastAck_t *paket, int rssi) {
    // We have announced this as a Broadcast, so we expected this. Dont do anything.
    if (paket->source == NodeID) {
        return;
    }

    // We are not the Sender of the Broadcast.
    if (paket->source != NodeID) {

        if (getIncompletePaketById(paket->id, paket->source) == nullptr) {
            // We have not received the Broadcast Announcement (FloodBroadcastHeaderPaket_t). We need to sleep now for the duration of the Transmission.
            int fragments = paket->totalTransmissionSize / 251 + 1;
            SenderWait(predictPacketSendTime(paket->totalTransmissionSize) + fragments * 15);
        } else {
            // Just dont Send until next Fragment arrives
            SenderWait(predictPacketSendTime(255) + 50);
        }
    }
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
            MeshRouter::SenderWait(random(0, 900));
            MeshRouter::announceNodeId(0);
        }
    } else {
        // ToDo: Route Dispute
    }

    // Jode just started, reset NodeID Counter
    NODE_ID_COUNTERS[paket->nodeId] = 0;

#ifdef RETRANSMIT_PAKETS
    if (paket->nodeId != NodeID) {
        auto *repeatedPacked = (NodeIdAnnounce_t *) malloc(sizeof(NodeIdAnnounce_t));
        memcpy(repeatedPacked, paket, sizeof(NodeIdAnnounce_t));
        repeatedPacked->lastHop = NodeID;
        repeatedPacked->respond = 0;
        QueuePaket(&sendQueue, (uint8_t *) repeatedPacked, sizeof(NodeIdAnnounce_t), NodeID, 0);
    }
#endif

}

/**
 * Announces the current Node ID to the Network
 */
void MeshRouter::announceNodeId(uint8_t respond) {
    // Create Paket
    auto *paket = (NodeIdAnnounce_t *) malloc(sizeof(NodeIdAnnounce_t));
    lastAnounceTime = millis();

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

    QueuePaket(&sendQueue, (uint8_t *) paket, sizeof(NodeIdAnnounce_t), NodeID, 0);
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

void MeshRouter::QueuePaket(LinkedList<QueuedPaket_t *> *listPointer, uint8_t *rawPaket, uint8_t size, uint8_t source, uint16_t id, long waitTimeAfter, int64_t hash) {
    auto *paket = (QueuedPaket_t *)malloc(sizeof(QueuedPaket_t));
    paket->paketPointer = rawPaket;
    paket->paketSize = size;
    paket->waitTimeAfter = waitTimeAfter;
    paket->hash = hash;
    paket->source = source;
    paket->id = id;

    listPointer->add(paket);

    if (displayQueueLength != nullptr) {
        *displayQueueLength = sendQueue.size();
    }
}

void MeshRouter::ProcessQueue() {
    if (sendQueue.size() == 0 || blockSendUntil + preambleAdd > millis()) {
        return;
    }
    QueuedPaket_t *paketQueueEntry = sendQueue.shift();

    if (paketQueueEntry->paketPointer == nullptr) {
        return;
    }

    MeshRouter::SendRaw(paketQueueEntry->paketPointer,
                        paketQueueEntry->paketSize);

    // Wait for ACK of Receiving Nodes
    if (paketQueueEntry->waitTimeAfter > 0) {
        SenderWait(paketQueueEntry->waitTimeAfter);
    }

    free(paketQueueEntry->paketPointer);
    free(paketQueueEntry);
    paketQueueEntry->paketPointer = nullptr;

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

void MeshRouter::CreateBroadcastPacket(uint8_t *payload, uint8_t source, uint16_t size, uint16_t id, int64_t hash) {
    // Announce Transmission with Header Paket
    auto *headerLoraPaket = (FloodBroadcastHeaderPaket_t *) malloc(sizeof(FloodBroadcastHeaderPaket_t));
    headerLoraPaket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_HEADER;
    headerLoraPaket->lastHop = NodeID;
    headerLoraPaket->id = id;
    headerLoraPaket->source = source;
    headerLoraPaket->size = size;

    // Very simple Content Checksum
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < size; i++) {
        checksum += payload[i];
    }
    headerLoraPaket->checksum = checksum;

    LinkedList<QueuedPaket_t *> paketQueue;
    QueuePaket(&paketQueue, (uint8_t *) headerLoraPaket, sizeof(FloodBroadcastHeaderPaket_t), source, id, 150, hash);

    uint8_t fragment = 0;

    // Send Payload as Fragments
    for (uint16_t i = 0; i < size;) {
        auto fragPaket = (FloodBroadcastFragmentPaket_t *) malloc(sizeof(FloodBroadcastFragmentPaket_t));
        fragPaket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT;
        fragPaket->id = headerLoraPaket->id;
        fragPaket->fragment = fragment++;

        uint16_t bytesToCopy =
                sizeof(fragPaket->payload) > size - i ? size - i
                                                      : sizeof(fragPaket->payload);
        memcpy(&fragPaket->payload, payload + i, bytesToCopy);
        i += bytesToCopy;

        if (i == size) {
            long fullWaitTime = predictPacketSendTime(255);
            QueuePaket(&paketQueue, (uint8_t *) fragPaket, sizeof(FloodBroadcastFragmentPaket_t),source, id,
                       200 + random(fullWaitTime, fullWaitTime + 250),hash);
        } else {
            QueuePaket(&paketQueue, (uint8_t *) fragPaket, sizeof(FloodBroadcastFragmentPaket_t),source, id, 0, hash);
        }
    }

    //while(paketQueue.size() > 0){
    //    sendQueue.add(paketQueue.shift());
    //}

    AddToSendQueueReplaceSameHashedPackets(&paketQueue, hash, source);
}

/**
 * This Method Checks for Messages of the Same Type, from the Same Host and Replaces them with the new Packets at the Same Position in the SendQueue.
 * This Prevents the Queue to fill up with already out of date Messaged.
 * When no hash is provided or no Message is found, the packets are regularly queued to the sendQueue.
 * @param newPackts
 * @param hash
 * @param messageSource
 */
void MeshRouter::AddToSendQueueReplaceSameHashedPackets(LinkedList<QueuedPaket_t *> *newPackts, int64_t hash, uint8_t messageSource) {
    // Default: No Type Provided, just add to SendQueue
    if(hash == 0){
        while(newPackts->size() > 0){
            sendQueue.add(newPackts->shift());
        }
        return;
    }

    bool hasPacketToReplace = false;
    for(int k=0;k<sendQueue.size();k++){
        QueuedPaket_t *queuedPaket = sendQueue.get(k);
        if(queuedPaket->hash == hash && queuedPaket->paketPointer[0] == MESSAGE_TYPE_FLOOD_BROADCAST_HEADER && queuedPaket->source == messageSource){

            hasPacketToReplace = true;
            break;
        }
    }

    // No Packets to replace found!
    if(!hasPacketToReplace){
        while(newPackts->size() > 0){
            sendQueue.add(newPackts->shift());
        }
        return;
    }

    LinkedList<QueuedPaket_t *> preReplaceList;
    // We have a Message to replace!
    // shift all packets infront of this message to another temporary list, then remove the Message's Packets, add the new Packets, and add the packets of the temporary list
    bool foundFirstPacket = false;
    int shiftedStuff = 0;
    while(!foundFirstPacket && sendQueue.size() > 1){
        auto shiftedPacket = sendQueue.shift();

        // When Message Header in Queue, Packet is not send yet and should be replaced
        if(shiftedPacket->hash == hash && shiftedPacket->paketPointer[0] == MESSAGE_TYPE_FLOOD_BROADCAST_HEADER && shiftedPacket->source == messageSource){
            uint16_t id = shiftedPacket->id;

            auto nextPacket = sendQueue.shift();
            // Remove all Fragments belonging to this Message Header
            while(sendQueue.size() > 0 && nextPacket->source == messageSource && nextPacket->id == id){
                free(nextPacket->paketPointer);
                free(nextPacket);
                nextPacket = sendQueue.shift();
                shiftedStuff++;
            }

            // Eins zu weit shifted
            sendQueue.unshift(nextPacket);
            //*debugString = "I" + String(id) + "S" + String((uint16_t)source ) + "i"+ String(sendQueue.get(0).id) + "s" + String((uint16_t)sendQueue.get(0).source ) + ":" + String(shiftedStuff) + ":" + String((uint16_t)sendQueue.get(0).paketPointer[0]) ;

            // Insert new Packet on same Position
            while(newPackts->size() > 0){
                sendQueue.unshift(newPackts->pop());
            }

            foundFirstPacket = true;
            free(shiftedPacket->paketPointer);
            free(shiftedPacket);
        }else{
            // We havent found a matching packet yet, remove the first packet from the sendQueue and add it to the preReplaceList
            preReplaceList.add(shiftedPacket);
        }
    }

    // add the previously shifted packets to the front again
    while(preReplaceList.size() > 0){
        sendQueue.unshift(preReplaceList.pop());
    }
    while(newPackts->size() > 0){
        sendQueue.add(newPackts->shift());
    }
}


/**
* Diese Methode wird aufgerufen, wenn ein vollständiges Serielles Paket über die USB Schnittstelle eingegangen ist.
* @param serialPaket
*/
void MeshRouter::ProcessFloodSerialPaket(SerialPayloadFloodPaket_t *serialPayloadFloodPaket) {
    CreateBroadcastPacket(serialPayloadFloodPaket->payload, NodeID, serialPayloadFloodPaket->size, ID_COUNTER++, serialPayloadFloodPaket->hash);

    free(serialPayloadFloodPaket->payload);
}

/**
 * Wird aufgerufen, wenn der Header einer anstehenden Übertragung empfangen wird.
 * @param paket
 */
void MeshRouter::OnFloodHeaderPaket(FloodBroadcastHeaderPaket_t *paket, int rssi) {
    if (getIncompletePaketById(paket->id, paket->source) != nullptr) {
        // Already received this Paket!
        return;
    }

    if (NODE_ID_COUNTERS[paket->source] > paket->id) {
        // We already received this paket, this is just a repetition
        return;
    }

#ifdef RETRANSMIT_PAKETS
    // Send Broadcast Confirmation
    auto *broadcastAck = (FloodBroadcastAck_t *) malloc(sizeof(FloodBroadcastAck_t));
    broadcastAck->source = paket->source;
    broadcastAck->totalTransmissionSize = paket->size;
    broadcastAck->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_ACK;
    broadcastAck->id = paket->id;

    SendRaw((uint8_t *) broadcastAck, sizeof(FloodBroadcastAck_t));

    free(broadcastAck);
    broadcastAck = nullptr;
#endif

    UpdateRSSI(paket->source, rssi);

    auto *incompletePaket = (FragmentedPaket_t *) malloc(sizeof(FragmentedPaket_t));
    incompletePaket->id = paket->id;
    incompletePaket->size = paket->size;
    incompletePaket->lastHop = paket->lastHop;
    incompletePaket->source = paket->source;
    incompletePaket->lastFragment = 0;
    incompletePaket->received = 0;
    incompletePaket->corrupted = false;
    incompletePaket->checksum = paket->checksum;

    lastBroadcastSourceId = paket->source;

    MeshRouter::UpdateRSSI(paket->lastHop, rssi);
    MeshRouter::UpdateHOP(paket->source, paket->lastHop);

    uint16_t nextFragLength = (uint16_t) paket->size > 255 ? 255 : paket->size;
    SenderWait((unsigned long) 300 + predictPacketSendTime(nextFragLength));

    if(xPortGetFreeHeapSize() - incompletePaket->size > incompletePaket->size + 10000){
        *debugString = "FH: " + String(incompletePaket->size) + " ID: " + String(incompletePaket->id);
        incompletePaket->payload = (uint8_t *) malloc(paket->size);
        incompletePaketList.add(incompletePaket);
    }else{
        free(incompletePaket);
    }

}

void MeshRouter::OnFloodFragmentPaket(FloodBroadcastFragmentPaket_t *paket) {
    FragmentedPaket_t *incompletePaket = getIncompletePaketById(paket->id, lastBroadcastSourceId);
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
    SenderWait(150 + predictPacketSendTime(bytesLeft > 255 ? 255 : bytesLeft));

    // Copy paket data to buffer
    memcpy(incompletePaket->payload + incompletePaket->received, paket->payload, bytesLeft);
    incompletePaket->received += bytesLeft;

    // Set last Received fragment
    incompletePaket->lastFragment = paket->fragment;

    //*debugString = "F" + String(paket->fragment) + "R" + String(incompletePaket->received) + "S" +
    //               String(incompletePaket->size) + "L" + String(bytesLeft);

    if (incompletePaket->received == incompletePaket->size) {
        // End of Transmission

        // Very simple Content Checksum
        uint8_t checksum = 0;
        for (uint16_t i = 0; i < incompletePaket->size; i++) {
            checksum += incompletePaket->payload[i]; // Intentional Overflow, Allowed and strictly defined in c standard for unsigned values
        }

        if (checksum != incompletePaket->checksum) {
            *debugString = "Invalid CS!" + String(checksum);
            incompletePaket->corrupted = true;
        } else if (incompletePaket->corrupted) {
            *debugString = "Paket Lost!";
        } else {
            *debugString = "Success: " + String(checksum);
        }

        if (!incompletePaket->corrupted) {
            OnPaketForHost(incompletePaket);
        }
    }
}

FragmentedPaket_t *MeshRouter::getIncompletePaketById(uint16_t transmissionid, uint8_t source) {
    for (int i = 0; i < incompletePaketList.size(); i++) {
        if (incompletePaketList.get(i)->id == transmissionid && incompletePaketList.get(i)->source == source) {
            return incompletePaketList.get(i);
        }
    }
    return nullptr;
}

void MeshRouter::removeIncompletePaketById(uint16_t transmissionid, uint8_t source) {
    for (int i = 0; i < incompletePaketList.size(); i++) {
        if (incompletePaketList.get(i)->id == transmissionid && incompletePaketList.get(i)->source == source) {
            incompletePaketList.remove(i);
        }
    }
}

void MeshRouter::OnPaketForHost(FragmentedPaket_t *paket) {
    auto payloadBuffer = (uint8_t *) malloc(sizeof(FragmentedPaket_t) + paket->size);
    memcpy(payloadBuffer, paket, sizeof(FragmentedPaket_t));
    memcpy(payloadBuffer + sizeof(FragmentedPaket_t), paket->payload, paket->size);

    auto serialPaketHeader = (SerialPaketHeader_t *) malloc(sizeof(SerialPaketHeader_t));
    serialPaketHeader->serialPaketType = SERIAL_PAKET_TYPE_FLOOD_PAKET;
    serialPaketHeader->size = sizeof(FragmentedPaket_t) + paket->size;

#ifdef TEST_MODE
    Serial.println("[PACKET] " + String(paket->size));
#else
    uint8_t magicBytes[] = { 0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2 };
    Serial.write(magicBytes, 8);
    Serial.write((uint8_t *) serialPaketHeader, 3);
    Serial.write(payloadBuffer, serialPaketHeader->size);
#endif



#ifdef TEST_MODE
    if(millis() < 900000){
        receivedBytes += paket->size;
    }
#endif


    // ReQueue Packet
#ifdef RETRANSMIT_PAKETS
    CreateBroadcastPacket(paket->payload, paket->source, paket->size, paket->id);
#endif
    *debugString = "HOST SEND: " + String(paket->size);

    blockSendUntil = millis() + random(10, 250);

    removeIncompletePaketById(paket->id, paket->source);
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

long MeshRouter::predictPacketSendTime(uint8_t size) {
    long time = (long) (1.0 * timePerByte * size + sendOverhead);
    if (time < 10) {
        time = 500;
    }
    return time;
}

