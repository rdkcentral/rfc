/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <cstdio>

#include "mtlsUtils.h"
#include "rfc_common.h"
#include "rfc_xconf_handler.h"
#include "tr181api.h"
#include "rfc_manager.h"
#include "xconf_handler.h"
#include <urlHelper.h>

using namespace std;
using namespace rfc;

// --- Fixture ---
class rfcMgrTest : public ::testing::Test {
    protected:
    class TestableXconfHandler : public xconf::XconfHandler {
    public:
            using xconf::XconfHandler::_estb_mac_address;  // expose protected member as public
    };

    void SetUp() override {
        xconfObj = new TestableXconfHandler();
    }

    void TearDown() override {
        delete xconfObj;
    }

    TestableXconfHandler* xconfObj;	
};

void writeToTr181storeFile(const std::string& key, const std::string& value, const std::string& filePath) {
    // Check if the file exists and is openable in read mode
    std::ifstream fileStream(filePath);
    bool found = false;
    std::string line;
    std::vector<std::string> lines;

    if (fileStream.is_open()) {
        while (getline(fileStream, line)) {
            // Check if the current line contains the key
            if (line.find(key) != std::string::npos && line.substr(0, key.length()) == key) {
                // Replace the line with the new key-value pair
                line = key + "=\"" + value + "\"";
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
        lines.push_back(key + "=\"" + value + "\"");
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

// Helper function to trim right whitespace or specific characters
std::string rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(" \n\r\t\f\v");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

// Function to read the entire content of a file into a std::string
std::string readFileContent(const std::string& filePath) {
    std::ifstream fileStream(filePath);
    if (fileStream) {
        std::ostringstream content;
        content << fileStream.rdbuf();
        fileStream.close();
        // Trim right to remove any trailing whitespace or newline characters
        return rtrim(content.str());
    }
    return "";  // Return empty string if file couldn't be opened
}

std::map<std::string, std::string> parseQueryString(std::stringstream& queryStream) {
    std::string query = queryStream.str();  // Extract string from stringstream
    std::map<std::string, std::string> params;
    std::stringstream ss(query);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            params[key] = value;
        }
    }
    return params;
}

char xconfResp[] = R"({
    "featureControl": {
        "features": [
            {
                "name": "11",
                "enable": false,
                "effectiveImmediate": false,
                "configData": {
                    "Warrens": "Feature"
                },
                "featureInstance": "11"
            },
            {
                "name": "ARU",
                "enable": true,
                "effectiveImmediate": false,
                "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AutoReboot.Enable": "true",
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AutoReboot.UpTime": "29"
                },
                "featureInstance": "ARU:E_29"
            },
            {
                "name": "AccountId",
                "enable": true,
                "effectiveImmediate": true,
                "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID": "4123705941507160513"
                },
                "featureInstance": "AccountId"
            },
            {
                "name": "LSA",
                "enable": true,
                "effectiveImmediate": true,
                "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.AdCacheEnable": "True",
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.ByteRangeDownload": "True",
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.Enable": "True",
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.PSNUrl": "https://5df09-linear-test.v.fwmrm.net/scte/request?nw=384776&csid=linear_endpoint_x1&resp=scte-130&prof=linear_mvpd",
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.PlacementReqUrl": "https://5df09-linear-test.v.fwmrm.net/scte/request?nw=384776&csid=linear_endpoint_x1&resp=scte-130&prof=linear_mvpd",
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.LSA.ProgrammerEnable": "True"
                },
                "featureInstance": "LSA_End2End"
            },
            {
                "name": "SSHWhiteList",
                "enable": true,
                "effectiveImmediate": false,
                "configData": {},
                "featureInstance": "SSHWhiteList:E_N",
                "listType": "SSHWhiteList_ipList",
                "listSize": 44,
                "SSHWhiteList_ipList": [
                    "96.114.220.197",
                    "96.114.220.34",
                    "96.118.137.237"
                ]
            },
            {
                "name": "Telemetry_2.0-31099",
                "enable": true,
                "effectiveImmediate": true,
                "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable": "true",
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.MTLS.Enable": "true",
                    "tr181.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.ConfigURL": "https://mockxconf/loguploader/getT2Settings",
                    "tr181.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Version": "2.0.1"
                },
                "featureInstance": "Telemetry_2.0-31099"
            },
            {
		"name": "PartnerId",
		"enable": true,
	        "effectiveImmediate": true,
	        "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.PartnerId": "comcast"
                },
	        "featureInstance": "PartnerId"	
	    } 
        ]
    }
})";


TEST_F(rfcMgrTest, initializeXconf) {
     write_on_file("/opt/partnerid", "default-parter");	
     write_on_file("/tmp/.estb_mac", "01:23:45:67:89:ab");
     write_on_file("/version.txt", "imagename:TestImage");
     //xconf::XconfHandler *xconfObj = new xconf::XconfHandler();
     int resutl = xconfObj->initializeXconfHandler();
     EXPECT_EQ(xconfObj->_estb_mac_address , "01:23:45:67:89:ab"); 
     //EXPECT_EQ(xconfObj->_partner_id , "default-partner");
     //EXPECT_EQ(xconfObj->_estb_mac_address , "TestImage");     
}


GTEST_API_ int main(int argc, char *argv[]){
    ::testing::InitGoogleTest(&argc, argv);

    cout << "Starting GTEST===========================>" << endl;
    return RUN_ALL_TESTS();
}


