#include "BackoffHandler.h"
#include <assert.h>

void BackoffHandler::invalidateBackoffPeriod()
{
    backoffPeriod_MS = -1;
}

bool BackoffHandler::isInvalidBackoffPeriod() const
{
    return backoffPeriod_MS == -1;
}

void BackoffHandler::generateBackoffPeriod()
{
    int slots = random(1, cwBackoff + 1);    // random in [1, cw]
    backoffPeriod_MS = slots * backoffFS_MS; // total delay
    remainderCW = cwBackoff - slots;
}

void BackoffHandler::scheduleBackoffTimer()
{
    if (isInvalidBackoffPeriod())
        generateBackoffPeriod();

    scheduler_->schedule(endBackoff, backoffPeriod_MS);
}

void BackoffHandler::decreaseBackoffPeriod()
{
    unsigned long elapsed = millis() - endBackoff->getTriggerTime();
    backoffPeriod_MS -= ((int)(elapsed / backoffFS_MS)) * backoffFS_MS;

    if (backoffPeriod_MS < 0)
        backoffPeriod_MS = 0;

    assert(backoffPeriod_MS >= 0);
}

void BackoffHandler::cancelBackoffTimer()
{
    scheduler_->cancel(endBackoff);
}
