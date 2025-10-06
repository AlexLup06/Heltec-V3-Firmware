#include "mesh/MeshRouter.h"

/**
 * Incoming Packets are Processed here
 **/

std::mutex serial_mtx;
std::mutex *getSerialMutex() { return &serial_mtx; }

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

/* Change LoRa module configuration.
 * Input parameters: spreading factor, transmission power, frequency, bandwidth
 */
void MeshRouter::applyModemConfig(uint8_t spreading_factor, uint8_t transmission_power, uint32_t frequency, uint32_t bandwidth)
{
    reInitRadio(frequency, spreading_factor, transmission_power, bandwidth);
}

// Initialize node by setting Node ID from MAC address.
void MeshRouter::init()
{
    // Read MAC Adress
    esp_read_mac(macAdress, ESP_MAC_WIFI_STA);

    // 255 = no assigned ID
    NodeID = macAdress[5];

    // Announce Local NodeID to the Network to
    MeshRouter::announceNodeId(1);

    for (int i = 0; i < 255; i++)
    {
        NODE_ID_COUNTERS[i] = 0;
    }
}

void MeshRouter::finish()
{
}

/**
 * Main Loop: Tasks performed based on operating-mode. This loop runs endlessly.
 *
 */
void MeshRouter::handle()
{
    // Modem has preable receiving
    // Modem hat preable Empfangen

    if (cad)
    {
        SenderWait(0);
        // Increase blocking time of other senders.
        preambleAdd = 1000 + predictPacketSendTime(255);
        cad = false;
    }

    switch (OPERATING_MODE)
    {
    case OPERATING_MODE_INIT:
        // Initialize LoRa module.
        // If already initialized, put LoRa module in continuous receive mode.
        startReceive();
        OPERATING_MODE = OPERATING_MODE_RECEIVE;
        break;
    case OPERATING_MODE_SEND:
        // ToDo: Process Packet Queue

        break;
    case OPERATING_MODE_RECEIVE:
        // LoRa module in receive mode
        if (receiveState == RECEIVE_STATE_PACKET_READY)
        {
            radio.standby();

            size_t len = radio.getPacketLength();
            uint8_t *receiveBuffer = (uint8_t *)malloc(len);
            if (!receiveBuffer)
            {
                Serial.println("ERROR: malloc failed!");
                return;
            }

            int state = radio.readData(receiveBuffer, len);
            float rssi = radio.getRSSI();

            if (state == RADIOLIB_ERR_NONE)
            {
                OnReceivePacket(receiveBuffer[0], receiveBuffer, len, rssi);
            }
            else
            {
                Serial.printf("Receive failed: %d\n", state);
            }

            receiveState = RECEIVE_STATE_IDLE;
            radio.startReceive();
        }
        // Send packet from the sendqueue.
        ProcessQueue();

#ifdef ANNOUNCE_IN_INTERVAL
        if (millis() - lastAnnounceTime > 5000)
        {
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
void MeshRouter::onReceiveIR()
{
    receiveState = RECEIVE_STATE_PACKET_READY;
}

void MeshRouter::onPreambleDetectedIR()
{
    cad = true;
}
void MeshRouter::onCRCerrorIR()
{
    Serial.println("CRC error (packet corrupted)");
}

/* The function reads the data from the SRAM and sets the preambleAdd to zero, because data was received. The evaluation of the data starts with the first byte which indicates the message type.
 * Input parameters: message type, packet, packet size, rssi
 */
void MeshRouter::OnReceivePacket(uint8_t messageType, uint8_t *rawPacket, uint8_t packetSize, int rssi)
{
    // remove preamble wait value
    preambleAdd = 0;
    switch (messageType)
    {
    case MESSAGE_TYPE_FLOOD_BROADCAST_HEADER:
        MeshRouter::OnFloodHeaderPacket((FloodBroadcastHeaderPacket_t *)rawPacket, rssi);
        SenderWait(0 + (rand() % 101)); //(random(0, 100));
        break;
    case MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT:
        MeshRouter::OnFloodFragmentPacket((FloodBroadcastFragmentPacket_t *)rawPacket);
        SenderWait(0 + (rand() % 101)); //(random(0, 100));
        break;
    case MESSAGE_TYPE_NODE_ANNOUNCE:
        MeshRouter::OnNodeIdAnnouncePacket((NodeIdAnnounce_t *)rawPacket, rssi);
        break;
    case MESSAGE_TYPE_FLOOD_BROADCAST_ACK:
        MeshRouter::OnFloodBroadcastAck((FloodBroadcastAck_t *)rawPacket, rssi);
        break;
    default:
        break;
    }

    // Packet Processed - Free Ram
    free(rawPacket);
}

/* Announces the Node ID to the Network
 * Input parameters: packet, RSSI value
 */
void MeshRouter::OnNodeIdAnnouncePacket(NodeIdAnnounce_t *packet, int rssi)
{
    if (NodeID != packet->nodeId)
    {

        if (packet->respond == 1)
        {
            // Unknown node! Block Sending for a time window, to allow other Nodes to respond.
            MeshRouter::SenderWait(0 + (rand() % 451)); // (random(0, 450));
            MeshRouter::announceNodeId(0);
        }
    }
    else
    {
        // ToDo: Route Dispute
    }

    // Node just started, reset NodeID Counter
    NODE_ID_COUNTERS[packet->nodeId] = 0;
}

/* Sets all nodes which are not the sender into recieving mode to prevent collisions on the media for long packet transmissions.
 * Input parameters: packet, RSSI value
 */
void MeshRouter::OnFloodBroadcastAck(FloodBroadcastAck_t *packet, int rssi)
{
    // We have announced this as a Broadcast, so we expected this. Dont do anything.
    if (packet->source == NodeID)
    {
        return;
    }

    // We are not the Sender of the Broadcast.

    if (getIncompletePacketById(packet->id, packet->source) == nullptr)
    {
        // We have not received the Broadcast Announcement (FloodBroadcastHeaderPacket_t). We need to sleep now for the duration of the Transmission.
        int fragments = packet->totalTransmissionSize / 251 + 1;
        SenderWait(predictPacketSendTime(packet->totalTransmissionSize) + fragments * 15);
    }
    else
    {
        // Just dont Send until next Fragment arrives
        SenderWait(predictPacketSendTime(255)); //(predictPacketSendTime(255) + 50);
    }
}

/**
 * Announces the current Node ID to the Network
 * Input parameters: packet respond variable
 */
void MeshRouter::announceNodeId(uint8_t respond)
{
    // Create Packet
    auto *packet = (NodeIdAnnounce_t *)malloc(sizeof(NodeIdAnnounce_t));
    lastAnnounceTime = millis();

    packet->messageType = MESSAGE_TYPE_NODE_ANNOUNCE;
    packet->nodeId = NodeID;
    packet->lastHop = NodeID;
    packet->respond = respond;
    memcpy(packet->deviceMac, macAdress, 6);

    char macHexStr[18] = {0};
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", packet->deviceMac[0], packet->deviceMac[1], packet->deviceMac[2],
            packet->deviceMac[3], packet->deviceMac[4], packet->deviceMac[5]);

    // Queue the packet into the sendQueue
    QueuePacket(&sendQueue, (uint8_t *)packet, sizeof(NodeIdAnnounce_t), NodeID, 0);
}

/**
 * Directly sends Packet with Hardware, you should never use this directly, because this will not wait for the Receive window of other Devices
 * Input parameters: rawPacket , size
 */
void MeshRouter::SendRaw(uint8_t *rawPacket, uint8_t size)
{
    unsigned long startTime = millis();

    sendPacket(rawPacket, size);

    packetTime = millis() - startTime;
}

/**
 * Delays the Next packet in the Queue for a specific time
 * Input parameters: delay time
 */
void MeshRouter::SenderWait(unsigned long waitTime)
{
    if (blockSendUntil < millis() + waitTime)
    {
        blockSendUntil = millis() + waitTime;
    }
}

/* Adds the packet to the sendQueue.
 * Input parameters: linked list of pointers to packets, packet to be added to queue, size of packet, source of packet, packet ID, wait time, hash value
 */
void MeshRouter::QueuePacket(LinkedList<QueuedPacket_t *> *listPointer, uint8_t *rawPacket, uint8_t size, uint8_t source, uint16_t id, long waitTimeAfter, int64_t hash)
{
    auto *packet = (QueuedPacket_t *)malloc(sizeof(QueuedPacket_t));
    packet->packetPointer = rawPacket;
    packet->packetSize = size;
    packet->waitTimeAfter = waitTimeAfter;
    packet->hash = hash;
    packet->source = source;
    packet->id = id;

    listPointer->add(packet);

    if (displayQueueLength != nullptr)
    {
        *displayQueueLength = sendQueue.size();
    }
}

// Send packet only if the queue is not empty and the blocking time is less than the current time measured in milli seconds. Other senders need to wait until the package is sent.
void MeshRouter::ProcessQueue()
{
    if (sendQueue.size() == 0 || blockSendUntil + preambleAdd > millis())
    {
        return;
    }
    QueuedPacket_t *packetQueueEntry = sendQueue.shift();

    if (packetQueueEntry->packetPointer == nullptr)
    {
        return;
    }

    MeshRouter::SendRaw(packetQueueEntry->packetPointer,
                        packetQueueEntry->packetSize);

    // Wait for ACK of Receiving Nodes before sending another packet.
    if (packetQueueEntry->waitTimeAfter > 0)
    {
        SenderWait(packetQueueEntry->waitTimeAfter);
    }

    // Free sent packet queue pointer
    free(packetQueueEntry->packetPointer);
    free(packetQueueEntry);
    packetQueueEntry->packetPointer = nullptr;

    // Update Queue length on Display
    if (displayQueueLength != nullptr)
    {
        *displayQueueLength = sendQueue.size();
    }
}

/* Sends the Broadcast packet as fragments and anticipate the packet sending and waiting time.
 * Wird nur von ProcessFloodSerialPacket(...) aufgerufen

 * Input parameters: packet, source, size of packet, packet ID, hash
 */
void MeshRouter::CreateBroadcastPacket(uint8_t *payload, uint8_t source, uint16_t size, uint16_t id, int64_t hash)
{
    // Announce Transmission with Header Packet
    auto *headerLoraPacket = (FloodBroadcastHeaderPacket_t *)malloc(sizeof(FloodBroadcastHeaderPacket_t));
    headerLoraPacket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_HEADER;
    headerLoraPacket->lastHop = NodeID;
    headerLoraPacket->id = id;
    headerLoraPacket->source = source;
    headerLoraPacket->size = size;

    // Very simple Content Checksum
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < size; i++)
    {
        checksum += payload[i];
    }
    headerLoraPacket->checksum = checksum;

    LinkedList<QueuedPacket_t *> packetQueue;
    QueuePacket(&packetQueue, (uint8_t *)headerLoraPacket, sizeof(FloodBroadcastHeaderPacket_t), source, id, 150, hash);

    uint8_t fragment = 0;

    // Send Payload as Fragments
    for (uint16_t i = 0; i < size;)
    {
        auto fragPacket = (FloodBroadcastFragmentPacket_t *)malloc(sizeof(FloodBroadcastFragmentPacket_t));
        fragPacket->messageType = MESSAGE_TYPE_FLOOD_BROADCAST_FRAGMENT;
        fragPacket->id = headerLoraPacket->id;
        fragPacket->fragment = fragment++;

        uint16_t bytesToCopy =
            sizeof(fragPacket->payload) > size - i ? size - i
                                                   : sizeof(fragPacket->payload);
        memcpy(&fragPacket->payload, payload + i, bytesToCopy);
        i += bytesToCopy;

        if (i == size)
        {
            // Predict packet send time
            long fullWaitTime = predictPacketSendTime(255);
            // QueuePacket(&packetQueue, (uint8_t *) fragPacket, sizeof(FloodBroadcastFragmentPacket_t),source, id,
            //            200 + random(fullWaitTime, fullWaitTime + 250),hash);
            //  After all fragments are acquired,  the list of fragments is added to the sending queue
            QueuePacket(&packetQueue, (uint8_t *)fragPacket, sizeof(FloodBroadcastFragmentPacket_t), source, id,
                        50 + (fullWaitTime + (rand() % 51)), hash);
        }
        else
        {
            QueuePacket(&packetQueue, (uint8_t *)fragPacket, sizeof(FloodBroadcastFragmentPacket_t), source, id, 0, hash);
        }
    }

    // while(packetQueue.size() > 0){
    //     sendQueue.add(packetQueue.shift());
    // }

    AddToSendQueueReplaceSameHashedPackets(&packetQueue, hash, source);
}

/**
 * This Method Checks for Messages of the Same Type, from the Same Host and Replaces them with the new Packets at the Same Position in the SendQueue.
 * This Prevents the Queue to fill up with already outdated Messages.
 * When no hash is provided or no Message is found, the packets are regularly queued to the sendQueue.
 * Wird nur von CreateBroadcastPacket(...) aufgerufen

 * Input parameters: list of new packets, hash value, message source
 */
void MeshRouter::AddToSendQueueReplaceSameHashedPackets(LinkedList<QueuedPacket_t *> *newPackts, int64_t hash, uint8_t messageSource)
{
    // Default: No Type Provided, just add to SendQueue
    if (hash == 0)
    {
        while (newPackts->size() > 0)
        {
            sendQueue.add(newPackts->shift());
        }
        return;
    }

    bool hasPacketToReplace = false;
    for (int k = 0; k < sendQueue.size(); k++)
    {
        QueuedPacket_t *queuedPacket = sendQueue.get(k);
        if (queuedPacket->hash == hash && queuedPacket->packetPointer[0] == MESSAGE_TYPE_FLOOD_BROADCAST_HEADER && queuedPacket->source == messageSource)
        {

            hasPacketToReplace = true;
            break;
        }
    }

    // No Packets found to replace!
    if (!hasPacketToReplace)
    {
        while (newPackts->size() > 0)
        {
            sendQueue.add(newPackts->shift());
        }
        return;
    }

    LinkedList<QueuedPacket_t *> preReplaceList;
    // We have a Message to replace!
    // shift all packets infront of this message to another temporary list, then remove the Message's Packets, add the new Packets, and add the packets of the temporary list
    bool foundFirstPacket = false;
    int shiftedStuff = 0;
    while (!foundFirstPacket && sendQueue.size() > 1)
    {
        auto shiftedPacket = sendQueue.shift();

        // When Message Header in Queue, Packet is not send yet and should be replaced
        if (shiftedPacket->hash == hash && shiftedPacket->packetPointer[0] == MESSAGE_TYPE_FLOOD_BROADCAST_HEADER && shiftedPacket->source == messageSource)
        {
            uint16_t id = shiftedPacket->id;

            auto nextPacket = sendQueue.shift();
            // Remove all Fragments belonging to this Message Header
            while (sendQueue.size() > 0 && nextPacket->source == messageSource && nextPacket->id == id)
            {
                free(nextPacket->packetPointer);
                free(nextPacket);
                nextPacket = sendQueue.shift();
                shiftedStuff++;
            }

            // Shifted one too far
            sendQueue.unshift(nextPacket);
            //*debugString = "I" + String(id) + "S" + String((uint16_t)source ) + "i"+ String(sendQueue.get(0).id) + "s" + String((uint16_t)sendQueue.get(0).source ) + ":" + String(shiftedStuff) + ":" + String((uint16_t)sendQueue.get(0).packetPointer[0]) ;

            // Insert new Packet on same Position
            while (newPackts->size() > 0)
            {
                sendQueue.unshift(newPackts->pop());
            }

            foundFirstPacket = true;
            free(shiftedPacket->packetPointer);
            free(shiftedPacket);
        }
        else
        {
            // We havent found a matching packet yet, remove the first packet from the sendQueue and add it to the preReplaceList
            preReplaceList.add(shiftedPacket);
        }
    }

    // add the previously shifted packets to the front again
    while (preReplaceList.size() > 0)
    {
        sendQueue.unshift(preReplaceList.pop());
    }
    while (newPackts->size() > 0)
    {
        sendQueue.add(newPackts->shift());
    }
}

/**
 * This method is called when a complete serial packet has arrived over the USB interface.
 * Diese Methode wird aufgerufen, wenn ein vollständiges Serielles Packet über die USB Schnittstelle eingegangen ist.
 * Aus main.cpp aufgerufen wenn HostHandler (Prozess 1) ein Paket von Host bekommen hat
 *
 * Input parameters: serial Packet
 */
void MeshRouter::ProcessFloodSerialPacket(SerialPayloadFloodPacket_t *serialPayloadFloodPacket)
{
    CreateBroadcastPacket(serialPayloadFloodPacket->payload, NodeID, serialPayloadFloodPacket->size, ID_COUNTER++, serialPayloadFloodPacket->hash);

    free(serialPayloadFloodPacket->payload);
}

/**
 * Called when the header of a pending transfer is received.
 * Wird aufgerufen, wenn der Header einer anstehenden Übertragung empfangen wird.

 * Input parameters: packet, RSSI
 */
void MeshRouter::OnFloodHeaderPacket(FloodBroadcastHeaderPacket_t *packet, int rssi)
{
    if (getIncompletePacketById(packet->id, packet->source) != nullptr)
    {
        // Already received this Packet!
        return;
    }

    if (NODE_ID_COUNTERS[packet->source] > packet->id)
    {
        // We already received this packet, this is just a repetition
        return;
    }
    auto *incompletePacket = (FragmentedPacket_t *)malloc(sizeof(FragmentedPacket_t));
    incompletePacket->id = packet->id;
    incompletePacket->size = packet->size;
    incompletePacket->lastHop = packet->lastHop;
    incompletePacket->source = packet->source;
    incompletePacket->lastFragment = 0;
    incompletePacket->received = 0;
    incompletePacket->corrupted = false;
    incompletePacket->checksum = packet->checksum;

    lastBroadcastSourceId = packet->source;

    uint16_t nextFragLength = (uint16_t)packet->size > 255 ? 255 : packet->size;
    SenderWait((unsigned long)20 + predictPacketSendTime(nextFragLength)); //((unsigned long) 300 + predictPacketSendTime(nextFragLength));

    if (xPortGetFreeHeapSize() - incompletePacket->size > incompletePacket->size + 10000)
    {
        *debugString = "FH: " + String(incompletePacket->size) + " ID: " + String(incompletePacket->id);
        incompletePacket->payload = (uint8_t *)malloc(packet->size);
        incompletePacketList.add(incompletePacket);
    }
    else
    {
        free(incompletePacket);
    }
}

/* Main program for fragment packet transmission.
 * Input parameters: fragment packet
 */
void MeshRouter::OnFloodFragmentPacket(FloodBroadcastFragmentPacket_t *packet)
{
    // Find fragment packet
    FragmentedPacket_t *incompletePacket = getIncompletePacketById(packet->id, lastBroadcastSourceId);
    if (incompletePacket == nullptr)
    {
        // No information about future packages. Assume the greatest!
        // Keine Informationen über zukünftige Pakete. Gehe vom größten aus!
        SenderWait(40 + predictPacketSendTime(255)); //(400 + predictPacketSendTime(255));
        *debugString = "Fragment: ERR";
        return;
    }

    // Ignore! We are receiving a repeated Packet!
    if (packet->fragment <= incompletePacket->lastFragment && packet->fragment > 0)
    {
        return;
    }

    // Check for lost Fragments
    if (packet->fragment - incompletePacket->lastFragment > 1)
    {
        // We lost a Packet! Doesn't matter, fill out bytes and continue

        *debugString = "LOSS: " + String(packet->fragment);

        // BUG??? wrong number; Falls das vorletzte packet veloren wurde, dann wird mit der Anzahl and bytes von dem
        // letzten packet aufgefüllt. Das wären nicht 251 wie es sein sollten => memory leak. Speicher für das Packet wird nie befreit
        uint8_t lostFragments = packet->fragment - incompletePacket->lastFragment - 1;

        incompletePacket->received += (sizeof packet->payload) * lostFragments;

        incompletePacket->lastFragment += lostFragments;
        incompletePacket->corrupted = true;
    }

    // BUG??? was wird hier überhaupt berechnet?? OK das ist einfach viel zu kompliziert dargestellt: bytesReceived
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

    if (incompletePacket->received == incompletePacket->size)
    {
        // End of Transmission

        // Very simple Content Checksum
        uint8_t checksum = 0;
        for (uint16_t i = 0; i < incompletePacket->size; i++)
        {
            checksum += incompletePacket->payload[i]; // Intentional Overflow, Allowed and strictly defined in c standard for unsigned values
        }

        if (checksum != incompletePacket->checksum)
        {
            *debugString = "Invalid CS!" + String(checksum);
            incompletePacket->corrupted = true;
        }
        else if (incompletePacket->corrupted)
        {
            *debugString = "Packet Lost!";
        }
        else
        {
            *debugString = "Success: " + String(checksum);
        }

        if (!incompletePacket->corrupted)
        {
            OnPacketForHost(incompletePacket);
        }
    }
}

/* Returns information about fragment of packet with matching transmission ID and source.
 * Input parameters: Transmission ID, source
 */
FragmentedPacket_t *MeshRouter::getIncompletePacketById(uint16_t transmissionid, uint8_t source)
{
    for (int i = 0; i < incompletePacketList.size(); i++)
    {
        if (incompletePacketList.get(i)->id == transmissionid && incompletePacketList.get(i)->source == source)
        {
            return incompletePacketList.get(i);
        }
    }
    return nullptr;
}

/* Delete information about fragment of packet with matching transmission ID and source.
 * Input parameters: Transmission ID, source
 BUG: Es gibt einen Memory leak. Wenn das packet corrupted ist, dann wird das packet nie gelöscht
 */
void MeshRouter::removeIncompletePacketById(uint16_t transmissionid, uint8_t source)
{
    for (int i = 0; i < incompletePacketList.size(); i++)
    {
        if (incompletePacketList.get(i)->id == transmissionid && incompletePacketList.get(i)->source == source)
        {
            incompletePacketList.remove(i);
        }
    }
}

/*
 * Handles received packet serially.
 * Input parameters: packet fragment
 */
void MeshRouter::OnPacketForHost(FragmentedPacket_t *packet)
{
    auto payloadBuffer = (uint8_t *)malloc(sizeof(FragmentedPacket_t) + packet->size);
    memcpy(payloadBuffer, packet, sizeof(FragmentedPacket_t));
    memcpy(payloadBuffer + sizeof(FragmentedPacket_t), packet->payload, packet->size);

    auto serialPacketHeader = (SerialPacketHeader_t *)malloc(sizeof(SerialPacketHeader_t));
    serialPacketHeader->serialPacketType = SERIAL_PACKET_TYPE_FLOOD_PACKET;
    serialPacketHeader->size = sizeof(FragmentedPacket_t) + packet->size;

    std::lock_guard<std::mutex> lck(serial_mtx);
    uint8_t magicBytes[] = {0xF0, 0x4L, 0x11, 0x9B, 0x39, 0xBC, 0xE4, 0xD2};
    Serial.write(magicBytes, 8);
    Serial.write((uint8_t *)serialPacketHeader, 3);
    Serial.write(payloadBuffer, serialPacketHeader->size);

    receivedBytes += packet->size;

    *debugString = "HOST SEND: " + String(packet->size);

    blockSendUntil = millis() + (10 + (rand() % 241));

    removeIncompletePacketById(packet->id, packet->source);
    free(packet->payload);
    free(packet);
    free(serialPacketHeader);
    free(payloadBuffer);
}

unsigned long timeMap[256];
uint8_t totalTimeMapEntries = 0;

/* Estimate time to send a packet.
 * Input parameters: size of packet
 */
long MeshRouter::predictPacketSendTime(uint8_t size)
{
    long time = size;
    time += 20; // ms = Byte overhead
    return time;
}
