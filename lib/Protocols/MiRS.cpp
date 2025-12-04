#include "MiRS.h"

const char *MiRS::getProtocolName()
{
    return "mirs";
}

void MiRS::initProtocol()
{
    initRTSCTS();
}

void MiRS::finishProtocol()
{
    finishRTSCTS();
    fsm.setState(0);
}

void MiRS::handleWithFSM(SelfMessage *msg)
{
    if (msg == nullptr)
    {
        static SelfMessage defaultMsg{"default"};
        msg = &defaultMsg;
    }

    FSMA_Switch(fsm)
    {
        FSMA_State(LISTENING)
        {
            FSMA_Event_Transition(we got rts now send cts,
                                  initiateCTS && isFreeToSend(),
                                  BACKOFF_CTS, );
            FSMA_Event_Transition(we have packet to send and just send it,
                                  currentTransmission != nullptr &&
                                      !isReceiving() &&
                                      !shortWaitTimer.isScheduled() &&
                                      !initiateCTS &&
                                      isFreeToSend(),
                                  BACKOFF, );
            FSMA_Event_Transition(detected preamble and trying to receive,
                                  isReceiving(),
                                  RECEIVING, );
        }
        FSMA_State(BACKOFF)
        {
            FSMA_Enter(regularBackoffHandler.scheduleBackoffTimer());
            FSMA_Event_Transition(backoff finished send rts,
                                  regularBackoff == *msg && isWithRTS(false),
                                  SEND_RTS,
                                  regularBackoffHandler.invalidateBackoffPeriod(););
            FSMA_Event_Transition(backoff finished message without rts - only NodeAnnounce in th1s protocol,
                                  regularBackoff == *msg && !isWithRTS(false),
                                  TRANSMITTING,
                                  regularBackoffHandler.invalidateBackoffPeriod(););
            FSMA_Event_Transition(receiving msg - cancle backoff - listen now,
                                  isReceiving(),
                                  RECEIVING,
                                  regularBackoffHandler.cancelBackoffTimer();
                                  regularBackoffHandler.decreaseBackoffPeriod(););
        }
        FSMA_State(SEND_RTS)
        {
            FSMA_Enter(sendRTS());
            FSMA_Event_Transition(rts is sent now wait f0r cts,
                                  !isTransmitting(),
                                  WAIT_CTS, );
        }
        FSMA_State(WAIT_CTS)
        {
            FSMA_Event_Transition(got some other CTS - wait f0r the maximum CTS CW time,
                                  isStrayCTS(receivedPacket),
                                  LISTENING,
                                  msgScheduler.cancel(&waitForCTSTimer);
                                  handleStrayCTS(receivedPacket, false);
                                  handleCTSTimeout(true););
            FSMA_Event_Transition(we didnt get cts go back to listening,
                                  waitForCTSTimer == *msg,
                                  LISTENING,
                                  handleCTSTimeout(true);
                                  msgScheduler.schedule(&shortWaitTimer, sifs_MS););
            FSMA_Event_Transition(received a CTS meant f0r us,
                                  isOurCTS(),
                                  TRANSMITTING,
                                  msgScheduler.cancel(&waitForCTSTimer););
        }
        FSMA_State(TRANSMITTING)
        {
            FSMA_Enter(sendPacket(currentTransmission->data, currentTransmission->packetSize));
            FSMA_Event_Transition(finished transmitting,
                                  !isTransmitting(),
                                  LISTENING, finishCurrentTransmission());
        }
        FSMA_State(BACKOFF_CTS)
        {
            FSMA_Enter(initiateCTS = false; ctsBackoffHandler.scheduleBackoffTimer(););
            FSMA_Event_Transition(got some other CTS - wait f0r the maximum CTS CW time,
                                  isStrayCTS(receivedPacket),
                                  LISTENING,
                                  ctsBackoffHandler.invalidateBackoffPeriod();
                                  ctsBackoffHandler.cancelBackoffTimer();
                                  handleStrayCTS(receivedPacket, false));
            FSMA_Event_Transition(cts backoff finished and we send cts,
                                  ctsBackoff == *msg && !isReceiving(),
                                  SEND_CTS,
                                  ctsBackoffHandler.invalidateBackoffPeriod(););
            FSMA_Event_Transition(got cts sent to same source as we want to send to,
                                  isCTSForSameRTSSource(receivedPacket),
                                  AWAIT_TRANSMISSION,
                                  ctsBackoffHandler.invalidateBackoffPeriod();
                                  ctsBackoffHandler.cancelBackoffTimer();
                                  clearRTSsource();
                                  msgScheduler.schedule(&transmissionStartTimer, sifs_MS););
            FSMA_Event_Transition(got packet from rts source,
                                  isPacketFromRTSSource(receivedPacket),
                                  RECEIVING,
                                  ctsBackoffHandler.invalidateBackoffPeriod();
                                  ctsBackoffHandler.cancelBackoffTimer();
                                  msgScheduler.schedule(&shortWaitTimer, sifs_MS););
        }
        FSMA_State(SEND_CTS)
        {
            FSMA_Enter(sendCTS());
            FSMA_Event_Transition(finished sending CTS now listen f0r packet,
                                  !isTransmitting(),
                                  AWAIT_TRANSMISSION, );
        }
        FSMA_State(AWAIT_TRANSMISSION)
        {
            FSMA_Event_Transition(got some other CTS - wait f0r the maximum CTS CW time,
                                  isStrayCTS(receivedPacket),
                                  AWAIT_TRANSMISSION,
                                  handleStrayCTS(receivedPacket, false));
            FSMA_Event_Transition(source didnt get cts - just go back to regular listening,
                                  transmissionStartTimer == *msg && !isReceiving(),
                                  LISTENING,
                                  clearRTSsource();
                                  msgScheduler.cancel(&transmissionEndTimer);
                                  msgScheduler.schedule(&shortWaitTimer, sifs_MS););
            FSMA_Event_Transition(received packet from RTS source - we waited f0r that,
                                  isPacketFromRTSSource(receivedPacket),
                                  RECEIVING,
                                  msgScheduler.cancel(&transmissionStartTimer);
                                  msgScheduler.cancel(&transmissionEndTimer);
                                  msgScheduler.schedule(&shortWaitTimer, sifs_MS););
            FSMA_Event_Transition(did not receive message from RTS source - just go back to listen,
                                  transmissionEndTimer == *msg,
                                  LISTENING,
                                  msgScheduler.schedule(&shortWaitTimer, sifs_MS););
        }
        FSMA_State(RECEIVING)
        {
            FSMA_Event_Transition(got - message,
                                  isReceivedPacketReady,
                                  LISTENING,
                                  handleProtocolPacket(receivedPacket));
            FSMA_Event_Transition(timeout,
                                  hasPreambleTimedOut(),
                                  LISTENING,
                                  finishReceiving());
        }
    }

    if (fsm.getState() != RECEIVING)
    {
        finishReceiving();
    }

    if (fsm.getState() == LISTENING && !customPacketQueue.isEmpty() && currentTransmission == nullptr)
    {
        currentTransmission = dequeuePacket();
    }
}

void MiRS::handleUpperPacket(MessageToSend *msg)
{
    if (msg->isMission)
    {
        createMessage(msg->payload, msg->size, nodeId, true, msg->isMission, true);
    }
    else
    {
        createMessage(msg->payload, msg->size, nodeId, false, msg->isMission, false);
    }
}

void MiRS::handleProtocolPacket(ReceivedPacket *receivedPacket)
{
    logReceivedStatistics(receivedPacket->payload, receivedPacket->size, receivedPacket->isMission);

    uint8_t messageType = receivedPacket->messageType;
    uint8_t *packet = receivedPacket->payload;
    size_t packetSize = receivedPacket->size;
    bool isMission = receivedPacket->isMission;

    switch (messageType)
    {
    case MESSAGE_TYPE_BROADCAST_RTS:
        handleRTS((BroadcastRTS *)packet, packetSize, isMission);
        break;
    case MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT:
        handleLeaderFragment((BroadcastLeaderFragment *)packet, packetSize, isMission);
        break;
    case MESSAGE_TYPE_BROADCAST_CONTINUOUS_RTS:
        handleContinuousRTS((BroadcastContinuousRTS *)packet, packetSize, isMission);
        break;
    case MESSAGE_TYPE_BROADCAST_FRAGMENT:
        handleFragment((BroadcastFragment *)packet, packetSize, isMission);
        break;
    case MESSAGE_TYPE_BROADCAST_CTS:
        handleCTS((BroadcastCTS *)packet, packetSize, isMission);
        break;
    default:
        break;
    }

    finishReceiving();
}

void MiRS::handleRTS(const BroadcastRTS *packet, const size_t packetSize, bool isMission)
{
    bool createdPacket = createIncompletePacket(packet->id, packet->size, packet->source, packet->hopId, packet->messageType, packet->checksum, isMission);
    if (!createdPacket)
        return;

    uint16_t sizeOfFragment = packet->size > LORA_MAX_FRAGMENT_PAYLOAD ? 255 : packet->size + BROADCAST_FRAGMENT_METADATA_SIZE;
    ctsData.fragmentSize = sizeOfFragment;
    ctsData.rtsSource = packet->hopId;
    rtsSource = packet->hopId;
    initiateCTS = true;
}

void MiRS::handleLeaderFragment(const BroadcastLeaderFragment *packet, const size_t packetSize, bool isMission)
{
    bool created = createIncompletePacket(packet->id, packet->size, packet->source, packet->hopId, packet->messageType, packet->checksum, isMission);
    if (!created)
        return;
    Result result = addToIncompletePacket(packet->id, packet->source, 0, packetSize, packet->payload, isMission, true);
    handlePacketResult(result, false, false);
}

void MiRS::handleContinuousRTS(const BroadcastContinuousRTS *packet, const size_t packetSize, bool isMission)
{
    if (!doesIncompletePacketExist(packet->source, packet->id, isMission))
        return;

    ctsData.fragmentSize = packet->fragmentSize;
    ctsData.rtsSource = packet->hopId;
    rtsSource = packet->hopId;
    initiateCTS = true;
}

void MiRS::handleFragment(const BroadcastFragment *packet, const size_t packetSize, bool isMission)
{
    Result result = addToIncompletePacket(packet->id, packet->source, packet->fragment, packetSize, packet->payload, isMission, false);
    handlePacketResult(result, true, true);
}