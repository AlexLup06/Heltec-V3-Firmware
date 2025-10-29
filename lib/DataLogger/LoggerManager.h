#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <map>
#include <memory>
#include "DataLogger.h"
#include "DataLogger.tpp"
#include "Metrics.h"
#include "FileHeader.h"
#include "functions.h"

class LoggerManager
{
public:
    LoggerManager(const String &topology, int startRun = 0);

    void init();
    void clearAll();
    void saveAll();
    void nextRun();
    void setTopology(const String &topo);
    int getRunNumber() const { return runNumber; }

    void increment(Metric metric, double value = 1.0);
    void setValue(Metric metric, double value);
    double getValue(Metric metric) const;
    void resetCounters();
    void saveCounters();

    template <typename T>
    void log(Metric metric, const T &data);

private:
    String topology;
    int runNumber;

    struct BaseLogger
    {
        virtual void add(void *data) = 0;
        virtual void save() = 0;
        virtual void clear() = 0;
        virtual void updateFilename(const String &filename, const FileHeader &header) = 0;
        virtual ~BaseLogger() {}
    };

    template <typename T>
    struct TypedLogger : BaseLogger
    {
        DataLogger<T> logger;
        String currentFile;
        TypedLogger(const String &filename, const FileHeader &header)
            : logger(filename.c_str(), &header), currentFile(filename) {}

        void add(void *data) override { logger.addDataPoint(*reinterpret_cast<T *>(data)); }
        void save() override { logger.saveToDisk(); }
        void clear() override { logger.clear(); }
        void updateFilename(const String &filename, const FileHeader &header) override
        {
            currentFile = filename;
            logger = DataLogger<T>(filename.c_str(), &header);
        }
    };

    std::map<Metric, std::shared_ptr<BaseLogger>> loggers;
    std::map<Metric, double> counters;

    template <typename T>
    void registerLogger(Metric metric);

    String makeFilename(Metric metric);
    FileHeader makeHeader(Metric metric);
};
#include "LoggerManager.tpp"
