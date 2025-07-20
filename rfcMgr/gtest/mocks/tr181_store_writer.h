#ifndef TR181_STORE_WRITER_H
#define TR181_STORE_WRITER_H

#include <string>

enum ValueFormat {
    Plain,      // key=value
    Quoted      // key="value"
};

/**
 * @brief Writes or updates a key="value" pair in a flat TR-181-style config file.
 *
 * @param key       The TR-181 parameter name (e.g., "Device.DeviceInfo...")
 * @param value     The string value to be assigned (e.g., "true")
 * @param filePath  Path to the config file to read/update/create
 */
void writeToTr181storeFile(const std::string& key, const std::string& value, const std::string& filePath);

void write_on_file(const std::string& filePath, const std::string& content);

#endif // TR181_STORE_WRITER_H
