#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <map>
#include <memory>
#include "DataLogger.h"
#include "DataLogger.tpp"
#include "Metrics.h"
#include "FileHeader.h"
#include "filesystemOperations.h"

class LoggerManager
{
public:
    LoggerManager() : networkName(""), currentMac(""), missionMessagesPerMin(0), numberOfNodes(0), runNumber(0) {}
    void init(uint8_t _nodeId);

    void clearAll();
    void saveAll();
    void updateFilename();
    void setNetworkTopology(const char *_networkId, int _numberOfNodes);
    void setMetadata(int _runNumber, const char *_newMac, int _missionMessagesPerMin);

    void increment(Metric metric, double value = 1.0);
    void setValue(Metric metric, double value);
    double getValue(Metric metric) const;
    void resetCounters();
    void saveCounters();

    template <typename T>
    void log(Metric metric, const T &data);

    char currentMac[16];
    int runNumber;
    int missionMessagesPerMin;

    int numberOfNodes;
    char networkName[32];

    uint8_t nodeId;

private:
    struct BaseLogger
    {
        virtual void add(void *data) = 0;
        virtual void save() = 0;
        virtual void clear() = 0;
        virtual void updateFilename(const char *filename, const FileHeader &header) = 0;
        virtual ~BaseLogger() {}
    };

    template <typename T>
    struct TypedLogger : BaseLogger
    {
        DataLogger<T> logger;
        char currentFile[128];

        TypedLogger(const char *filename, const FileHeader &header)
            : logger(filename, header)
        {
            // Safely copy filename into local buffer
            strncpy(currentFile, filename, sizeof(currentFile) - 1);
            currentFile[sizeof(currentFile) - 1] = '\0';
        }

        void add(void *data) override { logger.addDataPoint(*reinterpret_cast<T *>(data)); }
        void save() override { logger.saveToDisk(); }
        void clear() override { logger.clear(); }
        void updateFilename(const char *filename, const FileHeader &header) override
        {
            strncpy(currentFile, filename, sizeof(currentFile) - 1);
            currentFile[sizeof(currentFile) - 1] = '\0';
            // Reinitialize DataLogger with new file
            logger = DataLogger<T>(filename, header);
        }
    };

    std::map<Metric, std::shared_ptr<BaseLogger>> loggers;
    std::map<Metric, double> counters;

    template <typename T>
    void registerLogger(Metric metric);

    void makeFilename(Metric metric, char *out, size_t len);
    FileHeader makeHeader(Metric metric);
};

#include "LoggerManager.tpp"
