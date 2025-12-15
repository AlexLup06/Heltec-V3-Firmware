#pragma once
#include <Arduino.h>
#include <cstring>
#include <algorithm>
#include "functions.h"

class SelfMessage
{
public:
    explicit SelfMessage(const char *name = "default")
        : name_(name), triggerTime(0), scheduled(false) {}

    bool isName(const char *n) const { return strcmp(name_, n) == 0; }
    const char *getName() const { return name_; }

    double getTriggerTime() const { return triggerTime; }
    bool isScheduled() const { return scheduled; }

    bool operator==(const SelfMessage &other) const
    {
        return strcmp(name_, other.name_) == 0;
    }

    bool operator!=(const SelfMessage &other) const
    {
        return !(*this == other);
    }

private:
    friend class SelfMessageScheduler;
    const char *name_;
    double triggerTime;
    bool scheduled;
};
