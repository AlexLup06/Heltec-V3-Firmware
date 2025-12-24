#pragma once
#include <Arduino.h>
#include <LittleFS.h>

inline void createDirChecked(const char *path)
{
    if (!path || path[0] != '/')
    {
        Serial.printf("Invalid path '%s' (must start with '/')\n", path ? path : "NULL");
        return;
    }

    if (strlen(path) >= 63)
    {
        Serial.printf("Path too long: '%s'\n", path);
        return;
    }

    bool ok = LittleFS.mkdir(path);

    if (!ok)
    {
        if (LittleFS.exists(path))
        {
            Serial.printf("Directory already exists: %s\n", path);
        }
        else
        {
            Serial.printf("mkdir failed for: %s\n", path);
        }
    }
    else
    {
        Serial.printf("Created directory: %s\n", path);
    }
}

inline void dumpDir(File dir, String path = "")
{
    File file = dir.openNextFile();
    while (file)
    {
        String name = file.name();
        String fullPath = path + "/" + name;

        if (file.isDirectory())
        {
            dumpDir(file, fullPath);
        }
        else
        {
            size_t size = file.size();
            Serial.printf("BEGIN_FILE:%s:%u\n", fullPath.c_str(), (unsigned)size);
            delay(50);

            uint8_t buffer[128];
            while (file.available())
            {
                size_t bytesRead = file.read(buffer, sizeof(buffer));
                Serial.write(buffer, bytesRead);
            }

            Serial.println();
            Serial.printf("END_FILE:%s\n", fullPath.c_str());
            delay(100);
        }

        file = dir.openNextFile();
    }
}

inline void dumpFilesOverSerial()
{
    File root = LittleFS.open("/");
    if (!root)
    {
        Serial.println("Failed to open LittleFS root");
        return;
    }

    dumpDir(root, "");
    Serial.println("ALL_DONE");
}

inline String normalizePath(const String &p)
{
    String out = p;

    const char *prefix = "/littlefs";
    int plen = strlen(prefix);

    if (out.startsWith(prefix))
        out.remove(0, plen);

    if (!out.startsWith("/"))
        out = "/" + out;

    return out;
}

inline bool deleteRecursive(const char *path)
{
    File dir = LittleFS.open(path);
    if (!dir)
    {
        Serial.printf("Cannot open path: %s\n", path);
        return false;
    }

    if (!dir.isDirectory())
    {
        if (String(path).endsWith(".bin"))
        {
            dir.close();
            if (LittleFS.remove(path))
                Serial.printf("Deleted file: %s\n", path);
            else
                Serial.printf("Failed to delete file: %s\n", path);
        }
        return true;
    }

    File child = dir.openNextFile();
    while (child)
    {
        String name = child.name(); 
        child.close();

        String full = String(path);
        if (!full.endsWith("/"))
            full += "/";
        full += name;

        deleteRecursive(full.c_str());

        child = dir.openNextFile();
    }

    dir.close();

    if (String(path) != "/")
    {
        if (LittleFS.rmdir(path))
            Serial.printf("Removed directory: %s\n", path);
        else
            Serial.printf("Failed to remove directory: %s\n", path);
    }

    return true;
}

inline void deleteAllBinFilesAndDirs()
{
    Serial.println("Recursively deleting .bin files and directories...");

    deleteRecursive("/");

    Serial.println("Complete wipe finished.");
}