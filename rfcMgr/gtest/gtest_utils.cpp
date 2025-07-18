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

#include "jsonhandler.h"

using namespace std;

#ifdef GTEST_ENABLE
extern bool (*getGetParamTypeFunc())(char * const, DATA_TYPE *);
extern DATA_TYPE (*getConvertTypeFunc())(char);
extern int (*getClearAttributeFunc())(char * const);
extern int (*getSetAttributeFunc())(char * const, char, char *);
extern int (*getGetAttributeFunc())(char * const);
extern int (*getparseargsFunc())(int argc, char * argv[]);
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

TEST(rfcMgrTest, getParamType) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted);
   char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable");
   char *pcCallerID ="rfcdefaults";
   DATA_TYPE paramType;
   bool status = getGetParamTypeFunc()(pcParameterName, &paramType);
   EXPECT_EQ(status, true);
}

TEST(rfcMgrTest, CallconvertType) {
   char itype = 'i';
   DATA_TYPE status = getConvertTypeFunc()(itype);
   EXPECT_EQ(status, WDMP_INT);
   
   char stype = 's';
   status = getConvertTypeFunc()(stype);
   EXPECT_EQ(status, WDMP_STRING);
   
   char btype = 'b';
   status = getConvertTypeFunc()(btype);
   EXPECT_EQ(status, WDMP_BOOLEAN);

   char dtype = 'f';
   status = getConvertTypeFunc()(dtype);
   EXPECT_EQ(status, WDMP_INT);
}


TEST(rfcMgrTest, CallgetAttribute) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted); 
   char * const pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable";
   char *pcCallerID ="rfcdefaults";
   TR181_ParamData_t pstParamData;
   int status = getGetAttributeFunc()(pcParameterName);
   EXPECT_EQ(status, 0);
}

TEST(rfcMgrTest, CallsetAttribute) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted);
   char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable");
   char *pcCallerID ="rfcdefaults";
   char * value = const_cast<char*>("false");
   int status = getSetAttributeFunc()(pcParameterName, 'b', value);
   EXPECT_EQ(status, 0);
}


TEST(rfcMgrTest, CallclearAttribute) {
   //writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini");
   char * const pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable";
   int status = getClearAttributeFunc()(pcParameterName);
   EXPECT_EQ(status, 0);
}

TEST(rfcMgrTest, Callparseargs) {
   char* argv[] = { (char*)"tr181", (char*)"-n", (char*)"localOnly" };
   int argc = 3;
   int status = getparseargsFunc()(argc, argv);	
   EXPECT_EQ(status, 0);
}

TEST(rfcMgrTest, CallsetAttribute_args) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted);
   char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable");
   char *pcCallerID ="rfcdefaults";
   char * value = const_cast<char*>("false");
   char* argv[] = { (char*)"tr181", (char*)"-n", (char*)"localOnly" };
   int argc = 3;
   int args_status = getparseargsFunc()(argc, argv);
   int status = getSetAttributeFunc()(pcParameterName, 'b', value);
   EXPECT_EQ(status, 0);
}

TEST(rfcMgrTest, CallgetAttribute_args) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted);
   char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable");
   char *pcCallerID ="rfcdefaults";
   char * value = const_cast<char*>("false");
   char* argv[] = { (char*)"tr181", (char*)"-n", (char*)"localOnly" };
   int argc = 3;
   int args_status = getparseargsFunc()(argc, argv);
   int status = getGetAttributeFunc()(pcParameterName);
   EXPECT_EQ(status, 0);
}


TEST(rfcMgrTest, CallclearParam) {
   char * const pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable";
   char *pcCallerID ="rfcdefaults";
   tr181ErrorCode_t status = clearParam(pcCallerID, pcParameterName);
   EXPECT_EQ(status, tr181Success);
}

TEST(rfcMgrTest, CallsetParam) {
   char * const pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.TopSpeed";
   char *pcCallerID ="rfcdefaults";
   const char* pcParameterValue = "1280000";
   tr181ErrorCode_t status = setParam(pcCallerID, pcParameterName, pcParameterValue);
   EXPECT_EQ(status, tr181Success);
}


TEST(rfcMgrTest, CallreadFromFile) {
    std::string jsonString = R"({"jsonrpc":"2.0","id":3,"result":{"experience":"X1","success":true}})";
    write_on_file("/tmp/test.json", jsonString);    	
    char *result = readFromFile("/tmp/test.json");
    EXPECT_NE(result, nullptr);
    std::string actual(result);
    EXPECT_EQ(actual, jsonString+ "\n");

    delete [] result;
    std::remove("/tmp/test.json");
}

TEST(rfcMgrTest, CallgetArrayNode) {
    const char* jsonStr = R"({"jsonrpc":"2.0","id":3,"result":{"experience":"X1","success":true,"features":["A","B","C"]}})";
    cJSON* root = cJSON_Parse(jsonStr);
    EXPECT_NE(root, nullptr);

    cJSON* arrayNode = getArrayNode(root);
    EXPECT_NE(arrayNode, nullptr);
    EXPECT_EQ(cJSON_GetArraySize(arrayNode), 3);

    cJSON_Delete(root);
}


/* TEST(rfcMgrTest, iterateAndSaveArrayNodes) {
    
    const char* jsonStr = R"({"featureControl":{"features":["A","B","C"]}})";
    int count = iterateAndSaveArrayNodes("/tmp/test.json",jsonStr);
    EXPECT_EQ(count, 3);
} */

TEST(rfcMgrTest, saveToFile) {
    const char *format = "/tmp/%s_output.txt";
    const char *name = "testfile";
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array, cJSON_CreateString("line1"));
    cJSON_AddItemToArray(array, cJSON_CreateString("line2"));
    int status = saveToFile(array, format, name);
    EXPECT_EQ(status, 1);

    cJSON_Delete(array);
} 


TEST(rfcMgrTest, getFilePath) {
    char *path = getFilePath();
    std::string expected = "/opt/secure/RFC/.RFC_LIST_%s.ini";
    EXPECT_STREQ(path, expected.c_str());
    
    delete[] path;
}

GTEST_API_ int main(int argc, char *argv[]){
    ::testing::InitGoogleTest(&argc, argv);

    cout << "Starting GTEST===========================>" << endl;
    return RUN_ALL_TESTS();
}
