#include "shared/Common.h"

void commonLoop()
{
    if (macController.finishedAllRuns())
    {
        return;
    }

    button.update();

    if (configurator.isInConfigMode())
    {
        configurator.handleConfigMode();
        return;
    }

    if (!macController.isInWaitMode())
    {
        macProtocol->handle();
        messageSimulator.simulateMessages();
    }

    macController.update();

    if (messageSimulator.packetReady)
    {
        macProtocol->handleUpperPacket(messageSimulator.messageToSend);
        messageSimulator.cleanUp();
    }

    yield();
}
