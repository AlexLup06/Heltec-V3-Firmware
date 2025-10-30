#include "CadAloha.h"

String CadAloha::getProtocolName() { return "cadaloha"; }
void CadAloha::onPreambleDetectedIR() {}
void CadAloha::onCRCerrorIR() { DEBUG_PRINTLN("CRC error (packet corrupted)"); }

void CadAloha::handleWithFSM()
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
                TRANSMITTING, );
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

void CadAloha::handleUpperPacket(MessageToSend_t *msg)
{
    createMessage(msg->payload, msg->size, nodeId, false, msg->isMission);
}

void CadAloha::handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const size_t packetSize, bool isMission)
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

void CadAloha::handleLeaderFragment(const BroadcastLeaderFragmentPacket_t *packet, const size_t packetSize, bool isMission)
{
    createIncompletePacket(packet->id, packet->size, packet->source, packet->messageType, packet->checksum, isMission);
    Result result = addToIncompletePacket(packet->id, packet->source, 0, packetSize, packet->payload, isMission, true);
    handlePacketResult(result, false);
}

void CadAloha::handleFragment(const BroadcastFragmentPacket_t *packet, const size_t packetSize, bool isMission)
{
    Result result = addToIncompletePacket(packet->id, packet->source, packet->fragment, packetSize, packet->payload, isMission, false);
    handlePacketResult(result, false);
}