#pragma once
#include <Arduino.h>
#include <vector>
#include "SelfMessage.h"

class SelfMessageScheduler
{
public:
    void schedule(SelfMessage *msg, unsigned long delayMs);
    void scheduleOrExtend(SelfMessage *msg, unsigned long delayMs);
    void cancel(SelfMessage *msg);
    bool isScheduled(SelfMessage *msg) const;
    SelfMessage *popNextReady();
    void clear();

private:
    std::vector<SelfMessage *> messages;
};
