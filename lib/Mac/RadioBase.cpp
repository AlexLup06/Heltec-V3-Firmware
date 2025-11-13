#include "RadioBase.h"

void RadioBase::assignRadio(SX1262Public *_radio)
{
    radio = _radio;
}

void RadioBase::finishRadioBase()
{
    isReceivingVar = false;
    isTransmittingVar = false;
    radio->irqFlag = 0;
    lastPreambleTime = 0;
}

void RadioBase::setOnSendCallback(std::function<void()> cb)
{
    onSendCallback = cb;
}

void RadioBase::handleDio1Interrupt()
{
    uint16_t flags = radio->irqFlag;
    radio->irqFlag = 0;

    if (flags & RADIOLIB_SX126X_IRQ_TX_DONE)
    {
        DEBUG_PRINTLN("[RadioBase] RADIOLIB_SX126X_IRQ_TX_DONE");
        isTransmittingVar = false;
        onSendCallback();
        startReceive();
        return;
    }

    if (flags & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
    {
        DEBUG_PRINTLN("[RadioBase] got preamble");
        isReceivingVar = true;
        lastPreambleTime = millis();
        onPreambleDetectedIR();
    }

    if (flags & (RADIOLIB_SX126X_IRQ_RX_DONE |
                 RADIOLIB_SX126X_IRQ_HEADER_ERR |
                 RADIOLIB_SX126X_IRQ_CRC_ERR))
    {
        if (flags & RADIOLIB_SX126X_IRQ_RX_DONE)
        {
            DEBUG_PRINTLN("[RadioBase] RADIOLIB_SX126X_IRQ_RX_DONE");
            onReceiveIR();
        }
        else if (flags & (RADIOLIB_SX126X_IRQ_HEADER_ERR | RADIOLIB_SX126X_IRQ_CRC_ERR))
        {
            if (lastPreambleTime && (millis() - lastPreambleTime) < 250)
            {
                float snr = radio->getSNR();
                if (snr > -5)
                {
                    // TODO: handle Collision
                    loggerManager->increment(Metric::Collisions_S);
                    DEBUG_PRINTLN("[RadioBase] Possible collision detected!");
                }
            }
        }

        isReceivingVar = false;
        lastPreambleTime = 0;
        startReceive();
    }
}

void RadioBase::setReceivingVar(bool s)
{
    isReceivingVar = false;
}

void RadioBase::startReceive()
{
    radio->startReceive();
}

bool RadioBase::isReceiving()
{
    return isReceivingVar;
}
bool RadioBase::isTransmitting()
{
    return isTransmittingVar;
}

float RadioBase::getRSSI()
{
    return radio->getRSSI();
}

float RadioBase::getSNR()
{
    return radio->getSNR();
}

void RadioBase::standby()
{
    radio->standby();
}

size_t RadioBase::getPacketLength()
{
    return radio->getPacketLength();
}
int RadioBase::readData(uint8_t *data, size_t len)
{
    return radio->readData(data, len);
}

void RadioBase::sendPacket(const uint8_t *data, const size_t len)
{
    isTransmittingVar = true;
    int state = radio->sendRaw(data, len);

    if (state != RADIOLIB_ERR_NONE)
    {
        DEBUG_PRINTF("[RadioBase] Send failed: %d\n", state);
        isTransmittingVar = false;
        startReceive();
    }
    else
    {
        logSendStatistics(data, len);
    }
}

void RadioBase::logSendStatistics(const uint8_t *data, const size_t len)
{
    DEBUG_PRINTLN("[Mac Base] Log send");
    double timeOnAir = getToAByPacketSizeInUS(len);
    loggerManager->increment(Metric::TimeOnAir_S, timeOnAir);

    SentBytes_data sentBytes = SentBytes_data();
    sentBytes.bytes = len;
    loggerManager->log(Metric::SentBytes_V, sentBytes);
    DEBUG_PRINTF("[Mac Base] Log sent bytes: %d\n", sentBytes);

    SentEffectiveBytes_data sentEffectiveBytes = SentEffectiveBytes_data();
    if ((data[0] & 0x7F) == MESSAGE_TYPE_BROADCAST_FRAGMENT)
    {
        sentEffectiveBytes.bytes = len - BROADCAST_FRAGMENT_METADATA_SIZE;
        loggerManager->log(Metric::SentEffectiveBytes_V, sentEffectiveBytes);
        DEBUG_PRINTF("[Mac Base] Log sent effective bytes: %d\n", len - BROADCAST_FRAGMENT_METADATA_SIZE);
    }

    if ((data[0] & 0x7F) == MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT)
    {
        sentEffectiveBytes.bytes = len - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE;
        loggerManager->log(Metric::SentEffectiveBytes_V, sentEffectiveBytes);
        DEBUG_PRINTF("[Mac Base] Log sent effective bytes: %d\n", len - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE);
    }

    SentMissionRTS_data sentMissionRTS = SentMissionRTS_data();
    if (data[0] == (MESSAGE_TYPE_BROADCAST_RTS | 0x80))
    {
        sentMissionRTS.time = time(NULL);
        sentMissionRTS.missionId = ((uint16_t)data[1] << 8) | data[2]; //  second and thrird bytes is the id
        sentMissionRTS.source = data[3];                               // fourth byte is the source
        loggerManager->log(Metric::SentMissionRTS_V, sentMissionRTS);
        DEBUG_PRINTF("[Mac Base] Log sent mission RTS with id: %d\n", ((uint16_t)data[1] << 8) | data[2]);
    }

    SentMissionFragment_data sentMissionFragment = SentMissionFragment_data();
    if (data[0] == (MESSAGE_TYPE_BROADCAST_FRAGMENT | 0x80))
    {
        sentMissionFragment.time = time(NULL);
        sentMissionFragment.missionId = ((uint16_t)data[1] << 8) | data[2]; //  second and third bytes is the source
        sentMissionFragment.source = data[3];                               // fourth byte is the source
        loggerManager->log(Metric::SentMissionFragment_V, sentMissionFragment);
        DEBUG_PRINTF("[Mac Base] Log sent mission fragment with id: %d\n", ((uint16_t)data[1] << 8) | data[2]);
    }
    DEBUG_PRINTLN("[Mac Base] Finish log send");
}