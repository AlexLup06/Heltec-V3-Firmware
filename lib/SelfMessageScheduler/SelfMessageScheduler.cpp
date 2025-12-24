#include "SelfMessageScheduler.h"

void SelfMessageScheduler::schedule(SelfMessage *msg, double delayMs)
{
    double t = millis() + delayMs;

    for (auto it = messages.begin(); it != messages.end(); ++it)
    {
        if (*it == msg)
        {
            (*it)->triggerTime = t;
            (*it)->scheduled = true;
            messages.erase(it);
            break;
        }
    }

    msg->triggerTime = t;
    msg->scheduled = true;

    auto insertPos = std::lower_bound(
        messages.begin(),
        messages.end(),
        msg,
        [](const SelfMessage *a, const SelfMessage *b)
        {
            return a->triggerTime < b->triggerTime;
        });

    messages.insert(insertPos, msg);
}

void SelfMessageScheduler::scheduleOrExtend(SelfMessage *msg, double delayMs)
{
    double now = millis();
    double newTrigger = now + delayMs;

    if (!msg->scheduled)
    {
        schedule(msg, delayMs);
        return;
    }

    if (newTrigger - msg->triggerTime <= 0) {
        return;
    }

    for (auto it = messages.begin(); it != messages.end(); ++it)
    {
        if (*it == msg)
        {
            messages.erase(it);
            break;
        }
    }

    msg->triggerTime = newTrigger;

    auto insertPos = std::lower_bound(
        messages.begin(),
        messages.end(),
        msg,
        [](const SelfMessage *a, const SelfMessage *b)
        {
            return a->triggerTime < b->triggerTime;
        });

    messages.insert(insertPos, msg);
}

void SelfMessageScheduler::cancel(SelfMessage *msg)
{
    for (auto it = messages.begin(); it != messages.end(); ++it)
    {
        if (*it == msg)
        {
            (*it)->scheduled = false;
            messages.erase(it);
            return;
        }
    }
}

bool SelfMessageScheduler::isScheduled(SelfMessage *msg) const
{
    double now = millis();
    for (auto *m : messages)
    {
        if (m == msg && m->scheduled && now - m->triggerTime < 0)
            return true;
    }
    return false;
}

SelfMessage *SelfMessageScheduler::popNextReady()
{
    if (messages.empty())
        return nullptr;

    double now = millis();
    SelfMessage *m = messages.front();

    if (m->scheduled && now - m->triggerTime >= 0)
    {
        m->scheduled = false;
        messages.erase(messages.begin());
        return m;
    }

    return nullptr;
}

void SelfMessageScheduler::clear()
{
    messages.clear();
}
