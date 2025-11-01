#pragma once
#include <Arduino.h>
#include "SelfMessage.h"
#include "SelfMessageScheduler.h"

class BackoffHandler {
public:
    BackoffHandler(int fs_ms, int cw, SelfMessageScheduler* scheduler, SelfMessage* msg)
        : backoffFS_MS(fs_ms),
          cwBackoff(cw),
          scheduler_(scheduler),
          endBackoff(msg),
          backoffPeriod_MS(-1)
    {
    }

    void invalidateBackoffPeriod();
    bool isInvalidBackoffPeriod() const;
    void generateBackoffPeriod();
    void scheduleBackoffTimer();
    void decreaseBackoffPeriod();
    void cancelBackoffTimer();

    long getBackoffPeriod() const { return backoffPeriod_MS; }

private:
    const int backoffFS_MS;
    const int cwBackoff;
    long backoffPeriod_MS;

    SelfMessageScheduler* scheduler_;
    SelfMessage* endBackoff;
};
