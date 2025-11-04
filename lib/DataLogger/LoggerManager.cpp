#include "LoggerManager.h"

LoggerManager::LoggerManager(const String &topo, int startRun)
    : topology(topo), runNumber(startRun) {}

String LoggerManager::makeFilename(Metric metric)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "/%s_%s_run%02d.bin",
             metricToString(metric), topology.c_str(), runNumber);
    return String(buf);
}

FileHeader LoggerManager::makeHeader(Metric metric)
{
    FileHeader header;
    header.metric = metric;
    strncpy(header.topology, topology.c_str(), sizeof(header.topology) - 1);
    header.runNumber = runNumber;
    header.timestamp = millis();
    return header;
}

void LoggerManager::init()
{
    if (!LittleFS.begin(true))
    {
        DEBUG_PRINTLN("Failed to mount LittleFS!");
        return;
    }

    DEBUG_PRINTLN("LittleFS mounted successfully");

    registerLogger<ReceivedCompleteMission_data>(Metric::ReceivedCompleteMission_V);
    registerLogger<SentMissionRTS_data>(Metric::SentMissionRTS_V);
    registerLogger<SentMissionFragment_data>(Metric::SentMissionFragment_V);
}

void LoggerManager::saveAll()
{
    for (auto &[metric, ptr] : loggers)
        ptr->save();
}

void LoggerManager::clearAll()
{
    for (auto &[metric, ptr] : loggers)
        ptr->clear();
}

void LoggerManager::nextRun()
{
    runNumber++;
    for (auto &[metric, ptr] : loggers)
    {
        String newFile = makeFilename(metric);
        FileHeader header = makeHeader(metric);
        ptr->updateFilename(newFile, header);
    }
    clearAll();
    resetCounters();
    DEBUG_PRINTF("Started next run: %d\n", runNumber);
}

void LoggerManager::setTopology(const String &topo)
{
    topology = topo;
    for (auto &[metric, ptr] : loggers)
    {
        String newFile = makeFilename(metric);
        FileHeader header = makeHeader(metric);
        ptr->updateFilename(newFile, header);
    }
    DEBUG_PRINTF("Topology changed to: %s\n", topology.c_str());
}

void LoggerManager::increment(Metric metric, double value)
{
    counters[metric] += value;
}

void LoggerManager::setValue(Metric metric, double value)
{
    counters[metric] = value;
}

double LoggerManager::getValue(Metric metric) const
{
    auto it = counters.find(metric);
    return it != counters.end() ? it->second : 0.0;
}

void LoggerManager::resetCounters()
{
    counters.clear();
}

void LoggerManager::saveCounters()
{
    // Use a consistent name for counter metrics
    String filename = "/counters_" + topology + "_run" + String(runNumber) + ".bin";
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file)
    {
        DEBUG_PRINTF("Failed to open %s\n", filename.c_str());
        return;
    }

    // Write header first (consistent with other metrics)
    FileHeader header = makeHeader(Metric::SingleValues); // You can define this enum
    header.entrySize = sizeof(Metric) + sizeof(double);
    file.write((uint8_t *)&header, sizeof(header));

    // Write all metric/value pairs
    for (const auto &entry : counters)
    {
        const char *name = metricToString(entry.first);
        double value = entry.second;
        file.printf("%s = %.6f\n", name, value);
    }

    file.close();
    DEBUG_PRINTF("Saved %u counters to %s\n", (unsigned)counters.size(), filename.c_str());
}
