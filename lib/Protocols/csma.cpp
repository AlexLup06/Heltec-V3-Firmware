#include "Csma.h"

String Csma::getProtocolName() { return "csma"; }
void Csma::onPreambleDetectedIR() {}
void Csma::onCRCerrorIR() { DEBUG_PRINTLN("CRC error (packet corrupted)"); }

void Csma::handleWithFSM()
{
    FSMA_Switch(fsm)
    {
        FSMA_State(LISTENING)
        {
            FSMA_Event_Transition(
                detected preamble and trying to receive,
                isReceiving(),
                RECEIVING, );
            FSMA_Event_Transition(
                we have packet to send and just send it,
                currentTransmission != nullptr && !isReceiving(),
                BACKOFF, );
        }
        FSMA_State(BACKOFF)
        {

            FSMA_Enter(scheduleBackoffTimer());
            FSMA_Event_Transition(Start - Transmit,
                                  backoffFinished(),
                                  TRANSMITTING,
                                  invalidateBackoffPeriod(););
            FSMA_Event_Transition(Start - Receive,
                                  isReceiving(),
                                  RECEIVING,
                                  cancelBackoffTimer();
                                  decreaseBackoffPeriod(););
        }
        FSMA_State(TRANSMITTING)
        {
            FSMA_Enter(sendPacket(currentTransmission->data, currentTransmission->packetSize));
            FSMA_Event_Transition(
                finished transmitting,
                !isTransmitting(),
                LISTENING, finishCurrentTransmission());
        }
        FSMA_State(RECEIVING)
        {
            FSMA_Event_Transition(
                got - message,
                isReceivedPacketReady,
                LISTENING,
                handleProtocolPacket(receivedPacket->messageType, receivedPacket->payload, receivedPacket->size, receivedPacket->isMission););
        }
    }

    if (fsm.getState() == LISTENING && !customPacketQueue.isEmpty())
    {
        currentTransmission = dequeuePacket();
    }
}

void Csma::handleUpperPacket(MessageToSend_t *msg)
{
    createMessage(msg->payload, msg->size, nodeId, false, msg->isMission);
}

void Csma::handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const size_t packetSize, bool isMission)
{
    switch (messageType)
    {
    case MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT:
        handleLeaderFragment((BroadcastLeaderFragmentPacket_t *)packet, packetSize, isMission);
        break;
    case MESSAGE_TYPE_BROADCAST_FRAGMENT:
        handleFragment((BroadcastFragmentPacket_t *)packet, packetSize, isMission);
        break;
    default:
        break;
    }
}

void Csma::handleLeaderFragment(const BroadcastLeaderFragmentPacket_t *packet, const size_t packetSize, bool isMission)
{
    createIncompletePacket(packet->id, packet->size, packet->source, packet->messageType, packet->checksum, isMission);
    Result result = addToIncompletePacket(packet->id, packet->source, 0, packetSize, packet->payload, isMission, true);
    handlePacketResult(result, false);
}

void Csma::handleFragment(const BroadcastFragmentPacket_t *packet, const size_t packetSize, bool isMission)
{
    Result result = addToIncompletePacket(packet->id, packet->source, packet->fragment, packetSize, packet->payload, isMission, false);
    handlePacketResult(result, false);
}

// --------------- Backoff helper functions ---------------

bool Csma::backoffFinished()
{
    if (backoffPeriodMs < 0 || !backoffRunning)
        return false;

    int now = millis();
    int elapsed = now - backoffStartMs;

    if (elapsed >= backoffPeriodMs)
    {
        backoffRunning = false;
        backoffPeriodMs = -1;
        return true;
    }

    return false;
}

void Csma::invalidateBackoffPeriod()
{
    backoffPeriodMs = -1;
    backoffRunning = false;
}

void Csma::cancelBackoffTimer()
{
    backoffRunning = false;
    backoffStartMs = 0;

    DEBUG_PRINTLN("[CSMA] Backoff timer cancelled");
}

void Csma::decreaseBackoffPeriod()
{
    if (backoffPeriodMs <= 0 || !backoffRunning)
        return;

    int now = millis();
    int elapsed = now - backoffStartMs;

    int slotsElapsed = (int)(elapsed / slotTimeMS);
    backoffPeriodMs -= slotsElapsed * slotTimeMS;

    if (backoffPeriodMs < 0)
        backoffPeriodMs = 0;

    DEBUG_PRINTF("[CSMA] Backoff decreased, remaining=%ld ms\n", backoffPeriodMs);
}

void Csma::scheduleBackoffTimer()
{
    if (backoffPeriodMs < 0)
    {
        int slots = random(1, cw + 1);
        backoffPeriodMs = slots * slotTimeMS;
        DEBUG_PRINTF("[CSMA] New backoff generated: slots=%d -> %ld ms\n", slots, backoffPeriodMs);
    }
    else
    {
        DEBUG_PRINTF("[CSMA] Resuming backoff: remaining=%ld ms\n", backoffPeriodMs);
    }

    backoffStartMs = millis();
    backoffRunning = true;
}