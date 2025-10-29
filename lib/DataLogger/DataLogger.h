#pragma once
#include <Arduino.h>
#include <vector>
#include <FS.h>
#include <LittleFS.h>
#include "FileHeader.h"
#include "functions.h"

template <typename T>
class DataLogger
{
public:
    DataLogger(const char *filename = nullptr, const FileHeader *header = nullptr);

    void addDataPoint(const T &dp);
    bool saveToDisk();
    void clear();

    const std::vector<T> &getBuffer() const { return buffer; }
    void setHeader(const FileHeader &_header) { header = _header; }

private:
    std::vector<T> buffer;
    String filename;
    FileHeader header;
    bool headerWritten = false;
};
#include "DataLogger.tpp"
