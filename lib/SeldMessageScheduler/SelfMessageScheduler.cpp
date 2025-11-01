#include "SelfMessageScheduler.h"
#include <algorithm> // for std::lower_bound

void SelfMessageScheduler::schedule(SelfMessage *msg, unsigned long delayMs)
{
    unsigned long t = millis() + delayMs;

    // If message already scheduled, update its trigger time and reinsert
    for (auto it = messages.begin(); it != messages.end(); ++it)
    {
        if (*it == msg)
        {
            (*it)->triggerTime = t;
            (*it)->scheduled = true;
            // remove and reinsert in correct place
            messages.erase(it);
            break;
        }
    }

    msg->triggerTime = t;
    msg->scheduled = true;

    // find correct insertion point (sorted by triggerTime)
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
    unsigned long now = millis();
    for (auto *m : messages)
    {
        if (m == msg && m->scheduled && (long)(now - m->triggerTime) < 0)
            return true;
    }
    return false;
}

SelfMessage *SelfMessageScheduler::popNextReady()
{
    if (messages.empty())
        return nullptr;

    unsigned long now = millis();
    SelfMessage *m = messages.front();

    if (m->scheduled && (long)(now - m->triggerTime) >= 0)
    {
        m->scheduled = false;
        messages.erase(messages.begin());
        return m;
    }

    return nullptr; // nothing ready yet
}

void SelfMessageScheduler::clear()
{
    messages.clear();
}
