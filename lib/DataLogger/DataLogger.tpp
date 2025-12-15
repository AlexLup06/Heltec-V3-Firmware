#pragma once
#include "DataLogger.h"
#include <cstring>

template <typename T>
DataLogger<T>::DataLogger(const char *fname, const FileHeader &hdr)
    : header(hdr)
{
    headerWritten = false;
    strncpy(filename, fname ? fname : "", sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';
}

template <typename T>
void DataLogger<T>::addDataPoint(const T &dp)
{
    buffer.push_back(dp);
}

template <typename T>
bool DataLogger<T>::saveToDisk()
{
    vTaskDelay(pdMS_TO_TICKS(5));
    if (filename[0] == '\0')
    {
        DEBUG_PRINTLN("No filename set");
        return false;
    }

    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file)
    {
        DEBUG_PRINTF("Failed to open %s for writing\n", filename);
        return false;
    }

    if (!headerWritten)
    {
        header.entrySize = sizeof(T);
        file.write(reinterpret_cast<const uint8_t *>(&header), sizeof(FileHeader));
        headerWritten = true;
    }

    size_t bytes = file.write(reinterpret_cast<const uint8_t *>(buffer.data()),
                              buffer.size() * sizeof(T));
    file.close();
    vTaskDelay(pdMS_TO_TICKS(5));

    DEBUG_PRINTF("Saved %u entries + header to %s\n",
                 static_cast<unsigned>(buffer.size()), filename);

    return bytes == buffer.size() * sizeof(T);
}

template <typename T>
void DataLogger<T>::clear()
{
    buffer.clear();
    buffer.shrink_to_fit();
    headerWritten = false;
}
