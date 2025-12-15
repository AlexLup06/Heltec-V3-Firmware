#include "shared/Common.h"

void commonLoop()
{
    if (configurator.isInConfigMode())
    {
        button.update();
    }
    yield();
}
