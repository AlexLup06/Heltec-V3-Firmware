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

    rtsSource = -1;
    ctsData.fragmentSize = -1;
    ctsData.rtsSource = -1;
}

bool RtsCtsBase::isFreeToSend()
{
    return !ongoingTransmissionTimer.isScheduled() && !transmissionStartTimer.isScheduled();
}

bool RtsCtsBase::isCTSForSameRTSSource(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    const BroadcastCTS *cts = tryCastMessage<const BroadcastCTS>(receivedPacket->payload);
    if (cts == nullptr)
        return false;
    return cts->rtsSource == rtsSource;
}

bool RtsCtsBase::isPacketFromRTSSource(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    const BroadcastFragment *fragment = tryCastMessage<const BroadcastFragment>(receivedPacket->payload);
    if (fragment == nullptr)
    {
        return false;
    }

    FragmentedPacket *incompletePacket = getIncompletePacketBySource(fragment->source, receivedPacket->isMission);
    if (incompletePacket == nullptr)
    {
        return false;
    }

    if (incompletePacket->hopId == rtsSource)
    {
        rtsSource = -1;
        return true;
    }
    return false;
}

bool RtsCtsBase::isPacketNotFromRTSSource(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    if (const BroadcastFragment *fragment = tryCastMessage<const BroadcastFragment>(receivedPacket->payload))
    {
        FragmentedPacket *incompletePacket = getIncompletePacketBySource(fragment->source, receivedPacket->isMission);
        if (incompletePacket == nullptr)
        {
            return true;
        }

        if (incompletePacket->hopId == rtsSource)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        return true;
    }
}

void RtsCtsBase::sendCTS(bool instantFragment)
{
    DEBUG_PRINTLN("Send CTS");
    customPacketQueue.printQueue();

    BroadcastCTS *cts = createCTS(ctsData.fragmentSize, ctsData.rtsSource, ctsBackoffHandler.getChosenSlot());
    sendPacket((uint8_t *)cts, sizeof(cts));
    free(cts);

    // After we send CTS, we need to receive message within ctsFS + sifs otherwise we assume there will be no message
    // if we are RSMiTra then we need to wait for the remainder of the CTS cw window because the transmitter waits until the end
    double startSend = ctsFS_MS + sifs_MS + 6; // + 6 because of tx->rx is huge
    if (!instantFragment)
    {
        startSend += ctsBackoffHandler.getRemainderCW() * ctsFS_MS;
    }

    msgScheduler.schedule(&transmissionStartTimer, startSend);
    msgScheduler.schedule(&transmissionEndTimer, startSend + getToAByPacketSizeInUS(ctsData.fragmentSize) / 1000.0);

    ctsData.fragmentSize = -1;
    ctsData.rtsSource = -1;
}

void RtsCtsBase::sendRTS()
{
    customPacketQueue.printQueue();

    assert(currentTransmission->isHeader);

    sendPacket(currentTransmission->data, currentTransmission->packetSize);

    double scheduleCTSTimerTime =
        ctsFS_MS * (ctsCW - 1) + sifs_MS + 3 +
        (getToAByPacketSizeInUS(currentTransmission->packetSize) / 1000.0) +
        (getToAByPacketSizeInUS(BROADCAST_CTS_SIZE) / 1000.0);
    msgScheduler.schedule(&waitForCTSTimer, scheduleCTSTimerTime);

    finishCurrentTransmission();
}

void RtsCtsBase::clearRTSsource()
{
    rtsSource = -1;
}

// Put packet back to queue at front and currentTransmission is the header again, to try once again. After 3 tries delete the packet
void RtsCtsBase::handleCTSTimeout(bool withRetry)
{
    regularBackoffHandler.increaseCW();
    if (!withRetry)
    {
        finishCurrentTransmission();
        return;
    }

    currentTransmission->sendTrys++;
    if (currentTransmission->sendTrys > 2)
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

    const BroadcastCTS *cts = tryCastMessage<const BroadcastCTS>(receivedPacket->payload);

    if (cts == nullptr)
    {
        return false;
    }

    if (nodeId == cts->rtsSource)
    {
        regularBackoffHandler.resetCw();
        return true;
    }
    return false;
}

bool RtsCtsBase::isWithRTS(bool neighbourWithRTS)
{
    if (!neighbourWithRTS)
    {
        return currentTransmission->isMission;
    }

    return true;
}

bool RtsCtsBase::isStrayCTS(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    const BroadcastCTS *cts = tryCastMessage<const BroadcastCTS>(receivedPacket->payload);

    if (cts == nullptr)
    {
        return false;
    }

    return nodeId != cts->rtsSource;
}

void RtsCtsBase::handleStrayCTS(ReceivedPacket *receivedPacket, bool instantFragment)
{
    if (!isReceivedPacketReady)
        return;

    const BroadcastCTS *cts = tryCastMessage<const BroadcastCTS>(receivedPacket->payload);

    if (cts == nullptr)
    {
        return;
    }

    double time = getToAByPacketSizeInUS(cts->fragmentSize) / 1000.0 + sifs_MS + 6;
    if (!instantFragment)
    {
        double remainingCtsCwDuration = (ctsCW - cts->chosenSlot - 1) * ctsFS_MS;
        time += remainingCtsCwDuration;
    }

    msgScheduler.schedule(&ongoingTransmissionTimer, time);
}

bool RtsCtsBase::isRTS(ReceivedPacket *receivedPacket)
{
    if (!isReceivedPacketReady)
        return false;

    const BroadcastRTS *rts = tryCastMessage<const BroadcastRTS>(receivedPacket->payload);

    if (rts == nullptr)
    {
        return false;
    }
    return true;
}

void RtsCtsBase::handleUnhandeledRTS()
{
    double maxTransmissionTime = getToAByPacketSizeInUS(LORA_MAX_PACKET_SIZE) / 1000.0;
    double maxCtsCWTime = ctsCW * ctsFS_MS;
    double scheduleTime = maxTransmissionTime + maxCtsCWTime + sifs_MS;

    msgScheduler.scheduleOrExtend(&ongoingTransmissionTimer, scheduleTime);
}

void RtsCtsBase::handleCTS(const BroadcastCTS *cts, const size_t packetSize, bool instantFragment)
{
    // we are only here if we did not request the CTS. Just wait for the time that the transmission
    double time = (double)getToAByPacketSizeInUS(cts->fragmentSize) / 1000.0 + sifs_MS + 6;
    if (!instantFragment)
    {
        double remainingCtsCwDuration = (ctsCW - cts->chosenSlot - 1) * ctsFS_MS;
        time += remainingCtsCwDuration;
    }
    msgScheduler.scheduleOrExtend(&ongoingTransmissionTimer, time);
}
