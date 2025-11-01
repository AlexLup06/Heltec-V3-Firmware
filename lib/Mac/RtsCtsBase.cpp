#include "RtsCtsBase.h"

void RtsCtsBase::initRTSCTS()
{
    shortWaitTimer = SelfMessage("shortWaitTimer");
    waitForCTSTimer = SelfMessage("waitForCTSTimer");
    transmissionStartTimer = SelfMessage("transmissionStartTimer");
    transmissionEndTimer = SelfMessage("transmissionEndTimer");
    ongoingTransmissionTimer = SelfMessage("ongoingTransmissionTimer");
    ctsBackoff = SelfMessage("ctsBackoff");
    regularBackoff = SelfMessage("regularBackoff");
}

void RtsCtsBase::finishRTSCTS()
{
    initiateCTS = false;
    msgScheduler.cancel(&shortWaitTimer);
    msgScheduler.cancel(&waitForCTSTimer);
    msgScheduler.cancel(&transmissionStartTimer);
    msgScheduler.cancel(&transmissionEndTimer);
    msgScheduler.cancel(&ongoingTransmissionTimer);
    msgScheduler.cancel(&ctsBackoff);
    msgScheduler.cancel(&regularBackoff);

    ctsBackoffHandler.invalidateBackoffPeriod();
    regularBackoffHandler.invalidateBackoffPeriod();
}

bool RtsCtsBase::isFreeToSend()
{
    DEBUG_PRINTF("Isfree to send: %d\n", !ongoingTransmissionTimer.isScheduled() && !transmissionStartTimer.isScheduled());
    return !ongoingTransmissionTimer.isScheduled() && !transmissionStartTimer.isScheduled();
}

bool RtsCtsBase::isCTSForSameRTSSource(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    BroadcastCTS *cts = tryCastMessage<BroadcastCTS>(receivedPacket->payload);
    if (cts == nullptr)
        return false;
    return cts->rtsSource == rtsSource;
}

bool RtsCtsBase::isPacketFromRTSSource(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    BroadcastFragmentPacket *fragment = tryCastMessage<BroadcastFragmentPacket>(receivedPacket->payload);
    if (fragment == nullptr)
    {
        finishReceiving();
        return false;
    }

    FragmentedPacket *incompletePacket = getIncompletePacketBySource(fragment->source, receivedPacket->isMission);
    if (incompletePacket == nullptr)
    {
        finishReceiving();
        return false;
    }

    if (incompletePacket->hopId == rtsSource)
    {
        rtsSource = -1;
        return true;
    }
    finishReceiving();
    return false;
}

void RtsCtsBase::sendCTS()
{
    customPacketQueue.printQueue();

    DEBUG_PRINTLN("Send CTS");
    BroadcastCTS *cts = createCTS(ctsData.fragmentSize, ctsData.rtsSource);
    sendPacket((uint8_t *)cts, sizeof(cts));
    free(cts);

    // After we send CTS, we need to receive message within ctsFS + sifs otherwise we assume there will be no message
    msgScheduler.schedule(&transmissionStartTimer, ctsFS_MS + sifs_MS);
    msgScheduler.schedule(&transmissionEndTimer, ctsFS_MS + sifs_MS + getToAByPacketSizeInUS(ctsData.fragmentSize) / 1000);

    ctsData.fragmentSize = -1;
    ctsData.rtsSource = -1;
}

void RtsCtsBase::sendRTS()
{
    customPacketQueue.printQueue();

    assert(currentTransmission->isHeader);

    sendPacket(currentTransmission->data, currentTransmission->packetSize);

    int scheduleCTSTimerTime = ctsFS_MS * ctsCW + sifs_MS + getToAByPacketSizeInUS(currentTransmission->packetSize) / 1000 + ctsFS_MS;
    msgScheduler.schedule(&waitForCTSTimer, scheduleCTSTimerTime);

    finishCurrentTransmission();
    DEBUG_PRINTF("[RtsBase] CTS timeout in %d ms\n", scheduleCTSTimerTime);
}

void RtsCtsBase::clearRTSsource()
{
    rtsSource = -1;
}

// Put packet back to queue at front and currentTransmission is the header again, to try once again. After 3 tries delete the packet
void RtsCtsBase::handleCTSTimeout()
{
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
    if (!isReceivedPacketReady)
        return false;

    DEBUG_PRINTLN("Before casitng");
    BroadcastCTS *cts = tryCastMessage<BroadcastCTS>(receivedPacket->payload);
    DEBUG_PRINTLN("After casting");
    if (cts == nullptr)
        return false;
    DEBUG_PRINTLN("Before sourcing");
    uint8_t rs = cts->rtsSource;
    DEBUG_PRINTLN("After sourcing");
    return nodeId == cts->rtsSource;
}

// In this protocol all messages except NodeAnnounce are send with RTS
bool RtsCtsBase::withRTS(bool neighbourWithRTS)
{
    if (!neighbourWithRTS)
    {
        return !currentTransmission->isNodeAnnounce && currentTransmission->isMission;
    }

    return !currentTransmission->isNodeAnnounce;
}