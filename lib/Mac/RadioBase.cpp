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

void RadioBase::handleIRQFlags()
{
    uint16_t flags = radio->irqFlag;
    radio->irqFlag = 0;

    if (flags & RADIOLIB_SX126X_IRQ_TX_DONE)
    {
        DEBUG_PRINTLN("[RadioBase] RADIOLIB_SX126X_IRQ_TX_DONE");
        isTransmittingVar = false;
        onSendCallback();
        uint64_t t0 = esp_timer_get_time();
        startReceive();
        uint64_t t1 = esp_timer_get_time();
        DEBUG_PRINTF("[Radio] startReceive took %llu us\n", t1 - t0);
        return;
    }

    if (flags & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
    {
        if (!isReceivingVar)
        {
            DEBUG_PRINTLN("[RadioBase] got preamble");
            isReceivingVar = true;
            lastPreambleTime = millis();
            onPreambleDetectedIR();
        }
    }

    if (isReceivingVar && (millis() - lastPreambleTime) > MAX_PACKET_TIME && lastPreambleTime != 0)
    {
        isReceivingVar = false;
        DEBUG_PRINTLN("[RadioBase] Preamble Timeout");
        preambleTimedOutFlag = true;
        startReceive();
    }

    if (flags & (RADIOLIB_SX126X_IRQ_RX_DONE |
                 RADIOLIB_SX126X_IRQ_HEADER_ERR |
                 RADIOLIB_SX126X_IRQ_CRC_ERR))
    {
        if ((flags & RADIOLIB_SX126X_IRQ_RX_DONE) != 0 &&
            (flags & (RADIOLIB_SX126X_IRQ_HEADER_ERR | RADIOLIB_SX126X_IRQ_CRC_ERR)) == 0)
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

bool RadioBase::hasPreambleTimedOut() const
{
    return preambleTimedOutFlag;
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
    double timeOnAir = getToAByPacketSizeInUS(len) / 1000.0;
    loggerManager->increment(Metric::TimeOnAir_S, timeOnAir);

    loggerManager->increment(Metric::SentBytes_S, len);
    DEBUG_PRINTF("[Mac Base] Log sent bytes: %d\n", len);

    if ((data[0] & 0x7F) == MESSAGE_TYPE_BROADCAST_FRAGMENT)
    {
        loggerManager->increment(Metric::SentEffectiveBytes_S, len - BROADCAST_FRAGMENT_METADATA_SIZE);
        DEBUG_PRINTF("[Mac Base] Log sent effective bytes: %d\n", len - BROADCAST_FRAGMENT_METADATA_SIZE);
    }

    if ((data[0] & 0x7F) == MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT)
    {
        loggerManager->increment(Metric::SentEffectiveBytes_S, len - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE);
        DEBUG_PRINTF("[Mac Base] Log sent effective bytes: %d\n", len - BROADCAST_LEADER_FRAGMENT_METADATA_SIZE);
    }

    SentMissionRTS_data sentMissionRTS = SentMissionRTS_data();
    if ((data[0] == (MESSAGE_TYPE_BROADCAST_RTS | 0x80)) ||
        (data[0] == (MESSAGE_TYPE_BROADCAST_LEADER_FRAGMENT | 0x80)))
    {
        sentMissionRTS.time = time(NULL);
        sentMissionRTS.missionId = ((uint16_t)data[2] << 8) | (uint16_t)data[1]; //  second and thrird bytes is the id
        sentMissionRTS.source = data[3];                                         // fourth byte is the source
        loggerManager->log(Metric::SentMissionRTS_V, sentMissionRTS);
        DEBUG_PRINTF("[Mac Base] Log sent mission RTS with id: %d\n", ((uint16_t)data[2] << 8) | (uint16_t)data[1]);
    }
    DEBUG_PRINTLN("[Mac Base] Finish log send");
}