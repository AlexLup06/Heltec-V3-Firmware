#include "shared/CommonGlobals.h"

void macTask(void *param)
{
    for (;;)
    {
        if (button.inputActive)
        {
            vTaskDelay(20);
            continue;
        }

        if (macController.finishedAllRuns())
        {
            vTaskDelay(30);
            continue;
        }

        if (configurator.isInConfigMode())
        {
            configurator.handleConfigMode();
            loraDisplay.loop();
            vTaskDelay(10);
            continue;
        }

        macController.update();

        if (!macController.isInWaitMode())
        {
            if (macProtocol->shouldHandlePacket())
            {
                messageSimulator.simulateMessages();
            }

            if (messageSimulator.packetReady)
            {
                macProtocol->handleUpperPacket(messageSimulator.messageToSend);
                messageSimulator.cleanUp();
            }
            macProtocol->handle();
        }

        vTaskDelay(0);
    }
}

void radioIrqTask(void *param)
{
    SX1262Public *self = static_cast<SX1262Public *>(param);
    uint8_t evt;

    for (;;)
    {
        if (xQueueReceive(self->irqQueue, &evt, portMAX_DELAY))
        {
            if (!macController.isInWaitMode())
            {
                self->handleDio1Interrupt();
            }
        }
    }
}

void buttonTask(void *param)
{
    for (;;)
    {
        // button.update();
        vTaskDelay(10);
    }
}
