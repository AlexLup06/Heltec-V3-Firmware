#pragma once
#include <Arduino.h>
#include "functions.h"
#include "OneShotTimer.h"

class Backoff
{
public:
    Backoff(int fs_ms, int _cw)
        : backoffFS_MS(fs_ms),
          cw(_cw),
          backoffPeriod_MS(-1)
    {
    }

    void schedule();
    bool finished();
    void cancel();
    void invalidate();
    void decrease();
    bool isRunning() const { return backoffTimer.isScheduled(); }

private:
    OneShotTimer backoffTimer;

    bool done = false;

    long backoffPeriod_MS;
    const int backoffFS_MS;
    const int cw;

    void generateBackoffPeriod();
    bool isValidBackoffPeriod();
};
