#include "RadioBase.h"

void RadioBase::assignRadio(SX1262Public *_radio)
{
    radio = _radio;
}

void RadioBase::finishRadioBase()
{
    isReceivingVar = false;
    isTransmittingVar = false;
    irqFlag = 0b0;
}

void RadioBase::receiveDio1Interrupt()
{
    uint16_t irq = radio->getIrqFlags();

    if (irq & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
        irqFlag |= RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED;

    if (irq & RADIOLIB_SX126X_IRQ_RX_DONE)
        irqFlag |= RADIOLIB_SX126X_IRQ_RX_DONE;

    if (irq & RADIOLIB_SX126X_IRQ_CRC_ERR)
        irqFlag |= RADIOLIB_SX126X_IRQ_CRC_ERR;

    if (irq & RADIOLIB_SX126X_IRQ_HEADER_ERR)
        irqFlag |= RADIOLIB_SX126X_IRQ_HEADER_ERR;

    const uint16_t RX_IRQ_MASK =
        RADIOLIB_SX126X_IRQ_RX_DONE |
        RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED |
        RADIOLIB_SX126X_IRQ_HEADER_ERR |
        RADIOLIB_SX126X_IRQ_CRC_ERR;

    uint16_t relevant = irq & RX_IRQ_MASK;
    if (relevant)
        radio->clearIrqFlags(relevant);
}

void RadioBase::handleDio1Interrupt()
{
    uint16_t flags = irqFlag;
    irqFlag = 0;
    if (flags & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
    {
        isReceivingVar = true;
        onPreambleDetectedIR();
    }

    if (flags & (RADIOLIB_SX126X_IRQ_RX_DONE |
                 RADIOLIB_SX126X_IRQ_HEADER_ERR |
                 RADIOLIB_SX126X_IRQ_CRC_ERR))
    {
        isReceivingVar = false;
        if (flags & RADIOLIB_SX126X_IRQ_RX_DONE)
        {
            onReceiveIR();
        }

        startReceive();
    }
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
    radio->sendRaw(data, len);
    isTransmittingVar = false;
    startReceive();
}
