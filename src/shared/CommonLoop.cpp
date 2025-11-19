#include "shared/Common.h"

void commonLoop()
{
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
        loraDisplay.loop();
        return;
    }

    yield();
}
