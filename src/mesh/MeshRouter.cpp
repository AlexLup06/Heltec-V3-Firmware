#include "mesh\MeshRouter.h"

/**
 * Incoming Packets are Processed here
 **/

std::mutex serial_mtx;
std::mutex* getSerialMutex(){return &serial_mtx;}

/* Set new node ID
* Input parameters: new node ID
*/
uint8_t MeshRouter::setNodeID(uint8_t newNodeID)
{
    NodeID = newNodeID;

	// Announce Local NodeID to the Network to
	MeshRouter::announceNodeId(1);
    return NodeID;
}

// Returns signal - to - noise ratio of the latest received packet.
int8_t MeshRouter::getSNR()
{
    return LoRa.getSNR();
}

/* Returns RSSI as positive value.
* Input parameters : new node ID
*/
uint8_t MeshRouter::getRSSI(uint8_t nodeID)
{
	if (routingTable == nullptr) {
		return 255;
	}

	for (int i = 0; i < totalRoutes; i++) {
		if (routingTable[i]->nodeId == nodeID) {
            return 0-routingTable[i]->rssi; //make positiv
		}
	}
    return 255;
}

/* Change LoRa module configuration.
* Input parameters: spreading factor, transmission power, frequency, bandwidth
*/
   void MeshRouter::applyModemConfig(uint8_t spreading_factor, uint8_t transmission_power, uint32_t frequency, uint32_t bandwidth)
   {
    m_spreading_factor=spreading_factor;
    m_transmission_power=transmission_power;
    m_frequnecy=frequency;
    m_bandwidth=bandwidth;
    reInitLoRa();
   }

// Initialize LoRa communication. This function starts a SPI communication and sets LoRa module pins. It sets the Tx power, signal bandwidth, spreading factor and preamble length to their default values. It also sets a sync word to avoid interference from other Lora networks.
void MeshRouter::initLoRa()
{
   SPI.begin(SCK, MISO, MOSI, SS);

    LoRa.setPins(SS, RST_LoRa, DIO0);
    if (!LoRa.begin(LORA_FREQUENCY, false))
    {
        Serial.println("LoRa init failed. Check your connections.");
        while (true)
            ;
    }
    // Default:
    // High Power:
    LoRa.setTxPower(20, RF_PACONFIG_PASELECT_PABOOST);
    LoRa.setSignalBandwidth(LORA_BANDWIDTH);
    LoRa.setSpreadingFactor(LORA_SPREADINGFACTOR);
    LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);

    // Set your own sync word so that packets from other LoRa networks are ignored.
    // Eigenes Sync-Word setzen, damit Pakete anderer LoRa Netzwerke ignoriert werden
    LoRa.setSyncWord(0x12);
}

// Re-initialize LoRa module.
void MeshRouter::reInitLoRa()
{
   LoRa.end();
   vTaskDelay(50);
    if (!LoRa.begin(m_frequnecy, false))
    {
        Serial.println("LoRa init failed. Check your connections.");
        while (true)
            ;
    }
    // Default:
    // High Power:
    LoRa.setTxPower(m_transmission_power, RF_PACONFIG_PASELECT_PABOOST);
    LoRa.setSignalBandwidth(m_bandwidth);
    LoRa.setSpreadingFactor(m_spreading_factor);
    LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);

    // Set your own sync word so that packets from other LoRa networks are ignored.
    // Eigenes Sync-Word setzen, damit Pakete anderer LoRa Netzwerke ignoriert werden
    LoRa.setSyncWord(0x12);
}

// Initialize node by setting Node ID from MAC address.
void MeshRouter::initNode() {
    this->initLoRa();
    // Read MAC Adress
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    // 255 = no assigned ID
    NodeID = macAdress[5];
    routingTable = nullptr;

    // Announce Local NodeID to the Network to
    MeshRouter::announceNodeId(1);

    for (int i = 0; i < 255; i++) {
        NODE_ID_COUNTERS[i] = 0;
    }
}

/* Update route parameters for the given node ID.If node ID not found in Routing Table, new row is created based on MAC address.
* Input parameters: node ID, hop value, device MAC address, RSSI
*/ 
void MeshRouter::UpdateRoute(uint8_t nodeId, uint8_t hop, uint8_t deviceMac[], int rssi) {
    // Malloc pointer array, when not allocated yet
    if (routingTable == nullptr) {
        routingTable = (RoutingTable_t **) malloc(sizeof(RoutingTable_t *) * 255);
        totalRoutes = 0;
    }

#ifdef DEBUG_LORA_SERIAL
    Serial.println("nodeId: " + String(nodeId));
#endif
    // Search Node in existing RoutingTable
    // Suche Node in vorhandenem RoutingTable
    uint8_t foundIndex = 255;
    for (int i = 0; i < totalRoutes; i++) {
        if (memcmp(routingTable[i]->deviceMac, deviceMac, 6) == 0) {
            foundIndex = i;
        }
    }

    // Node not yet in Routing Table
    // Node noch nicht im Routing Table
    if (foundIndex == 255) {
        totalRoutes++;

        foundIndex = totalRoutes - 1;
        // Allocate actual data structure
        // Alloziiere eigentliche Datenstruktur
        routingTable[foundIndex] = (RoutingTable_t *) malloc(sizeof(RoutingTable_t));
        memcpy(routingTable[foundIndex]->deviceMac, deviceMac, 6);
    }
    
    routingTable[foundIndex]->nodeId = nodeId;
    routingTable[foundIndex]->hop = hop;
    routingTable[foundIndex]->rssi = rssi;
    routingTable[foundIndex]->lastSeen = millis();
}

/* Set RSSI for the given node ID.
* Input parameters: node ID, RSSI value
*/
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

/* Set new hop value for the given node ID.
* Input parameters: node ID, hop value
*/
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

/* Finds and returns the next free node ID in Routing Table.
* Input parameters: ...??
*/ 
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

// Print Routing Table.
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
 * Main Loop: Tasks performed based on operating-mode. This loop runs endlessly.
 * 
 */
void MeshRouter::handle() {
    // Modem has preable receiving
    // Modem hat preable Empfangen
    if (cad) {
        SenderWait(0);
        // Increase blocking time of other senders.
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
            // Initialize LoRa module.
            // If already initialized, put LoRa module in continuous receive mode.
            LoRa.receive();
            OPERATING_MODE = OPERATING_MODE_RECEIVE;
            break;
        case OPERATING_MODE_SEND:
            // ToDo: Process Packet Queue

            break;
        case OPERATING_MODE_RECEIVE:
            // LoRa module in receive mode
            if (receiveState == RECEIVE_STATE_PACKET_READY) {
                // We need to set the Module to idle, to prevent the module from overwriting the SRAM with a new Packet and to be able to modify the Modem SRAM Pointer
                LoRa.idle();

                auto *receiveBuffer = (uint8_t *) malloc(readyPacketSize);
                int rssi = LoRa.packetRssi();

                // set FIFO address to current RX address
                LoRa.writeRegister(REG_FIFO_ADDR_PTR, LoRa.readRegister(REG_FIFO_RX_CURRENT_ADDR));

                // We only need the Packet Type at this point
                receiveBuffer[0] = LoRa.readRegister(REG_FIFO);

                // If packet is received, data is copied from SRAM.
                OnReceivePacket(receiveBuffer[0], receiveBuffer, readyPacketSize, rssi);

                receiveState = RECEIVE_STATE_IDLE;

                // Set Modem SRAM Pointer to 0x00
                LoRa.writeRegister(REG_FIFO_RX_CURRENT_ADDR, 0);
                LoRa.receive();
            }
            // Send packet from the sendqueue.
            ProcessQueue();

#ifdef ANNOUNCE_IN_INTERVAL
            if (millis() - lastAnnounceTime > 5000) {
                MeshRouter::announceNodeId(0);
            }
#endif
            break;

        case OPERATING_MODE_UPDATE_IDLE:
            break;
    }
}

/* Handles received interrupt packets.
* Input parameters: size of packet
*/
void MeshRouter::OnReceiveIR(int size) {
    receiveState = RECEIVE_STATE_PACKET_READY;
    readyPacketSize = size;
}

/**
 * Reads the Rest of the Packet into the given receive Buffer
 * Input parameters: receiveBuffer, size
 */
void MeshRouter::readPacketFromSRAM(uint8_t *receiveBuffer, uint8_t start, uint8_t end) {
    for (int i = start; i < end; i++) {
        receiveBuffer[i] = LoRa.readRegister(REG_FIFO);
    }
}

/**
 * Writes the Rest of the Packet into the given receive Buffer
 * Input parameters: packet to be written, size
 */
void MeshRouter::writePacketToSRAM(uint8_t *packet, uint8_t start, uint8_t end) {
    LoRa.writeRegister(REG_FIFO_ADDR_PTR, start);
    for (int i = start; i < end; i++) {
        LoRa.writeRegister(REG_FIFO, packet[i]);
    }
}

/* The function reads the data from the SRAM and sets the preambleAdd to zero, because data was received. The evaluation of the data starts with the first byte which indicates the message type.
* Input parameters: message type, packet, packet size, rssi
*/
void MeshRouter::OnReceivePacket(uint8_t messageType, uint8_t *rawPacket, uint8_t packetSize, int rssi) {
#ifdef DEBUG_LORA_SERIAL
    Serial.println("MessageType: " + String(messageType));
#endif
    readPacketFromSRAM(rawPacket, 1, packetSize);

    // remove preamble wait value
    preambleAdd = 0;
    switch (messageType) {
        case MESSAGE_TYPE_FLOOD_BROADCAST_HEADER:
            MeshRouter::OnFloodHeaderPacket((FloodBroadcastHeaderPacket_t *) rawPacket, rssi);
            SenderWait(0 + (rand() % 101));//(random(0, 150));
            break;
        case MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT:
            MeshRouter::OnFloodFragmentPacket((FloodBroadcastFragmentPacket_t *) rawPacket);
            SenderWait(0 + (rand() % 101));//(random(0, 150));
            break;
        case MESSAGE_TYPE_NODE_ANNOUNCE:
            MeshRouter::OnNodeIdAnnouncePacket((NodeIdAnnounce_t *) rawPacket, rssi);
            break;
        case MESSAGE_TYPE_FLOOD_BROADCAST_ACK:
            MeshRouter::OnFloodBroadcastAck((FloodBroadcastAck_t *) rawPacket, rssi);
            break;
        default:
            break;
    }


    // Packet Processed - Free Ram
    free(rawPacket);
}

/* Sets all nodes which are not the sender into recieving mode to prevent collisions on the media for long packet transmissions.
* Input parameters: packet, RSSI value
*/ 
void MeshRouter::OnFloodBroadcastAck(FloodBroadcastAck_t *packet, int rssi) {
    // We have announced this as a Broadcast, so we expected this. Dont do anything.
    if (packet->source == NodeID) {
        return;
    }

    // We are not the Sender of the Broadcast.
    if (packet->source != NodeID) {

        if (getIncompletePacketById(packet->id, packet->source) == nullptr) {
            // We have not received the Broadcast Announcement (FloodBroadcastHeaderPacket_t). We need to sleep now for the duration of the Transmission.
            int fragments = packet->totalTransmissionSize / 251 + 1;
            SenderWait(predictPacketSendTime(packet->totalTransmissionSize) + fragments * 15);
        } else {
            // Just dont Send until next Fragment arrives
            SenderWait(predictPacketSendTime(255));//(predictPacketSendTime(255) + 50);
        }
    }
}

/* Announces the Node ID to the Network
* Input parameters: packet, RSSI value
*/
void MeshRouter::OnNodeIdAnnouncePacket(NodeIdAnnounce_t *packet, int rssi) {
#ifdef DEBUG_LORA_SERIAL
    char macHexStr[18];
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", packet->deviceMac[0], packet->deviceMac[1], packet->deviceMac[2],
            packet->deviceMac[3], packet->deviceMac[4], packet->deviceMac[5]);
    Serial.println("NodeIDDiscorveryFrom: " + String(macHexStr));
#endif
    if (NodeID != packet->nodeId) {
        MeshRouter::UpdateRoute(packet->nodeId, packet->lastHop, packet->deviceMac, rssi);

        if (packet->respond == 1) {
            // Unknown node! Block Sending for a time window, to allow other Nodes to respond.
            MeshRouter::SenderWait(0 + (rand() % 451));// (random(0, 900));
            MeshRouter::announceNodeId(0);
        }
    } else {
        // ToDo: Route Dispute
    }

    // Node just started, reset NodeID Counter
    NODE_ID_COUNTERS[packet->nodeId] = 0;

#ifdef RETRANSMIT_PACKETS
    if (packet->nodeId != NodeID) {
        auto *repeatedPacked = (NodeIdAnnounce_t *) malloc(sizeof(NodeIdAnnounce_t));
        memcpy(repeatedPacked, packet, sizeof(NodeIdAnnounce_t));
        repeatedPacked->lastHop = NodeID;
        repeatedPacked->respond = 0;
        QueuePacket(&sendQueue, (uint8_t *) repeatedPacked, sizeof(NodeIdAnnounce_t), NodeID, 0);
    }
#endif

}

/**
 * Announces the current Node ID to the Network
 * Input parameters: packet respond variable
 */
void MeshRouter::announceNodeId(uint8_t respond) {
    // Create Packet
    auto *packet = (NodeIdAnnounce_t *) malloc(sizeof(NodeIdAnnounce_t));
    lastAnnounceTime = millis();

    packet->messageType = MESSAGE_TYPE_NODE_ANNOUNCE;
    packet->nodeId = NodeID;
    packet->lastHop = NodeID;
    packet->respond = respond;
    memcpy(packet->deviceMac, macAdress, 6);

    char macHexStr[18] = {0};
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", packet->deviceMac[0], packet->deviceMac[1], packet->deviceMac[2],
            packet->deviceMac[3], packet->deviceMac[4], packet->deviceMac[5]);
#ifdef DEBUG_LORA_SERIAL
    Serial.println("SendNodeDiscovery: " + String(macHexStr));
#endif
    // Queue the packet into the sendQueue
    QueuePacket(&sendQueue, (uint8_t *) packet, sizeof(NodeIdAnnounce_t), NodeID, 0);
}

/**
 * Directly sends Packet with Hardware, you should never use this directly, because this will not wait for the Receive window of other Devices
 * Input parameters: rawPacket , size  
 */
void MeshRouter::SendRaw(uint8_t *rawPacket, uint8_t size) {
    unsigned long time = millis();
    LoRa.beginPacket();
    LoRa.write(rawPacket, size);
    LoRa.endPacket();
    packetTime = millis() - time;
    measurePacketTime(size, packetTime);

    LoRa.receive();
}

/**
 * Delays the Next packet in the Queue for a specific time
 * Input parameters: delay time
 */
void MeshRouter::SenderWait(unsigned long waitTime) {
    if (blockSendUntil < millis() + waitTime) {
        blockSendUntil = millis() + waitTime;
    }
}

/* Adds the packet to the sendQueue.
 * Input parameters: linked list of pointers to packets, packet to be added to queue, size of packet, source of packet, packet ID, wait time, hash value
 */
void MeshRouter::QueuePacket(LinkedList<QueuedPacket_t *> *listPointer, uint8_t *rawPacket, uint8_t size, uint8_t source, uint16_t id, long waitTimeAfter, int64_t hash) {
    auto *packet = (QueuedPacket_t *)malloc(sizeof(QueuedPacket_t));
    packet->packetPointer = rawPacket;
    packet->packetSize = size;
    packet->waitTimeAfter = waitTimeAfter;
    packet->hash = hash;
    packet->source = source;
    packet->id = id;

    listPointer->add(packet);

    if (displayQueueLength != nullptr) {
        *displayQueueLength = sendQueue.size();
    }
}

// Send packet only if the queue is not empty and the blocking time is less than the current time measured in milli seconds. Other senders need to wait until the package is sent.
void MeshRouter::ProcessQueue() {
    if (sendQueue.size() == 0 || blockSendUntil + preambleAdd > millis()) {
        return;
    }
    QueuedPacket_t *packetQueueEntry = sendQueue.shift();

    if (packetQueueEntry->packetPointer == nullptr) {
        return;
    }

    MeshRouter::SendRaw(packetQueueEntry->packetPointer,
                        packetQueueEntry->packetSize);

    // Wait for ACK of Receiving Nodes before sending another packet.
    if (packetQueueEntry->waitTimeAfter > 0) {
        SenderWait(packetQueueEntry->waitTimeAfter);
    }

    // Free sent packet queue pointer
    free(packetQueueEntry->packetPointer);
    free(packetQueueEntry);
    packetQueueEntry->packetPointer = nullptr;

    // Update Queue length on Display
    if (displayQueueLength != nullptr) {
        *displayQueueLength = sendQueue.size();
    }
}

/* Return hop of the destination node if found in Routing Table, else return 0.
 * Input parameters: destination Node ID
 */
uint8_t MeshRouter::findNextHopForDestination(uint8_t dest) {
    if (routingTable == nullptr) {
        return 0;
    }

    //Search Node in existing RoutingTable
    // Suche Node in vorhandenem RoutingTable
    for (int i = 0; i < totalRoutes; i++) {
        if (routingTable[i]->nodeId == dest) {
            return routingTable[i]->hop;
        }
    }
    return 0;
}

/* Sends the Broadcast packet as fragments and anticipate the packet sending and waiting time.
 * Input parameters: packet, source, size of packet, packet ID, hash
 */
void MeshRouter::CreateBroadcastPacket(uint8_t *payload, uint8_t source, uint16_t size, uint16_t id, int64_t hash) {
    // Announce Transmission with Header Packet
    auto *headerLoraPacket = (FloodBroadcastHeaderPacket_t *) malloc(sizeof(FloodBroadcastHeaderPacket_t));
    headerLoraPacket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_HEADER;
    headerLoraPacket->lastHop = NodeID;
    headerLoraPacket->id = id;
    headerLoraPacket->source = source;
    headerLoraPacket->size = size;

    // Very simple Content Checksum
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < size; i++) {
        checksum += payload[i];
    }
    headerLoraPacket->checksum = checksum;

    LinkedList<QueuedPacket_t *> packetQueue;
    QueuePacket(&packetQueue, (uint8_t *) headerLoraPacket, sizeof(FloodBroadcastHeaderPacket_t), source, id, 150, hash);

    uint8_t fragment = 0;

    // Send Payload as Fragments
    for (uint16_t i = 0; i < size;) {
        auto fragPacket = (FloodBroadcastFragmentPacket_t *) malloc(sizeof(FloodBroadcastFragmentPacket_t));
        fragPacket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT;
        fragPacket->id = headerLoraPacket->id;
        fragPacket->fragment = fragment++;

        uint16_t bytesToCopy =
                sizeof(fragPacket->payload) > size - i ? size - i
                                                      : sizeof(fragPacket->payload);
        memcpy(&fragPacket->payload, payload + i, bytesToCopy);
        i += bytesToCopy;

        if (i == size) {
            // Predict packet send time 
            long fullWaitTime = predictPacketSendTime(255);
            //QueuePacket(&packetQueue, (uint8_t *) fragPacket, sizeof(FloodBroadcastFragmentPacket_t),source, id,
            //           200 + random(fullWaitTime, fullWaitTime + 250),hash);
            // After all fragments are acquired,  the list of fragments is added to the sending queue
            QueuePacket(&packetQueue, (uint8_t *) fragPacket, sizeof(FloodBroadcastFragmentPacket_t),source, id,
                       50 + (fullWaitTime + (rand() % 51)),hash);
        } else {
            QueuePacket(&packetQueue, (uint8_t *) fragPacket, sizeof(FloodBroadcastFragmentPacket_t),source, id, 0, hash);
        }
    }

    //while(packetQueue.size() > 0){
    //    sendQueue.add(packetQueue.shift());
    //}

    AddToSendQueueReplaceSameHashedPackets(&packetQueue, hash, source);
}

/**
 * This Method Checks for Messages of the Same Type, from the Same Host and Replaces them with the new Packets at the Same Position in the SendQueue.
 * This Prevents the Queue to fill up with already outdated Messages.
 * When no hash is provided or no Message is found, the packets are regularly queued to the sendQueue.
 * Input parameters: list of new packets, hash value, message source
 */
void MeshRouter::AddToSendQueueReplaceSameHashedPackets(LinkedList<QueuedPacket_t *> *newPackts, int64_t hash, uint8_t messageSource) {
    // Default: No Type Provided, just add to SendQueue
    if(hash == 0){
        while(newPackts->size() > 0){
            sendQueue.add(newPackts->shift());
        }
        return;
    }

    bool hasPacketToReplace = false;
    for(int k=0;k<sendQueue.size();k++){
        QueuedPacket_t *queuedPacket = sendQueue.get(k);
        if(queuedPacket->hash == hash && queuedPacket->packetPointer[0] == MESSAGE_TYPE_FLOOD_BROADCAST_HEADER && queuedPacket->source == messageSource){

            hasPacketToReplace = true;
            break;
        }
    }

    // No Packets found to replace!
    if(!hasPacketToReplace){
        while(newPackts->size() > 0){
            sendQueue.add(newPackts->shift());
        }
        return;
    }

    LinkedList<QueuedPacket_t *> preReplaceList;
    // We have a Message to replace!
    // shift all packets infront of this message to another temporary list, then remove the Message's Packets, add the new Packets, and add the packets of the temporary list
    bool foundFirstPacket = false;
    int shiftedStuff = 0;
    while(!foundFirstPacket && sendQueue.size() > 1){
        auto shiftedPacket = sendQueue.shift();

        // When Message Header in Queue, Packet is not send yet and should be replaced
        if(shiftedPacket->hash == hash && shiftedPacket->packetPointer[0] == MESSAGE_TYPE_FLOOD_BROADCAST_HEADER && shiftedPacket->source == messageSource){
            uint16_t id = shiftedPacket->id;

            auto nextPacket = sendQueue.shift();
            // Remove all Fragments belonging to this Message Header
            while(sendQueue.size() > 0 && nextPacket->source == messageSource && nextPacket->id == id){
                free(nextPacket->packetPointer);
                free(nextPacket);
                nextPacket = sendQueue.shift();
                shiftedStuff++;
            }

            // Shifted one too far
            sendQueue.unshift(nextPacket);
            //*debugString = "I" + String(id) + "S" + String((uint16_t)source ) + "i"+ String(sendQueue.get(0).id) + "s" + String((uint16_t)sendQueue.get(0).source ) + ":" + String(shiftedStuff) + ":" + String((uint16_t)sendQueue.get(0).packetPointer[0]) ;

            // Insert new Packet on same Position
            while(newPackts->size() > 0){
                sendQueue.unshift(newPackts->pop());
            }

            foundFirstPacket = true;
            free(shiftedPacket->packetPointer);
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
* This method is called when a complete serial packet has arrived over the USB interface.
* Diese Methode wird aufgerufen, wenn ein vollständiges Serielles Packet über die USB Schnittstelle eingegangen ist.
* Input parameters: serial Packet
*/
void MeshRouter::ProcessFloodSerialPacket(SerialPayloadFloodPacket_t *serialPayloadFloodPacket) {
    CreateBroadcastPacket(serialPayloadFloodPacket->payload, NodeID, serialPayloadFloodPacket->size, ID_COUNTER++, serialPayloadFloodPacket->hash);

    free(serialPayloadFloodPacket->payload);
}

/**
 * Called when the header of a pending transfer is received.
 * Wird aufgerufen, wenn der Header einer anstehenden Übertragung empfangen wird.
 * Input parameters: packet, RSSI
 */
void MeshRouter::OnFloodHeaderPacket(FloodBroadcastHeaderPacket_t *packet, int rssi) {
    if (getIncompletePacketById(packet->id, packet->source) != nullptr) {
        // Already received this Packet!
        return;
    }

    if (NODE_ID_COUNTERS[packet->source] > packet->id) {
        // We already received this packet, this is just a repetition
        return;
    }

#ifdef RETRANSMIT_PACKETS
    // Send Broadcast Confirmation
    auto *broadcastAck = (FloodBroadcastAck_t *) malloc(sizeof(FloodBroadcastAck_t));
    broadcastAck->source = packet->source;
    broadcastAck->totalTransmissionSize = packet->size;
    broadcastAck->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_ACK;
    broadcastAck->id = packet->id;

    SendRaw((uint8_t *) broadcastAck, sizeof(FloodBroadcastAck_t));

    free(broadcastAck);
    broadcastAck = nullptr;
#endif

    UpdateRSSI(packet->source, rssi);

    auto *incompletePacket = (FragmentedPacket_t *) malloc(sizeof(FragmentedPacket_t));
    incompletePacket->id = packet->id;
    incompletePacket->size = packet->size;
    incompletePacket->lastHop = packet->lastHop;
    incompletePacket->source = packet->source;
    incompletePacket->lastFragment = 0;
    incompletePacket->received = 0;
    incompletePacket->corrupted = false;
    incompletePacket->checksum = packet->checksum;

    lastBroadcastSourceId = packet->source;

    MeshRouter::UpdateRSSI(packet->lastHop, rssi);
    MeshRouter::UpdateHOP(packet->source, packet->lastHop);

    uint16_t nextFragLength = (uint16_t) packet->size > 255 ? 255 : packet->size;
    SenderWait((unsigned long) 20 + predictPacketSendTime(nextFragLength));//((unsigned long) 300 + predictPacketSendTime(nextFragLength));

    if(xPortGetFreeHeapSize() - incompletePacket->size > incompletePacket->size + 10000){
        *debugString = "FH: " + String(incompletePacket->size) + " ID: " + String(incompletePacket->id);
        incompletePacket->payload = (uint8_t *) malloc(packet->size);
        incompletePacketList.add(incompletePacket);
    }else{
        free(incompletePacket);
    }

}

/* Main program for fragment packet transmission.
* Input parameters: fragment packet
*/
void MeshRouter::OnFloodFragmentPacket(FloodBroadcastFragmentPacket_t *packet) {
    // Find fragment packet
    FragmentedPacket_t *incompletePacket = getIncompletePacketById(packet->id, lastBroadcastSourceId);
    if (incompletePacket == nullptr) {
        // No information about future packages. Assume the greatest!
        // Keine Informationen über zukünftige Pakete. Gehe vom größten aus!
        SenderWait(40 + predictPacketSendTime(255));//(400 + predictPacketSendTime(255));
        *debugString = "Fragment: ERR";
        return;
    }

    // Ignore! We are receiving a repeated Packet!
    if (packet->fragment <= incompletePacket->lastFragment && packet->fragment > 0) {
        return;
    }

    // Check for lost Fragments
    if (packet->fragment - incompletePacket->lastFragment > 1) {
        // We lost a Packet! Doesn't matter, fill out bytes and continue

        *debugString = "LOSS: " + String(packet->fragment);
        uint8_t lostFragments = packet->fragment - incompletePacket->lastFragment - 1;

        incompletePacket->received += (sizeof packet->payload) * lostFragments;

        incompletePacket->lastFragment += lostFragments;
        incompletePacket->corrupted = true;
    }

    uint16_t bytesLeft = incompletePacket->size - incompletePacket->received > (sizeof packet->payload)
                         ? (sizeof packet->payload)
                         : incompletePacket->size - incompletePacket->received;

    // Delay Transmission of Packets
    SenderWait(20 + predictPacketSendTime(bytesLeft > 255 ? 255 : bytesLeft)); //(150 + predictPacketSendTime(bytesLeft > 255 ? 255 : bytesLeft));

    // Copy packet data to buffer
    memcpy(incompletePacket->payload + incompletePacket->received, packet->payload, bytesLeft);
    incompletePacket->received += bytesLeft;

    // Set last Received fragment
    incompletePacket->lastFragment = packet->fragment;

    //*debugString = "F" + String(packet->fragment) + "R" + String(incompletePacket->received) + "S" +
    //               String(incompletePacket->size) + "L" + String(bytesLeft);

    if (incompletePacket->received == incompletePacket->size) {
        // End of Transmission

        // Very simple Content Checksum
        uint8_t checksum = 0;
        for (uint16_t i = 0; i < incompletePacket->size; i++) {
            checksum += incompletePacket->payload[i]; // Intentional Overflow, Allowed and strictly defined in c standard for unsigned values
        }

        if (checksum != incompletePacket->checksum) {
            *debugString = "Invalid CS!" + String(checksum);
            incompletePacket->corrupted = true;
        } else if (incompletePacket->corrupted) {
            *debugString = "Packet Lost!";
        } else {
            *debugString = "Success: " + String(checksum);
        }

        if (!incompletePacket->corrupted) {
            OnPacketForHost(incompletePacket);
        }
    }
}

/* Returns information about fragment of packet with matching transmission ID and source.
* Input parameters: Transmission ID, source
*/
FragmentedPacket_t *MeshRouter::getIncompletePacketById(uint16_t transmissionid, uint8_t source) {
    for (int i = 0; i < incompletePacketList.size(); i++) {
        if (incompletePacketList.get(i)->id == transmissionid && incompletePacketList.get(i)->source == source) {
            return incompletePacketList.get(i);
        }
    }
    return nullptr;
}

/* Delete information about fragment of packet with matching transmission ID and source.
* Input parameters: Transmission ID, source
*/
void MeshRouter::removeIncompletePacketById(uint16_t transmissionid, uint8_t source) {
    for (int i = 0; i < incompletePacketList.size(); i++) {
        if (incompletePacketList.get(i)->id == transmissionid && incompletePacketList.get(i)->source == source) {
            incompletePacketList.remove(i);
        }
    }
}

/*
* Handles received packet serially.
* Input parameters: packet fragment
*/
void MeshRouter::OnPacketForHost(FragmentedPacket_t *packet) {
    auto payloadBuffer = (uint8_t *) malloc(sizeof(FragmentedPacket_t) + packet->size);
    memcpy(payloadBuffer, packet, sizeof(FragmentedPacket_t));
    memcpy(payloadBuffer + sizeof(FragmentedPacket_t), packet->payload, packet->size);

    auto serialPacketHeader = (SerialPacketHeader_t *) malloc(sizeof(SerialPacketHeader_t));
    serialPacketHeader->serialPacketType = SERIAL_PACKET_TYPE_FLOOD_PACKET;
    serialPacketHeader->size = sizeof(FragmentedPacket_t) + packet->size;

#ifdef TEST_MODE
    Serial.println("[PACKET] " + String(packet->size));
#else
    {
        std::lock_guard<std::mutex> lck(serial_mtx);
        uint8_t magicBytes[] = { 0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2 };
        Serial.write(magicBytes, 8);
        Serial.write((uint8_t *) serialPacketHeader, 3);
        Serial.write(payloadBuffer, serialPacketHeader->size);
    }
#endif



#ifdef TEST_MODE
    if(millis() < 900000){
        receivedBytes += packet->size;
    }
#endif

    receivedBytes += packet->size;

    // ReQueue Packet
#ifdef RETRANSMIT_PACKETS
    CreateBroadcastPacket(packet->payload, packet->source, packet->size, packet->id);
#endif
    *debugString = "HOST SEND: " + String(packet->size);

    blockSendUntil = millis() + (10 + (rand() % 241));

    removeIncompletePacketById(packet->id, packet->source);
    free(packet->payload);
    free(packet);
    free(serialPacketHeader);
    free(payloadBuffer);
}

/* Calculates the AirTime of a packet, based on its Payload size.
 Calculation is based of the LoRa Chip Datasheet.
 SX1276-7-8-9 Datasheet-> https://semtech--c.na98.content.force.com/sfc/dist/version/download/?oid=00DE0000000JelG&ids=0682R000006TQEPQA4&d=%2Fa%2F2R0000001Rbr%2F6EfVZUorrpoKFfvaF_Fkpgp5kzjiNyiAbqcpqh9qSjE&operationContext=DELIVERY&asPdf=true&viewId=05H2R000002POsoUAG&dpt= */
double MeshRouter::getSendTimeByPacketSizeInMS(int PL) {
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


void MeshRouter::measurePacketTime(uint8_t size, unsigned long time) {
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

/* Estimate time to send a packet.
* Input parameters: size of packet
*/
long MeshRouter::predictPacketSendTime(uint8_t size) {
long time = size;
time += 20; //ms = Byte overhead
//    long time = (long) (1.0 * timePerByte * size + sendOverhead);
//    if (time < 10) {
//        time = 500;
//    }
    return time;
}

