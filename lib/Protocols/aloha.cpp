#include "Aloha.h"

void Aloha::init()
{
}

void Aloha::setNodeID()
{
}

void Aloha::handleWithFSM()
{
    FSMA_Switch(fsm)
    {
        FSMA_State(LISTENING)
        {
            FSMA_Enter();
            FSMA_Event_Transition(a, true, RECEIVING, );
            FSMA_Event_Transition(
                we have packet to send and just send it,
                currentTransmission,
                TRANSMITTING, );
        }
        FSMA_State(TRANSMITTING)
        {
            FSMA_Enter(sendPacket(currentTransmission->data, currentTransmission->packetSize));
            FSMA_Event_Transition(
                finished transmitting,
                !isTransmitting(),
                LISTENING, );
        }
        FSMA_State(RECEIVING)
        {
            FSMA_Enter();
            FSMA_Event_Transition(
                got - message,
                isReceivedPacketReady,
                LISTENING,
                handleProtocolPacket(receivedPacket->messageType, receivedPacket->payload, receivedPacket->size, receivedPacket->rssi););
        }
    }

    if (currentTransmission)
    {
        sendPacket(currentTransmission->data, currentTransmission->packetSize);
    }

    if (isReceivedPacketReady)
    {
        handleProtocolPacket(receivedPacket->messageType, receivedPacket->payload, receivedPacket->size, receivedPacket->rssi);
    }
}

void Aloha::onPreambleDetectedIR()
{
}

void Aloha::clearMacData()
{
}

void Aloha::handleProtocolPacket(const uint8_t messageType, const uint8_t *packet, const uint8_t packetSize, const int rssi)
{
    switch (messageType)
    {
    case MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT:
        handleLeaderFragment(packet, packetSize);
        break;
    case MESSAGE_TYPE_BROADCAST_FRAGMENT:
        handleFragment(packet, packetSize);
        break;
    default:
        break;
    }
}

void Aloha::handleLeaderFragment(const uint8_t *packet, const uint8_t packetSize)
{
}

void Aloha::handleFragment(const uint8_t *packet, const uint8_t packetSize)
{
}