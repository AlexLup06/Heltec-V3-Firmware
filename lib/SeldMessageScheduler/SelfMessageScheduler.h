#pragma once
#include <Arduino.h>
#include <vector>
#include "SelfMessage.h"

class SelfMessageScheduler {
public:
    void schedule(SelfMessage* msg, unsigned long delayMs);
    void cancel(SelfMessage* msg);
    bool isScheduled(SelfMessage* msg) const;
    SelfMessage* popNextReady();     // returns nullptr if none ready
    void clear();                    // clears everything

private:
    std::vector<SelfMessage*> messages;
};
