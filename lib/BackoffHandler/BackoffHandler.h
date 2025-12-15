#pragma once
#include <Arduino.h>
#include "SelfMessage.h"
#include "SelfMessageScheduler.h"

class BackoffHandler
{
public:
    BackoffHandler(int fs_ms, int cw, SelfMessageScheduler *scheduler, SelfMessage *msg)
        : backoffFS_MS(fs_ms),
          cwBackoff(cw),
          scheduler_(scheduler),
          endBackoff(msg),
          backoffPeriod_MS(-1),
          remainderCW(0),
          chosenSlot(0),
          cwBackoff_CONST(cw)
    {
    }

    void invalidateBackoffPeriod();
    bool isInvalidBackoffPeriod() const;
    void generateBackoffPeriod();
    void scheduleBackoffTimer();
    void decreaseBackoffPeriod();
    void cancelBackoffTimer();

    void increaseCW()
    {
        if (cwBackoff < cwBackoff_CONST * 2)
            cwBackoff = cwBackoff_CONST * 2;
    }
    void resetCw() { cwBackoff = cwBackoff_CONST; }
    void setCw(uint8_t newCw) { cwBackoff = newCw; }
    uint8_t getCw() { return cwBackoff; }

    double getBackoffPeriod() const { return backoffPeriod_MS; }
    int getRemainderCW() const { return remainderCW; }
    int getChosenSlot() const { return chosenSlot; }

private:
    int backoffFS_MS;
    int cwBackoff;
    const int cwBackoff_CONST;
    double backoffPeriod_MS;

    int chosenSlot;

    uint16_t remainderCW;

    SelfMessageScheduler *scheduler_;
    SelfMessage *endBackoff;
};
