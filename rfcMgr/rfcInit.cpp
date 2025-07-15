#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <regex>
#include <ctime>
#include <iomanip>

#include "rfc_common.h"

namespace fs = std::filesystem;

class RFCInit {
private:
    const std::string RFC_INIT_LOCK = "/opt/.rfcInitInProgress";
    const std::string SECURE_DIR = "/opt/secure";
    const std::string RFC_BASE = "/opt/secure/RFC";
    const std::string OLD_RFC_BASE = "/opt/RFC";
    const std::string RFC_LIST_FILE_NAMES = "/opt/secure/RFC/.RFC_";
    const std::string RFC_LIST_FILE_NAME_SUFFIX = ".list";
    const std::string TR181STORE_FILE = "/opt/secure/RFC/tr181store.ini";
    const std::string OLD_TR181STORE_FILE = "/opt/RFC/tr181store.ini";
    const std::string RFC_RAM_PATH = "/tmp/RFC";
    const std::string RFC_DEFAULTS_PATH = "/tmp/rfcdefaults.ini";
    const std::string RFC_DEFAULTS_DIR = "/etc/rfcdefaults";

public:
    RFCInit() = default;

    bool fileExists(const std::string& path) {
        return fs::exists(path);
    }

    bool directoryExists(const std::string& path) {
        return fs::exists(path) && fs::is_directory(path);
    }

    void createLockFile() {
        std::ofstream lockFile(RFC_INIT_LOCK);
        lockFile.close();
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFCINIT: Created lock file %s\n", __FUNCTION__, __LINE__, RFC_INIT_LOCK.c_str());
    }

    void removeLockFile() {
        if (fileExists(RFC_INIT_LOCK)) {
            fs::remove(RFC_INIT_LOCK);
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Completed service, deleting lock\n",  __FUNCTION__, __LINE__);
        }
    }

    bool copyDirectory(const std::string& source, const std::string& destination) {
        try {
            fs::copy(source, destination, fs::copy_options::recursive);
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Successfully copied directory from %s to %s\n",  __FUNCTION__, __LINE__, source.c_str(), destination.c_str());
            return true;
        } catch (const fs::filesystem_error& e) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Error copying directory from %s to %s: %s\n",  __FUNCTION__, __LINE__, source.c_str(), destination.c_str(), e.what());
            return false;
        }
    }

    bool removeDirectory(const std::string& path) {
        try {
            if (directoryExists(path)) {
                fs::remove_all(path);
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Removed directory %s\n",  __FUNCTION__, __LINE__, path.c_str());
                return true;
            }
            return true;
        } catch (const fs::filesystem_error& e) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Error removing directory %s: %s\n", __FUNCTION__, __LINE__, path.c_str(), e.what());
            return false;
        }
    }

    void listDirectoryToLog(const std::string& path) {
        try {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Listing directory contents for %s\n", __FUNCTION__, __LINE__, path.c_str());

            for (const auto& entry : fs::directory_iterator(path)) {
                auto ftime = fs::last_write_time(entry);
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                auto time_t = std::chrono::system_clock::to_time_t(sctp);

                char timeStr[100];
                std::strftime(timeStr, sizeof(timeStr), "%b %d %H:%M", std::localtime(&time_t));

                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: %s %s\n", __FUNCTION__, __LINE__, timeStr, entry.path().filename().string().c_str());
            }
        } catch (const fs::filesystem_error& e) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Error listing directory %s: %s\n", __FUNCTION__, __LINE__, path.c_str(), e.what());
        }
    }

    // Copy RFC list files matching pattern
    void copyRfcListFiles() {
        try {
            std::string pattern = RFC_LIST_FILE_NAMES + "*" + RFC_LIST_FILE_NAME_SUFFIX;
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFCINIT: Copying list files matching pattern %s to %s\n", __FUNCTION__, __LINE__, pattern.c_str(), RFC_RAM_PATH.c_str());

            // Create RFC_RAM_PATH if it doesn't exist
            if (!directoryExists(RFC_RAM_PATH)) {
                fs::create_directories(RFC_RAM_PATH);
            }

            std::string sourceDir = fs::path(RFC_LIST_FILE_NAMES).parent_path();
            std::string prefix = fs::path(RFC_LIST_FILE_NAMES).filename();

            for (const auto& entry : fs::directory_iterator(sourceDir)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.find(prefix) == 0 &&
                        filename.substr(filename.length() - RFC_LIST_FILE_NAME_SUFFIX.length()) == RFC_LIST_FILE_NAME_SUFFIX) {

                        std::string destPath = RFC_RAM_PATH + "/" + filename;
                        fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);

                        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Copied list file %s to %s\n", __FUNCTION__, __LINE__, filename.c_str(), destPath.c_str());
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Error copying RFC list files: %s\n", __FUNCTION__, __LINE__, e.what());
        }
    }

    // Extract ConfigSetTime from tr181store.ini file
    long getConfigSetTime(const std::string& iniFilePath) {
        std::ifstream file(iniFilePath);
        if (!file.is_open()) {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFCINIT: Could not open file %s\n", __FUNCTION__, __LINE__, iniFilePath.c_str());
            return 0;
        }

        std::string line;
        std::regex timeRegex(R"(ConfigSetTime\s*=\s*(\d+))");
        std::smatch match;

        while (std::getline(file, line)) {
            if (std::regex_search(line, match, timeRegex)) {
                try {
                    long time = std::stol(match[1].str());
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFCINIT: Found ConfigSetTime %ld in %s\n", __FUNCTION__, __LINE__, time, iniFilePath.c_str());
                    return time;
                } catch (const std::exception& e) {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Error parsing ConfigSetTime: %s\n", __FUNCTION__, __LINE__, e.what());
                    return 0;
                }
            }
        }

        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFCINIT: ConfigSetTime not found in %s\n", __FUNCTION__, __LINE__, iniFilePath.c_str());
        return 0;
    }

    void rfcMoveToNewLoc() {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Starting RFC migration process\n", __FUNCTION__, __LINE__);

        // Check if secure directory exists
        if (!directoryExists(SECURE_DIR)) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: ERROR configuring RFC location NewLoc=%s, no parent folder\n", __FUNCTION__, __LINE__, SECURE_DIR.c_str());
            return;
        }

        // Mark transition in progress
        createLockFile();

        // Remove existing RFC directory in new location
        if (directoryExists(RFC_BASE)) {
            removeDirectory(RFC_BASE);
        }

        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Changing RFC location CurrentLoc=%s, NewLoc=%s\n", __FUNCTION__, __LINE__, OLD_RFC_BASE.c_str(), RFC_BASE.c_str());

        // List old directory contents
        if (directoryExists(OLD_RFC_BASE)) {
            listDirectoryToLog(OLD_RFC_BASE);

            // Copy RFC directory from old location to secure location
            if (copyDirectory(OLD_RFC_BASE, SECURE_DIR)) {
                // Update list files
                copyRfcListFiles();
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Successfully completed RFC migration\n", __FUNCTION__, __LINE__);
            } else {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Failed to copy RFC directory\n", __FUNCTION__, __LINE__);
            }
        } else {
            RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR, "[%s][%d] RFCINIT: Old RFC directory %s does not exist\n", __FUNCTION__, __LINE__, OLD_RFC_BASE.c_str());
        }

        // Remove transition lock file
        removeLockFile();
    }

    // Create rfcdefaults.ini from multiple ini files
    void createRfcDefaults() {
        if (fileExists(RFC_DEFAULTS_PATH)) {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: rfcdefaults.ini exist\n", __FUNCTION__, __LINE__);
            return;
        }

        std::ofstream outputFile(RFC_DEFAULTS_PATH);
        if (!outputFile.is_open()) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Could not create %s\n", __FUNCTION__, __LINE__, RFC_DEFAULTS_PATH.c_str());
            return;
        }

        try {
            if (directoryExists(RFC_DEFAULTS_DIR)) {
                int fileCount = 0;
                for (const auto& entry : fs::directory_iterator(RFC_DEFAULTS_DIR)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".ini") {
                        std::ifstream inputFile(entry.path());
                        if (inputFile.is_open()) {
                            outputFile << inputFile.rdbuf() << std::endl;
                            inputFile.close();
                            fileCount++;
                            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] RFCINIT: Added %s to rfcdefaults.ini\n", __FUNCTION__, __LINE__, entry.path().filename().string().c_str());
                        }
                    }
                }
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Created rfcdefaults.ini from %d ini files\n", __FUNCTION__, __LINE__, fileCount);
            } else {
                RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR, "[%s][%d] RFCINIT: RFC defaults directory %s does not exist\n", __FUNCTION__, __LINE__, RFC_DEFAULTS_DIR.c_str());
            }
        } catch (const fs::filesystem_error& e) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Error creating rfcdefaults.ini: %s\n", __FUNCTION__, __LINE__, e.what());
        }

        outputFile.close();
    }

    int run() {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Initialization started\n", __FUNCTION__, __LINE__);

        long oldTime = 0;
        long newTime = 0;

        // Get timestamps from configuration files
        if (fileExists(OLD_TR181STORE_FILE)) {
            oldTime = getConfigSetTime(OLD_TR181STORE_FILE);
        }

        if (fileExists(TR181STORE_FILE)) {
            newTime = getConfigSetTime(TR181STORE_FILE);
        }

        // Check if RFC transition has to take place
        if (oldTime == 0 || newTime == 0) {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Old time or New time is empty. The value of oldTime = %ld and newTime = %ld\n", __FUNCTION__, __LINE__, oldTime, newTime);
        } else {
            if (oldTime > newTime || fileExists(RFC_INIT_LOCK)) {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Transition - Old time %ld is greater than new time %ld\n", __FUNCTION__, __LINE__, oldTime, newTime);
                rfcMoveToNewLoc();
            } else {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: No Transition - Old time %ld is SMALLER than new time %ld\n", __FUNCTION__, __LINE__, oldTime, newTime);
            }
        }

        // Create rfcdefaults.ini
        createRfcDefaults();

        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCINIT: Initialization completed\n", __FUNCTION__, __LINE__);
        return 0;
    }
};

int main() {
    if (rdk_logger_init("/etc/debug.ini") != 0) {
        std::cerr << "Failed to initialize RDK Logger" << std::endl;
        return 1;
    }

    try {
        RFCInit rfcInit;
        int result = rfcInit.run();

        rdk_logger_deinit();

        return result;
    } catch (const std::exception& e) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFCINIT: Fatal error: %s\n", __FUNCTION__, __LINE__, e.what());
        rdk_logger_deinit();
        return 1;
    }
}
