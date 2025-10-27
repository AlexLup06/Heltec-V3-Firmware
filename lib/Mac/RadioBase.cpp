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

void RadioBase::initRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw)
{
    SPI.begin();

    int state = radio->begin(frequency);
    if (state != RADIOLIB_ERR_NONE)
    {
        DEBUG_LORA_SERIAL("Radio init failed: %d\n", state);
        while (true)
            ;
    }

    radio->setSpreadingFactor(sf);
    radio->setOutputPower(txPower);
    radio->setBandwidth(bw);
    radio->setCodingRate(5);
    radio->setSyncWord(0x12);
    radio->setPreambleLength(8);
}

void RadioBase::reInitRadio(float frequency, uint8_t sf, uint8_t txPower, uint32_t bw)
{
    radio->standby();
    vTaskDelay(pdMS_TO_TICKS(50));

    int state = radio->begin(frequency);
    if (state != RADIOLIB_ERR_NONE)
    {
        DEBUG_LORA_SERIAL("SX1262 re-init failed! Code: %d\n", state);
        while (true)
            ;
    }

    radio->setOutputPower(txPower);
    radio->setBandwidth(bw);
    radio->setSpreadingFactor(sf);
    radio->setCodingRate(5);
    radio->setPreambleLength(12);
    radio->setSyncWord(0x12);

    radio->startReceive();

    DEBUG_LORA_SERIAL("SX1262 re-initialized successfully!");
}

void RadioBase::startReceive()
{
    radio->startReceive();
}

void RadioBase::sendPacket(const uint8_t *data, const size_t len)
{
    isTransmittingVar = true;
    int state = radio->transmit(data, len);
    if (state == RADIOLIB_ERR_NONE)
    {
        DEBUG_LORA_SERIAL("Packet sent!");
    }
    else
    {
        DEBUG_LORA_SERIAL("Send failed: %d\n", state);
    }
    isTransmittingVar = false;
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