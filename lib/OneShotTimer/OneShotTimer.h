#pragma once
#include <Arduino.h>

class OneShotTimer
{
private:
    bool active = false;
    unsigned long triggerTime = 0;

public:
    void schedule(unsigned long delayMs);
    void cancel();
    bool triggered();
    bool isScheduled() const;
    unsigned long getTriggerTime() const { return triggerTime; }
};
