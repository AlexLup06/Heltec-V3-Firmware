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
    int slot = random(0, cwBackoff);
    backoffPeriod_MS = (double)(slot * backoffFS_MS) + (double)random(0, 3000) / 1000.0 + 6; // + 6 because tx->rx takes 6ms
    remainderCW = cwBackoff - slot - 1;
    chosenSlot = slot;
}

void BackoffHandler::scheduleBackoffTimer()
{
    if (isInvalidBackoffPeriod())
        generateBackoffPeriod();

    DEBUG_PRINTF("[BackoffHandler] Backoff duration: %.3f\n", backoffPeriod_MS);
    scheduler_->schedule(endBackoff, backoffPeriod_MS);
}

void BackoffHandler::decreaseBackoffPeriod()
{
    double remaining = endBackoff->getTriggerTime() - (double)millis();
    if (remaining < 0)
    {
        remaining = 0;
    }
    double elapsed = backoffPeriod_MS - remaining;
    backoffPeriod_MS -= (double)(((int)(elapsed / backoffFS_MS)) * backoffFS_MS);
    backoffPeriod_MS += 6 + (double)random(0, 3000) / 1000.0;

    if (backoffPeriod_MS < 0)
        backoffPeriod_MS = 6 + (double)random(0, 3000) / 1000.0;
}

void BackoffHandler::cancelBackoffTimer()
{
    scheduler_->cancel(endBackoff);
}
