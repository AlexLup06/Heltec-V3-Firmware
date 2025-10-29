#include "Aloha.h"

String Aloha::getProtocolName() { return "aloha"; }
void Aloha::onPreambleDetectedIR() {}
void Aloha::onCRCerrorIR() { DEBUG_PRINTLN("CRC error (packet corrupted)"); }

void Aloha::handleWithFSM()
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
                handleProtocolPacket(receivedPacket->messageType, receivedPacket->payload, receivedPacket->size, receivedPacket->rssi, receivedPacket->isMission););
            FSMA_Event_Transition(
                we have packet to send and just send it,
                currentTransmission != nullptr,
                TRANSMITTING, );
        }
    }
}

void Aloha::handleUpperPacket(MessageToSend_t *msg)
{
    createMessage(msg->payload, msg->size, nodeId, false, msg->isMission);
}

void Aloha::handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const size_t packetSize, const int rssi, bool isMission)
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

void Aloha::handleLeaderFragment(const BroadcastLeaderFragmentPacket_t *packet, const size_t packetSize, bool isMission)
{
    createIncompletePacket(packet->id, packet->size, packet->source, packet->messageType, packet->checksum, isMission);
    Result result = addToIncompletePacket(packet->id, packet->source, 0, packetSize, packet->payload, isMission, true);
    handlePacketResult(result, false);
}

void Aloha::handleFragment(const BroadcastFragmentPacket_t *packet, const size_t packetSize, bool isMission)
{
    Result result = addToIncompletePacket(packet->id, packet->source, packet->fragment, packetSize, packet->payload, isMission, false);
    handlePacketResult(result, false);
}