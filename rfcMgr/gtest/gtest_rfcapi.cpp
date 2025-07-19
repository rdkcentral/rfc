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
#include <fstream>
#include <string>
#include <iostream>

#include "rfcapi.h"

using namespace std;

#define RFCDEFAULTS_ETC_DIR "/etc/rfcdefaults/"


#ifdef GTEST_ENABLE
extern size_t (*getWriteCurlResponse(void))(void *ptr, size_t size, size_t nmemb, std::string stream);
#endif

enum ValueFormat {
    Plain,      // key=value
    Quoted      // key="value"
};


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

void write_on_file(const std::string& filePath, const std::string& content)
{
   std::ofstream outfile(filePath, std::ios::app);
   if (outfile.is_open()) {
        outfile << content<< "\n" ;
        outfile.close();
    } else {
        std::cerr << "Could not open file for appending.\n";
    }

}


TEST(rfcMgrTest, init_rfcdefaults) {
   bool result = init_rfcdefaults();
   EXPECT_EQ(result, true);
}

TEST(rfcMgrTest, init_rfcdefaults_removed_etc_dir) {
   std::string cmd = std::string("rm -rf ") + RFCDEFAULTS_ETC_DIR;
   int status = system(cmd.c_str());
   EXPECT_EQ(status, 0);
   bool result = init_rfcdefaults();
   EXPECT_EQ(result, false);
}

TEST(rfcMgrTest, isRFCEnabled) {
     bool result = isRFCEnabled("Instance"); 
     EXPECT_EQ(result, false);
}

TEST(rfcMgrTest, getRFCErrorString) {
    EXPECT_STREQ(getRFCErrorString(WDMP_SUCCESS), " Success");
    EXPECT_STREQ(getRFCErrorString(WDMP_FAILURE), " Request Failed");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_TIMEOUT), " Request Timeout");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_PARAMETER_NAME), " Invalid Parameter Name");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_PARAMETER_TYPE), " Invalid Parameter Type");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_PARAMETER_VALUE), " Invalid Parameter Value");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_NOT_WRITABLE), " Not writable");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_SETATTRIBUTE_REJECTED), " SetAttribute Rejected");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_NAMESPACE_OVERLAP), " Namespace Overlap");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_UNKNOWN_COMPONENT), " Unknown Component");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_NAMESPACE_MISMATCH), " Namespace Mismatch");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_UNSUPPORTED_NAMESPACE), " Unsupported Namespace");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_DP_COMPONENT_VERSION_MISMATCH), " Component Version Mismatch");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_PARAM), " Invalid Param");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_UNSUPPORTED_DATATYPE), " Unsupported Datatype");
    EXPECT_STREQ(getRFCErrorString(WDMP_STATUS_RESOURCES), " Resources");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_WIFI_BUSY), " Wifi Busy");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_ATTRIBUTES), " Invalid Attributes");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_WILDCARD_NOT_SUPPORTED), " Wildcard Not Supported");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_SET_OF_CMC_OR_CID_NOT_SUPPORTED), " Set of CMC or CID Not Supported");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_VALUE_IS_EMPTY), " Value is Empty");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_VALUE_IS_NULL), " Value is Null");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_DATATYPE_IS_NULL), " Datatype is Null");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_CMC_TEST_FAILED), " CMC Test Failed");    
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_NEW_CID_IS_MISSING), " New CID is Missing");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_CID_TEST_FAILED), " CID Test Failed");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_SETTING_CMC_OR_CID), " Setting CMC or CID");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_INPUT_PARAMETER), " Invalid Input Parameter");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_ATTRIBUTES_IS_NULL), " Attributes is Null");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_NOTIFY_IS_NULL), " Notify is Null");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_WIFI_INDEX), " Invalid Wifi Index");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INVALID_RADIO_INDEX), " Invalid Radio Index");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_ATOMIC_GET_SET_FAILED), " Atomic Get Set Failed");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_METHOD_NOT_SUPPORTED), " Method Not Supported");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_INTERNAL_ERROR), " Internal Error");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_DEFAULT_VALUE), " Default Value");
    EXPECT_STREQ(getRFCErrorString(WDMP_ERR_MAX_REQUEST), " Unknown error code");
}

TEST(rfcMgrTest, writeCurlResponse) {
   const char* input = "MockCurlData";
   size_t size = 1;
   size_t nmemb = strlen(input);
   std::string response;
   size_t written = getWriteCurlResponse()((void*)input, size, nmemb, response);
   EXPECT_EQ(written, nmemb);
}

TEST(rfcMgrTest, getRFCParameter) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.LogUpload.LogServerUrl", "logs.xcal.tv", "/opt/secure/RFC/tr181store.ini", Plain);
   const char* pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.LogUpload.LogServerUrl";
   char *pcCallerID ="rfcdefaults";
   RFC_ParamData_t pstParamData;
   WDMP_STATUS result = getRFCParameter(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_STREQ(pstParamData.value, "logs.xcal.tv");
   EXPECT_EQ(result , WDMP_SUCCESS);
}

TEST(rfcMgrTest, getRFCParameter_rfcdefault) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.LowSpeed", "12800", "/opt/secure/RFC/bootstrap.ini", Plain);
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.LowSpeed";
   char *pcCallerID ="rfcdefaults";
   RFC_ParamData_t pstParamData;
   WDMP_STATUS result = getRFCParameter(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_EQ(result, WDMP_SUCCESS);
   EXPECT_STREQ(pstParamData.value, "12800");
}

TEST(rfcMgrTest, getRFCParameter_HTTP) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable";
   char *pcCallerID ="rfcdefaults";
   write_on_file("/tmp/.tr69hostif_http_server_ready", ".tr69hostif_http_server_ready");
   RFC_ParamData_t pstParamData;
   WDMP_STATUS result = getRFCParameter(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_STREQ(pstParamData.value, "true");
   EXPECT_EQ(result, WDMP_SUCCESS);
}

TEST(rfcMgrTest, getRFCParameter_wildcard) {
   const char* pcParameterName ="Device.DeviceInfo.";
   char *pcCallerID ="rfcdefaults";
   RFC_ParamData_t pstParamData;
   WDMP_STATUS result = getRFCParameter(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_EQ(result, WDMP_FAILURE);
}


TEST(rfcMgrTest, setRFCParameter_wildcard) {
   const char* pcParameterName = "Device.DeviceInfo.";
   char *pcCallerID ="rfcdefaults";
   const char* pcParameterValue = NULL;
   RFC_ParamData_t pstParamData;
   WDMP_STATUS result = setRFCParameter(pcCallerID, pcParameterName, pcParameterValue, WDMP_STRING);
   EXPECT_EQ(result, WDMP_FAILURE);
}

TEST(rfcMgrTest, setRFCParameter) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.SsrUrl";
   char *pcCallerID ="rfcdefaults";
   const char* pcParameterValue = "https://ssr.ccp.xcal.tv";
   RFC_ParamData_t pstParamData;
   WDMP_STATUS result = setRFCParameter(pcCallerID, pcParameterName, pcParameterValue, WDMP_STRING);
   EXPECT_EQ(result, WDMP_SUCCESS);
}


GTEST_API_ int main(int argc, char *argv[]){
    ::testing::InitGoogleTest(&argc, argv);

    cout << "Starting GTEST===========================>" << endl;
    return RUN_ALL_TESTS();
}
