#pragma once
#include <Arduino.h>
#include <algorithm> // for std::lower_bound
#include "functions.h"

class SelfMessage
{
public:
    explicit SelfMessage(const String &name = "default")
        : name_(name), triggerTime(0), scheduled(false) {}

    bool isName(const String &n) const { return name_ == n; }
    const String &getName() const { return name_; }

    unsigned long getTriggerTime() const { return triggerTime; }
    bool isScheduled() const { return scheduled; }

    bool operator==(const SelfMessage &other) const
    {
        return name_ == other.name_;
    }

    bool operator!=(const SelfMessage &other) const
    {
        return !(*this == other);
    }

private:
    friend class SelfMessageScheduler;
    String name_;
    unsigned long triggerTime;
    bool scheduled;
};
