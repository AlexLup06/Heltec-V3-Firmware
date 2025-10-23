#include "MeshRouter.h"

std::mutex serial_mtx;
std::mutex *getSerialMutex() { return &serial_mtx; }

uint8_t MeshRouter::setNodeID(uint8_t newNodeID)
{
    nodeId = newNodeID;

    MeshRouter::announceNodeId(1);
    return nodeId;
}

// Change LoRa module configuration.
void MeshRouter::applyModemConfig(uint8_t spreading_factor, uint8_t transmission_power, uint32_t frequency, uint32_t bandwidth)
{
    reInitRadio(frequency, spreading_factor, transmission_power, bandwidth);
}

// Initialize node by setting Node ID from MAC address.
void MeshRouter::init()
{

    // Announce Local NodeID to the Network to
    MeshRouter::announceNodeId(1);

    for (int i = 0; i < 255; i++)
    {
        NODE_ID_COUNTERS[i] = 0;
    }
}

void MeshRouter::clearMacData()
{
    // TODO: clear all data structures
}

String MeshRouter::getProtocolName()
{
    return "meshrouter";
}

void MeshRouter::handleWithFSM()
{
    ProcessQueue();

    if (millis() - lastAnnounceTime > 5000)
    {
        MeshRouter::announceNodeId(0);
    }
}

void MeshRouter::onPreambleDetectedIR()
{
    SenderWait(0);
    // Increase blocking time of other senders.
    preambleAdd = 1000 + predictPacketSendTime(255);
}
void MeshRouter::onCRCerrorIR()
{
    Serial.println("CRC error (packet corrupted)");
}

void MeshRouter::handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const uint8_t packetSize, const int rssi)
{
    // remove preamble wait value
    preambleAdd = 0;
    switch (messageType)
    {
    case MESSAGE_TYPE_BROADCAST_RTS:
        MeshRouter::OnFloodHeaderPacket((BroadcastRTSPacket_t *)packet, rssi);
        SenderWait(0 + (rand() % 101));
        break;
    case MESSAGE_TYPE_BROADCAST_FRAGMENT:
        MeshRouter::OnFloodFragmentPacket((BroadcastFragmentPacket_t *)packet);
        SenderWait(0 + (rand() % 101));
        break;
    case MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE:
        MeshRouter::OnNodeIdAnnouncePacket((NodeIdAnnounce_t *)packet, rssi);
        break;
    default:
        break;
    }
}

/* Announces the Node ID to the Network
 * Input parameters: packet, RSSI value
 */
void MeshRouter::OnNodeIdAnnouncePacket(NodeIdAnnounce_t *packet, int rssi)
{
    if (nodeId != packet->nodeId)
    {

        if (packet->respond == 1)
        {
            // Unknown node! Block Sending for a time window, to allow other Nodes to respond.
            MeshRouter::SenderWait(0 + (rand() % 451)); // (random(0, 450));
            MeshRouter::announceNodeId(0);
        }
    }

    // Node just started, reset NodeID Counter
    NODE_ID_COUNTERS[packet->nodeId] = 0;
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

    packet->messageType = MESSAGE_TYPE_BROADCAST_NODE_ANNOUNCE;
    packet->nodeId = nodeId;
    packet->lastHop = nodeId;
    packet->respond = respond;
    memcpy(packet->deviceMac, macAdress, 6);

    char macHexStr[18] = {0};
    sprintf(macHexStr, "%02X:%02X:%02X:%02X:%02X:%02X", packet->deviceMac[0], packet->deviceMac[1], packet->deviceMac[2],
            packet->deviceMac[3], packet->deviceMac[4], packet->deviceMac[5]);

    // Queue the packet into the sendQueue
    // TODO: always place in front and replace with old node announce
    QueuePacket(&sendQueue, (uint8_t *)packet, sizeof(NodeIdAnnounce_t), nodeId, false, false, 0);
}

/**
 * Directly sends Packet with Hardware, you should never use this directly, because this will not wait for the Receive window of other Devices
 * Input parameters: rawPacket , size
 */
void MeshRouter::sendRaw(const uint8_t *rawPacket, const uint8_t size)
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
void MeshRouter::QueuePacket(LinkedList<QueuedPacket_t *> *listPointer, uint8_t *rawPacket, uint8_t size, uint8_t source, uint16_t id, bool isHeader, bool isMission, long waitTimeAfter)
{
    auto *packet = (QueuedPacket_t *)malloc(sizeof(QueuedPacket_t));
    packet->packetPointer = rawPacket;
    packet->packetSize = size;
    packet->waitTimeAfter = waitTimeAfter;
    packet->source = source;
    packet->id = id;
    packet->isMission = isMission;
    packet->isHeader = isHeader;

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

    sendRaw(packetQueueEntry->packetPointer, packetQueueEntry->packetSize);

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
void MeshRouter::CreateBroadcastPacket(uint8_t *payload, uint8_t source, uint16_t size, uint16_t id, bool isMission)
{
    // Announce Transmission with Header Packet
    auto *headerLoraPacket = (BroadcastRTSPacket_t *)malloc(sizeof(BroadcastRTSPacket_t));
    headerLoraPacket->messageType = MESSAGE_TYPE_BROADCAST_RTS;
    headerLoraPacket->lastHop = nodeId;
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
    QueuePacket(&packetQueue, (uint8_t *)headerLoraPacket, sizeof(BroadcastRTSPacket_t), source, id, true, isMission, 150);

    uint8_t fragment = 0;

    // Send Payload as Fragments
    for (uint16_t i = 0; i < size;)
    {
        auto fragPacket = (BroadcastFragmentPacket_t *)malloc(sizeof(BroadcastFragmentPacket_t));
        fragPacket->messageType = MESSAGE_TYPE_BROADCAST_FRAGMENT;
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
            //  After all fragments are acquired,  the list of fragments is added to the sending queue
            QueuePacket(&packetQueue, (uint8_t *)fragPacket, sizeof(BroadcastFragmentPacket_t), source, id, false, isMission, 50 + (fullWaitTime + (rand() % 51)));
        }
        else
        {
            QueuePacket(&packetQueue, (uint8_t *)fragPacket, sizeof(BroadcastFragmentPacket_t), source, id, false, isMission, 0);
        }
    }

    AddToSendQueue(&packetQueue, isMission);
}

void MeshRouter::AddToSendQueue(LinkedList<QueuedPacket_t *> *newPackets, bool isMission)
{
    // --- Case 1: Mission message ---
    if (isMission)
    {
        while (newPackets->size() > 0)
        {
            sendQueue.add(newPackets->shift());
        }
        return;
    }

    // --- Case 2: Neighbour message ---
    int headerIndex = -1;

    // Find the first neighbour header in queue
    for (int i = 0; i < sendQueue.size(); i++)
    {
        QueuedPacket_t *pkt = sendQueue.get(i);
        if (!pkt->isMission && pkt->isHeader)
        {
            headerIndex = i;
            break;
        }
    }

    // No neighbour header found → just append new packets
    if (headerIndex == -1)
    {
        while (newPackets->size() > 0)
        {
            sendQueue.add(newPackets->shift());
        }
        return;
    }

    // --- Found a neighbour header: remove it + all neighbour packets after it ---
    // We'll delete them properly to avoid memory leaks
    while (headerIndex < sendQueue.size())
    {
        QueuedPacket_t *pkt = sendQueue.get(headerIndex);
        if (pkt->isMission)
            break; // stop when reaching a non-neighbour packet

        // remove & free neighbour packet
        sendQueue.remove(headerIndex);
        free(pkt->packetPointer);
        free(pkt);
    }

    // --- Insert new neighbour packets starting at that position ---
    // (first element in newPackets is header)
    for (int i = 0; i < newPackets->size(); i++)
    {
        sendQueue.add(headerIndex + i, newPackets->get(i));
    }

    // Now newPackets elements are part of sendQueue; clear pointer list to avoid double frees
    newPackets->clear();
}

/**
 * This method is called when a complete serial packet has arrived over the USB interface.
 * Diese Methode wird aufgerufen, wenn ein vollständiges Serielles Packet über die USB Schnittstelle eingegangen ist.
 * Aus main.cpp aufgerufen wenn HostHandler (Prozess 1) ein Paket von Host bekommen hat
 *
 * Input parameters: serial Packet
 */
void MeshRouter::handleUpperPacket(MessageToSend_t *serialPayloadFloodPacket)
{
    CreateBroadcastPacket(serialPayloadFloodPacket->payload, nodeId, serialPayloadFloodPacket->size, ID_COUNTER++, serialPayloadFloodPacket->isMission);

    free(serialPayloadFloodPacket->payload);
}

/**
 * Called when the header of a pending transfer is received.
 * Wird aufgerufen, wenn der Header einer anstehenden Übertragung empfangen wird.

 * Input parameters: packet, RSSI
 */
void MeshRouter::OnFloodHeaderPacket(BroadcastRTSPacket_t *packet, int rssi)
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
    auto *incompletePacket = (FragmentedPacketMeshRouter_t *)malloc(sizeof(FragmentedPacketMeshRouter_t));
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
void MeshRouter::OnFloodFragmentPacket(BroadcastFragmentPacket_t *packet)
{
    // Find fragment packet
    FragmentedPacketMeshRouter_t *incompletePacket = getIncompletePacketById(packet->id, lastBroadcastSourceId);
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
FragmentedPacketMeshRouter_t *MeshRouter::getIncompletePacketById(uint16_t transmissionid, uint8_t source)
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
void MeshRouter::OnPacketForHost(FragmentedPacketMeshRouter_t *packet)
{
    auto payloadBuffer = (uint8_t *)malloc(sizeof(FragmentedPacketMeshRouter_t) + packet->size);
    memcpy(payloadBuffer, packet, sizeof(FragmentedPacketMeshRouter_t));
    memcpy(payloadBuffer + sizeof(FragmentedPacketMeshRouter_t), packet->payload, packet->size);

    auto serialPacketHeader = (SerialPacketHeader_t *)malloc(sizeof(SerialPacketHeader_t));
    serialPacketHeader->serialPacketType = SERIAL_PACKET_TYPE_FLOOD_PACKET;
    serialPacketHeader->size = sizeof(FragmentedPacketMeshRouter_t) + packet->size;

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

long MeshRouter::predictPacketSendTime(uint8_t size)
{
    long time = size;
    time += 20; // ms = Byte overhead
    return time;
}
