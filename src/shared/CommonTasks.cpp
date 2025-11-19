#include "shared/CommonGlobals.h"

void macTask(void *param)
{
    for (;;)
    {
        if (button.inputActive)
        {
            vTaskDelay(2);
            continue;
        }

        if (macController.finishedAllRuns())
        {
            vTaskDelay(3);
            continue;
        }

        if (configurator.isInConfigMode())
        {
            vTaskDelay(2);
            continue;
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

        vTaskDelay(2);
    }
}

void radioIrqTask(void *param)
{
    SX1262Public *self = static_cast<SX1262Public *>(param);

    for (;;)
    {
        if (self->dio1Flag)
        {
            self->dio1Flag = false;
            self->handleDio1Interrupt();
        }

        vTaskDelay(1);
    }
}

void buttonTask(void *param)
{
    for (;;)
    {
        button.update();
        vTaskDelay(1);
    }
}
