#include "shared/Common.h"

void commonLoop()
{
    loraDisplay.loop();

    if (configurator.isInConfigMode())
    {
        configurator.handleConfigMode();
        return;
    }

    if (isInWaitMode)
    {
        macProtocol->handle();
        messageSimulator.simulateMessages();
    }

    updateMacController();

    if (messageSimulator.packetReady)
    {
        meshRouter.handleUpperPacket(messageSimulator.messageToSend);
        messageSimulator.packetReady = false;
    }
}
