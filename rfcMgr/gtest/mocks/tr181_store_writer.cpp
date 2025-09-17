#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "tr181_store_writer.h"

void writeToTr181storeFile(const std::string& key, const std::string& value, const std::string& filePath, ValueFormat format) {
    // Check if the file exists and is openable in read mode
    std::ifstream fileStream(filePath);
    bool found = false;
    std::string line;
    std::vector<std::string> lines;

    std::string formattedLine = (format == Quoted)
        ? key + "=\"" + value + "\""
        : key + "=" + value;

    if (fileStream.is_open()) {
        while (getline(fileStream, line)) {
            // Check if the current line contains the key
            if (line.find(key) != std::string::npos && line.substr(0, key.length()) == key) {
                // Replace the line with the new key-value pair
                line = formattedLine;
                found = true;
            }
            lines.push_back(line);
        }
        fileStream.close();
    } else {
        std::cout << "File does not exist or cannot be opened for reading. It will be created." << std::endl;
    }

    // If the key was not found in an existing file or the file did not exist, add it to the vector
    if (!found) {
        lines.push_back(formattedLine);
    }

    // Open the file in write mode to overwrite old content or create new file
    std::ofstream outFileStream(filePath);
    if (outFileStream.is_open()) {
        for (const auto& outputLine : lines) {
            outFileStream << outputLine << std::endl;
        }
        outFileStream.close();
        std::cout << "Configuration updated successfully." << std::endl;
    } else {
        std::cout << "Error opening file for writing." << std::endl;
    }
}

void write_on_file(const std::string& filePath, const std::string& data)
{
   std::ofstream outfile(filePath, std::ios::app);
   if (outfile.is_open()) {
        std::cout << "File Open" << std::endl;
	outfile << data<< "\n" ;
	std::cout << "Writing in file" << std::endl;
        outfile.close();
	std::cout << "File written successfully." << std::endl;
    } else {
        std::cerr << "Unable to open file." << std::endl ;
    }

}

