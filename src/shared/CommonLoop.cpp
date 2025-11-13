#include "shared/Common.h"

void commonLoop()
{
    button.update();

    if (button.inputActive)
    {
        return;
    }

    if (macController.finishedAllRuns())
    {
        return;
    }

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
