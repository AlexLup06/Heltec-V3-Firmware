#include "MeshRouter.h"

const char *MeshRouter::getProtocolName()
{
    return "meshrouter";
}

void MeshRouter::finishProtocol()
{
    blockSendUntil = 0;
    preambleAdd = 0;
}

void MeshRouter::handleWithFSM(SelfMessage *msg)
{
    if (isReceivedPacketReady)
    {
        handleProtocolPacket(receivedPacket);
    }

    if (blockSendUntil + preambleAdd > millis())
    {
        return;
    }

    QueuedPacket *queuedPacket = dequeuePacket();
    if (queuedPacket == nullptr)
        return;

    sendPacket(queuedPacket->data, queuedPacket->packetSize);

    if (queuedPacket->data[0] == MESSAGE_TYPE_BROADCAST_RTS)
    {
        SenderWait(150);
    }

    if (dequeuedPacketWasLast())
    {
        long fullWaitTime = predictPacketSendTime(255);
        SenderWait(50 + (fullWaitTime + (rand() % 51)));
    }

    free(queuedPacket->data);
    queuedPacket->data = nullptr;
    free(queuedPacket);
    queuedPacket = nullptr;
}

void MeshRouter::onPreambleDetectedIR()
{
    SenderWait(0);
    // Increase blocking time of other senders.
    preambleAdd = 1000 + predictPacketSendTime(255);
}

void MeshRouter::handleProtocolPacket(ReceivedPacket *receivedPacket)
{
    logReceivedStatistics(receivedPacket->payload, receivedPacket->size);

    preambleAdd = 0;
    uint8_t messageType = receivedPacket->messageType;
    uint8_t *packet = receivedPacket->payload;
    size_t packetSize = receivedPacket->size;
    bool isMission = receivedPacket->isMission;

    switch (messageType)
    {
    case MESSAGE_TYPE_BROADCAST_RTS:
        MeshRouter::OnFloodHeaderPacket((BroadcastRTSPacket *)packet, packetSize, isMission);
        SenderWait(0 + (rand() % 101));
        break;
    case MESSAGE_TYPE_BROADCAST_FRAGMENT:
        MeshRouter::OnFloodFragmentPacket((BroadcastFragmentPacket *)packet, packetSize, isMission);
        SenderWait(0 + (rand() % 101));
        break;
    default:
        break;
    }

    finishReceiving();
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

void MeshRouter::handleUpperPacket(MessageToSend *messageToSend)
{
    createMessage(messageToSend->payload, messageToSend->size, nodeId, true, messageToSend->isMission, false);
}

void MeshRouter::OnFloodHeaderPacket(BroadcastRTSPacket *packet, size_t packetSize, bool isMission)
{
    if (doesIncompletePacketExist(packet->id, packet->source, isMission))
    {
        // Already received this Packet!
        return;
    }

    bool createdPacket = createIncompletePacket(packet->id, packet->size, packet->source, packet->hopId, packet->messageType, packet->checksum, isMission);
    if (!createdPacket)
        return;
    uint16_t nextFragLength = (uint16_t)packet->size > 255 ? 255 : packet->size;
    SenderWait((unsigned long)20 + predictPacketSendTime(nextFragLength));
}

void MeshRouter::OnFloodFragmentPacket(BroadcastFragmentPacket *packet, size_t packetSize, bool isMission)
{
    if (doesIncompletePacketExist(packet->source, packet->id, isMission))
    {
        // No information about future packages. Assume the greatest!
        // Keine Informationen über zukünftige Pakete. Gehe vom größten aus!
        SenderWait(40 + predictPacketSendTime(255));
        return;
    }

    Result result = addToIncompletePacket(packet->id, packet->source, packet->fragment, packetSize, packet->payload, isMission, false);
    if (result.completePacket == nullptr)
        return;

    handlePacketResult(result, true, false);

    if (result.sendUp)
    {
        blockSendUntil = millis() + (10 + (rand() % 241));
    }

    // Delay Transmission of Packets
    SenderWait(20 + predictPacketSendTime(result.bytesLeft > 255 ? 255 : result.bytesLeft));
}

long MeshRouter::predictPacketSendTime(uint8_t size)
{
    long time = size;
    time += 20; // ms = Byte overhead
    return time;
}
