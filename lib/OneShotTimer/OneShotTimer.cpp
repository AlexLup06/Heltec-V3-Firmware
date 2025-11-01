#include "OneShotTimer.h"

void OneShotTimer::schedule(unsigned long delayMs)
{
    triggerTime = millis() + delayMs;
    active = true;
}

void OneShotTimer::cancel()
{
    active = false;
}

bool OneShotTimer::triggered()
{
    if (active && (long)(millis() - triggerTime) >= 0)
    {
        active = false;
        return true;
    }
    return false;
}

bool OneShotTimer::isScheduled() const
{
    if (!active)
        return false;

    if ((long)(millis() - triggerTime) >= 0)
        return false;
    return true;
}
