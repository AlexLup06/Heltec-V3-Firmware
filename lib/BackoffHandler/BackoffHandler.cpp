#include "BackoffHandler.h"

bool Backoff::finished()
{
    if (done)
        return true;
    if (backoffTimer.triggered())
    {
        done = true;
        return true;
    }
    return false;
}

void Backoff::invalidate()
{
    done = false;
    backoffPeriod_MS = -1;
}

bool Backoff::isValidBackoffPeriod()
{
    return backoffPeriod_MS != -1;
}

void Backoff::generateBackoffPeriod()
{
    int slots = random(1, cw + 1);
    backoffPeriod_MS = slots * backoffFS_MS;
}

void Backoff::schedule()
{
    if (!isValidBackoffPeriod())
        generateBackoffPeriod();
    backoffTimer.schedule(backoffPeriod_MS);
}

void Backoff::decrease()
{
    long elapsedBackoffTime = millis() - backoffTimer.getTriggerTime();
    backoffPeriod_MS -= ((int)(elapsedBackoffTime / backoffFS_MS)) * backoffFS_MS;
    assert(backoffPeriod_MS >= 0);
}

void Backoff::cancel()
{
    backoffTimer.cancel();
}