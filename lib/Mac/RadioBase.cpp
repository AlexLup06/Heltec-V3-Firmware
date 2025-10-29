#include "RadioBase.h"

void RadioBase::assignRadio(SX1262Public *_radio)
{
    radio = _radio;
}

void RadioBase::receiveDio1Interrupt()
{
    uint16_t irq = radio->getIrqFlags();
    radio->clearIrqFlags(irq);

    if (irq & RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)
    {
        isReceivingVar = true;
        onPreambleDetectedIR();
    }

    if (irq & RADIOLIB_SX126X_IRQ_RX_DONE)
    {
        onReceiveIR();
        isReceivingVar = false;
    }

    if (irq & RADIOLIB_SX126X_IRQ_CRC_ERR)
        onCRCerrorIR();
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
