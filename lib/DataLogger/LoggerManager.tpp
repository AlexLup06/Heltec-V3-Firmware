#pragma once
#include "LoggerManager.h"

template <typename T>
void LoggerManager::registerLogger(Metric metric)
{
    String filename = makeFilename(metric);
    FileHeader header = makeHeader(metric);
    loggers[metric] = std::make_shared<TypedLogger<T>>(filename, header);
}

template <typename T>
void LoggerManager::log(Metric metric, const T &data)
{
    auto it = loggers.find(metric);
    if (it == loggers.end())
        return;
    it->second->add((void *)&data);
}