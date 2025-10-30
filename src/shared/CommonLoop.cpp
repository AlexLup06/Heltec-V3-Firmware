#include "shared/Common.h"

void commonLoop()
{
    button.update();

    if (configurator.isInConfigMode())
    {
        configurator.handleConfigMode();
        return;
    }

    if (!isInWaitMode)
    {
        macProtocol->handle();
        messageSimulator.simulateMessages();
    }

    updateMacController();

    if (messageSimulator.packetReady)
    {
        macProtocol->handleUpperPacket(messageSimulator.messageToSend);
        messageSimulator.cleanUp();
    }

    yield();
}
