#include "Aloha.h"

String Aloha::getProtocolName()
{
    return "aloha";
}

void Aloha::finishProtocol()
{
    fsm.setState(0);
}

void Aloha::handleWithFSM(SelfMessage *msg)
{
    FSMA_Switch(fsm)
    {
        FSMA_State(LISTENING)
        {
            FSMA_Event_Transition(
                detected preamble and trying to receive,
                isReceiving() && currentTransmission == nullptr,
                RECEIVING, );
            FSMA_Event_Transition(
                we have packet to send and just send it,
                currentTransmission != nullptr,
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
                handleProtocolPacket(receivedPacket););
            FSMA_Event_Transition(
                we have packet to send and just send it,
                currentTransmission != nullptr,
                TRANSMITTING, );
        }
    }

    if (fsm.getState() == LISTENING && !customPacketQueue.isEmpty() && currentTransmission == nullptr)
    {
        currentTransmission = dequeuePacket();
    }
}

void Aloha::handleUpperPacket(MessageToSend *msg)
{
    createMessage(msg->payload, msg->size, nodeId, false, msg->isMission, false);
}

void Aloha::handleProtocolPacket(ReceivedPacket *receivedPacket)
{
    DEBUG_PRINTLN("[RadioBase] handleProtocolPacket");

    uint8_t messageType = receivedPacket->messageType;
    uint8_t *packet = receivedPacket->payload;
    size_t packetSize = receivedPacket->size;
    bool isMission = receivedPacket->isMission;

    switch (messageType)
    {
    case MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT:
        handleLeaderFragment((BroadcastLeaderFragmentPacket *)packet, packetSize, isMission);
        break;
    case MESSAGE_TYPE_BROADCAST_FRAGMENT:
        handleFragment((BroadcastFragmentPacket *)packet, packetSize, isMission);
        break;
    default:
        break;
    }

    finishReceiving();
}

void Aloha::handleLeaderFragment(const BroadcastLeaderFragmentPacket *packet, const size_t packetSize, bool isMission)
{
    createIncompletePacket(packet->id, packet->size, packet->source, -1, packet->messageType, packet->checksum, isMission);
    Result result = addToIncompletePacket(packet->id, packet->source, 0, packetSize, packet->payload, isMission, true);
    handlePacketResult(result, false, false);
}

void Aloha::handleFragment(const BroadcastFragmentPacket *packet, const size_t packetSize, bool isMission)
{
    Result result = addToIncompletePacket(packet->id, packet->source, packet->fragment, packetSize, packet->payload, isMission, false);
    handlePacketResult(result, false, false);
}