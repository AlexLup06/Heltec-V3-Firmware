#include "LoggerManager.h"

void LoggerManager::makeFilename(Metric metric, char *out, size_t len)
{
    snprintf(out, len, "/%s/nodes-%d/%s-%s-%s-%d-%d-%d-vec-run%02d.bin",
             networkName,
             numberOfNodes,
             metricToString(metric), currentMac, networkName, missionMessagesPerMin, numberOfNodes, nodeId, runNumber);
}

FileHeader LoggerManager::makeHeader(Metric metric)
{
    FileHeader header;
    header.metric = (uint8_t)metric;
    strncpy(header.networkName, networkName, sizeof(header.networkName) - 1);
    strncpy(header.currentMac, currentMac, sizeof(header.currentMac) - 1);
    header.missionMessagesPerMin = missionMessagesPerMin;
    header.numberOfNodes = numberOfNodes;
    header.runNumber = runNumber;
    header.timestamp = millis();
    header.nodeId = nodeId;

    return header;
}

void LoggerManager::init(uint8_t _nodeId)
{

    nodeId = _nodeId;

    char path[64];

    // Top-level folders
    snprintf(path, sizeof(path), "/%s", networkIdToString(TOPOLOGY_FULLY_MESHED));
    createDirChecked(path);

    snprintf(path, sizeof(path), "/%s", networkIdToString(TOPOLOGY_COMPLEX));
    createDirChecked(path);

    snprintf(path, sizeof(path), "/%s", networkIdToString(TOPOLOGY_LINE));
    createDirChecked(path);

    for (int i = 1; i <= MAX_NUMBER_OF_NODES; i++)
    {
        snprintf(path, sizeof(path), "/%s/nodes-%d",
                 networkIdToString(TOPOLOGY_FULLY_MESHED), i);
        createDirChecked(path);

        snprintf(path, sizeof(path), "/%s/nodes-%d",
                 networkIdToString(TOPOLOGY_LINE), i);
        createDirChecked(path);

        snprintf(path, sizeof(path), "/%s/nodes-%d",
                 networkIdToString(TOPOLOGY_COMPLEX), i);
        createDirChecked(path);
    }

    registerLogger<ReceivedCompleteMission_data>(Metric::ReceivedCompleteMission_V);
    registerLogger<SentMissionRTS_data>(Metric::SentMissionRTS_V);
    registerLogger<ReceivedMissionIdFragment_data>(Metric::ReceivedMissionIdFragment_V);
    registerLogger<ReceivedNeighbourIdFragment_data>(Metric::ReceivedNeighbourIdFragment_V);
}

void LoggerManager::updateFilename()
{
    for (auto &[metric, ptr] : loggers)
    {
        char filename[128];
        makeFilename(metric, filename, sizeof(filename));
        FileHeader header = makeHeader(metric);
        ptr->updateFilename(filename, header);
    }
}

void LoggerManager::saveAll()
{
    for (auto &[metric, ptr] : loggers)
    {
        ptr->save();
    }
}

void LoggerManager::clearAll()
{
    for (auto &[metric, ptr] : loggers)
        ptr->clear();
}

void LoggerManager::setMetadata(int _runNumber, const char *_newMac, int _missionMessagesPerMin)
{
    runNumber = _runNumber;
    strncpy(currentMac, _newMac, sizeof(currentMac) - 1);
    currentMac[sizeof(currentMac) - 1] = '\0';
    missionMessagesPerMin = _missionMessagesPerMin;
    DEBUG_PRINTF("[LoggerManager] Setting new Metadata, runNumber: %d, currentMac: %s, missionMessagesPerMin: %d\n", _runNumber, _newMac, _missionMessagesPerMin);
}

void LoggerManager::setNetworkTopology(const char *_networkName, int _numberOfNodes)
{
    strncpy(networkName, _networkName, sizeof(networkName) - 1);
    networkName[sizeof(networkName) - 1] = '\0';
    numberOfNodes = _numberOfNodes;
    DEBUG_PRINTF("[LoggerManager] Setting new Toplogy, newtorkId: %s, numberOfNodes: %d\n", networkName, numberOfNodes);
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
    char filenameBuf[128];
    snprintf(filenameBuf, sizeof(filenameBuf), "/%s/nodes-%d/counter-%s-%s-%d-%d-%d-vec-run%02d.bin",
             networkName,
             numberOfNodes,
             currentMac, networkName, missionMessagesPerMin, numberOfNodes, nodeId, runNumber);

    File file = LittleFS.open(filenameBuf, FILE_WRITE);
    if (!file)
    {
        DEBUG_PRINTF("Failed to open %s\n", filenameBuf);
        return;
    }

    FileHeader header = makeHeader(Metric::SingleValues);
    header.entrySize = sizeof(Metric) + sizeof(double);
    file.write((uint8_t *)&header, sizeof(header));

    for (const auto &entry : counters)
    {
        const char *name = metricToString(entry.first);
        double value = entry.second;
        file.printf("%s = %.6f\n", name, value);
    }

    file.close();
    DEBUG_PRINTF("Saved %u counters to %s\n", (unsigned)counters.size(), filenameBuf);
}