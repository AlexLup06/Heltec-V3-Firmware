#pragma once
#include "DataLogger.h"

template <typename T>
DataLogger<T>::DataLogger(const char* fname, const FileHeader* _header)
    : filename(fname ? fname : "") {
    if (_header) header = *_header;
}

template <typename T>
void DataLogger<T>::addDataPoint(const T& dp) {
    buffer.push_back(dp);
}

template <typename T>
bool DataLogger<T>::saveToDisk() {
    if (filename.isEmpty()) {
        DEBUG_PRINTLN("No filename set");
        return false;
    }

    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) {
        DEBUG_PRINTF("Failed to open %s for writing\n", filename.c_str());
        return false;
    }

    // --- Write header once ---
    if (!headerWritten) {
        header.entrySize = sizeof(T);
        file.write((uint8_t*)&header, sizeof(FileHeader));
        headerWritten = true;
    }

    // --- Write data points ---
    size_t bytes = file.write((uint8_t*)buffer.data(), buffer.size() * sizeof(T));
    file.close();

    DEBUG_PRINTF("Saved %u entries + header to %s\n",
                  (unsigned)buffer.size(), filename.c_str());
    return bytes == buffer.size() * sizeof(T);
}

template <typename T>
void DataLogger<T>::clear() {
    buffer.clear();
    headerWritten = false;
}
