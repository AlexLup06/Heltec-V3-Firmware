#include "RtsCtsBase.h"

bool RtsCtsBase::isFreeToSend()
{
    // DEBUG_PRINTLN("[RtsBase] isFreeToSend");

    return !ongoingTransmissionTimer.isScheduled() && !transmissionStartTimer.isScheduled();
}

bool RtsCtsBase::isCTSForSameRTSSource(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    BroadcastCTS_t *cts = tryCastMessage<BroadcastCTS_t>(receivedPacket->payload);
    if (cts == nullptr)
        return false;

    return cts->rtsSource == rtsSource;
}

bool RtsCtsBase::isPacketFromRTSSource(ReceivedPacket *receivedPacket)
{
    // DEBUG_PRINTLN("[RtsBase] isPacketFromRTSSource");

    if (!isReceivedPacketReady)
        return false;

    BroadcastFragmentPacket_t *fragment = tryCastMessage<BroadcastFragmentPacket_t>(receivedPacket->payload);
    if (fragment == nullptr)
        return false;

    FragmentedPacket *incompletePacket = getIncompletePacketBySource(fragment->source, receivedPacket->isMission);
    if (incompletePacket == nullptr)
        return false;

    if (incompletePacket->hopId == rtsSource)
    {
        rtsSource = -1;
        return true;
    }
    return false;
}

void RtsCtsBase::sendCTS()
{
    // DEBUG_PRINTLN("[RtsBase] sendCTS");

    BroadcastCTS_t *cts = createCTS(ctsData.fragmentSize, ctsData.rtsSource);
    sendPacket((uint8_t *)cts, sizeof(cts));
    free(cts);

    // After we send CTS, we need to receive message within ctsFS + sifs otherwise we assume there will be no message
    transmissionStartTimer.schedule(ctsFS_MS + sifs_MS);
    transmissionEndTimer.schedule(ctsFS_MS + sifs_MS + getToAByPacketSizeInUS(ctsData.fragmentSize) / 1000);

    ctsData.fragmentSize = -1;
    ctsData.rtsSource = -1;
}

void RtsCtsBase::sendRTS()
{
    // DEBUG_PRINTLN("[RtsBase] sendRTS");

    assert(currentTransmission->isHeader);

    int scheduleCTSTimerTime = ctsFS_MS * ctsCW + sifs_MS + getToAByPacketSizeInUS(currentTransmission->packetSize) / 1000 + ctsFS_MS;
    DEBUG_PRINTF("[RtsBase] CTS timeout in %d ms\n", scheduleCTSTimerTime);
    waitForCTSTimer.schedule(scheduleCTSTimerTime);
    sendPacket(currentTransmission->data, currentTransmission->packetSize);
    finishCurrentTransmission();
}

void RtsCtsBase::clearRTSsource()
{
    // DEBUG_PRINTLN("[RtsBase] clearRTSsource");

    rtsSource = -1;
}

// Put packet back to queue at front and currentTransmission is the header again, to try once again. After 3 tries delete the packet
void RtsCtsBase::handleCTSTimeout()
{
    // DEBUG_PRINTLN("[RtsBase] handleCTSTimeout");

    currentTransmission->sendTrys++;
    if (currentTransmission->sendTrys >= 3)
    {
        finishCurrentTransmission();
        return;
    }

    customPacketQueue.enqueuePacketAtPosition(currentTransmission, 0);

    if (currentTransmission->hasContinuousRTS)
        currentTransmission = createNewContinuousRTS(currentTransmission);
    else
        currentTransmission = createNewRTS(currentTransmission);
}

// check if the CTS that we got was addressed to us - we are the RTS source
bool RtsCtsBase::isOurCTS()
{
    // DEBUG_PRINTLN("[RtsBase] isOurCts");
    if (!isReceivedPacketReady)
        return false;

    BroadcastCTS_t *cts = tryCastMessage<BroadcastCTS_t>(receivedPacket->payload);
    if (cts == nullptr)
        return false;
    return nodeId == cts->rtsSource;
}

// In this protocol all messages except NodeAnnounce are send with RTS
bool RtsCtsBase::withRTS(bool neighbourWithRTS)
{
    // DEBUG_PRINTLN("[RtsBase] withRTS");

    if (!neighbourWithRTS)
    {
        return !currentTransmission->isNodeAnnounce && currentTransmission->isMission;
    }

    return !currentTransmission->isNodeAnnounce;
}