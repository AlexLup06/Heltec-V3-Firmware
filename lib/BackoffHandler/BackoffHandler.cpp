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

    DEBUG_PRINTF("[BackoffHandler] Backoff duration: %d\n", backoffPeriod_MS);
    scheduler_->schedule(endBackoff, backoffPeriod_MS);
}

void BackoffHandler::decreaseBackoffPeriod()
{
    long remaining = (long)endBackoff->getTriggerTime() - (long)millis();
    if (remaining < 0)
    {
        remaining = 0;
    }
    long elapsed = backoffPeriod_MS - remaining;
    backoffPeriod_MS -= ((int)(elapsed / backoffFS_MS)) * backoffFS_MS;
    DEBUG_PRINTF("[BackoffHandler] Decrease by: %d\n", ((int)(elapsed / backoffFS_MS)) * backoffFS_MS);

    if (backoffPeriod_MS < 0)
        backoffPeriod_MS = 0;
}

void BackoffHandler::cancelBackoffTimer()
{
    scheduler_->cancel(endBackoff);
}
