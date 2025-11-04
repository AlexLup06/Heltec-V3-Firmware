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
    // DEBUG_PRINTF("Isfree to send: %d\n", !ongoingTransmissionTimer.isScheduled() && !transmissionStartTimer.isScheduled());
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

    DEBUG_PRINTLN("[RtsCts] Check isPacketFromRTSSource");
    BroadcastFragmentPacket *fragment = tryCastMessage<BroadcastFragmentPacket>(receivedPacket->payload);
    if (fragment == nullptr)
    {
        DEBUG_PRINTLN("[RtsCts] Packet NOT fragment");
        return false;
    }

    FragmentedPacket *incompletePacket = getIncompletePacketBySource(fragment->source, receivedPacket->isMission);
    if (incompletePacket == nullptr)
    {
        DEBUG_PRINTLN("[RtsCts] Did not receive header");
        return false;
    }

    if (incompletePacket->hopId == rtsSource)
    {
        DEBUG_PRINTLN("[RtsCts] Received packet from rts source");
        rtsSource = -1;
        return true;
    }
    DEBUG_PRINTLN("[RtsCts] DID NOT receive packet from rts source");
    return false;
}

void RtsCtsBase::sendCTS(bool waitForCTStimeout)
{
    customPacketQueue.printQueue();

    BroadcastCTS *cts = createCTS(ctsData.fragmentSize, ctsData.rtsSource);
    sendPacket((uint8_t *)cts, sizeof(cts));
    free(cts);

    // After we send CTS, we need to receive message within ctsFS + sifs otherwise we assume there will be no message
    // if we are RSMiTra then we need to wait for the remainder of the CTS cw window because the transmitter waits until the end
    uint16_t startSend = ctsFS_MS + sifs_MS;
    if (waitForCTStimeout)
    {
        startSend += ctsBackoffHandler.getRemainderCW() * ctsFS_MS;
    }

    msgScheduler.schedule(&transmissionStartTimer, startSend);
    msgScheduler.schedule(&transmissionEndTimer, startSend + getToAByPacketSizeInUS(ctsData.fragmentSize) / 1000);

    ctsData.fragmentSize = -1;
    ctsData.rtsSource = -1;
}

void RtsCtsBase::sendRTS()
{
    customPacketQueue.printQueue();

    assert(currentTransmission->isHeader);

    sendPacket(currentTransmission->data, currentTransmission->packetSize);

    unsigned long scheduleCTSTimerTime = ctsFS_MS * ctsCW + sifs_MS + (getToAByPacketSizeInUS(currentTransmission->packetSize) / 1000UL) + ctsFS_MS;
    msgScheduler.schedule(&waitForCTSTimer, scheduleCTSTimerTime);

    finishCurrentTransmission();
    DEBUG_PRINTF("[RtsBase] CTS timeout in %d ms\n", scheduleCTSTimerTime);
    DEBUG_PRINTF("[RtsBase] we sent rts and scheduled cts timeout at millis:  %lu\n", millis());
}

void RtsCtsBase::clearRTSsource()
{
    rtsSource = -1;
}

// Put packet back to queue at front and currentTransmission is the header again, to try once again. After 3 tries delete the packet
void RtsCtsBase::handleCTSTimeout()
{
    DEBUG_PRINTF("[RtsBase] CTS cw timeout at millis:  %lu\n", millis());
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

    BroadcastCTS *cts = tryCastMessage<BroadcastCTS>(receivedPacket->payload);

    if (cts == nullptr)
    {
        return false;
    }

    uint8_t rs = cts->rtsSource;
    return nodeId == rs;
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