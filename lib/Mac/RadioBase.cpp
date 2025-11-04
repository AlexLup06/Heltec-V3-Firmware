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
}
