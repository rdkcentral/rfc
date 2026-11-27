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
#include "tr181api.h"
#include <urlHelper.h>
#include "rfcapi.h"
#include "jsonhandler.h"
#include "tr181_store_writer.h"
#include <curl/curl.h>
#include <unistd.h>

using namespace std;

#ifdef GTEST_ENABLE
extern bool (*getGetParamTypeFunc())(char * const, DATA_TYPE *);
extern DATA_TYPE (*getConvertTypeFunc())(char);
extern int (*getClearAttributeFunc())(char * const);
extern int (*getSetAttributeFunc())(char * const, char, char *);
extern int (*getGetAttributeFunc())(char * const);
extern size_t (*getWriteCurlResponse(void))(void *ptr, size_t size, size_t nmemb, std::string stream);
extern int (*getparseargsFunc())(int argc, char * argv[]);
extern void (*getMuteStderrFunc())(void);
extern void (*getWriteValueLineFunc())(const char *);
#endif

extern CURLcode simulated_curl_result;
int isNodeArrayType(cJSON * const item);

/* Forces the mocked HTTP transport to fail for the lifetime of the object. */
class UtilsCurlFailureGuard {
    CURLcode saved_ = simulated_curl_result;
public:
    UtilsCurlFailureGuard() { simulated_curl_result = CURLE_COULDNT_CONNECT; }
    ~UtilsCurlFailureGuard() { simulated_curl_result = saved_; }
};

/* parseargs() keeps the caller id in a static; use it to select local or HTTP mode. */
static void setCallerId(const char *callerId)
{
    char *argv[] = { (char*)"tr181", (char*)"-n", (char*)callerId };
    getparseargsFunc()(3, argv);
}

TEST(utilsTest, getParamType) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted);
    char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable");
    DATA_TYPE paramType;
    bool status = getGetParamTypeFunc()(pcParameterName, &paramType);
    EXPECT_EQ(status, true);
}

TEST(utilsTest, CallconvertType) {
    char itype = 'i';
    DATA_TYPE status = getConvertTypeFunc()(itype);
    EXPECT_EQ(status, WDMP_INT);
   
    char stype = 's';
    status = getConvertTypeFunc()(stype);
    EXPECT_EQ(status, WDMP_STRING);
   
    char btype = 'b';
    status = getConvertTypeFunc()(btype);
    EXPECT_EQ(status, WDMP_BOOLEAN);

    char ftype = 'f';
    status = getConvertTypeFunc()(ftype);
    EXPECT_EQ(status, WDMP_INT);
}

TEST(utilsTest, CallgetAttribute) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted); 
    char * const pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable";
    int status = getGetAttributeFunc()(pcParameterName);
    EXPECT_EQ(status, 0);
}

TEST(utilsTest, CallsetAttribute) {
    char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_xOpsDeviceMgmt.ForwardSSH.Enable");
    char * value = const_cast<char*>("false");
    int status = getSetAttributeFunc()(pcParameterName, 'b', value);
    EXPECT_EQ(status, 0);
}

TEST(utilsTest, CallclearAttribute) {
    char * const pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable";
    int status = getClearAttributeFunc()(pcParameterName);
    EXPECT_EQ(status, 0);
}

TEST(utilsTest, Callparseargs) {
    char* argv[] = { (char*)"tr181", (char*)"-n", (char*)"localOnly" };
    int argc = 3;
    int status = getparseargsFunc()(argc, argv);
    EXPECT_EQ(status, 0);
}

TEST(utilsTest, CallsetAttribute_args) {
    char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MOCASSH.Enable");
    char * value = const_cast<char*>("false");
    char* argv[] = { (char*)"tr181", (char*)"-n", (char*)"localOnly" };
    int argc = 3;
    int args_status = getparseargsFunc()(argc, argv);
    int status = getSetAttributeFunc()(pcParameterName, 'b', value);
    EXPECT_EQ(status, 0);
}

TEST(utilsTest, CallgetAttribute_args) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.DHCPv6Client.Enable", "true", "/opt/secure/RFC/tr181localstore.ini", Quoted);
    char * const pcParameterName = const_cast<char*>("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.DHCPv6Client.Enable");
    char *pcCallerID ="rfcdefaults";
    char * value = const_cast<char*>("false");
    char* argv[] = { (char*)"tr181", (char*)"-n", (char*)"localOnly" };
    int argc = 3;
    int args_status = getparseargsFunc()(argc, argv);
    int status = getGetAttributeFunc()(pcParameterName);
    EXPECT_EQ(status, 0);
}

TEST(utilsTest, CallreadFromFile) {
    std::string jsonString = R"({"jsonrpc":"2.0","id":3,"result":{"experience":"X1","success":true}})";
    write_on_file("/tmp/test.json", jsonString);    	
    char *result = readFromFile("/tmp/test.json");
    EXPECT_NE(result, nullptr);
    
    delete [] result;
    std::remove("/tmp/test.json");
}

TEST(utilsTest, CallgetArrayNode) {
    const char* jsonStr = R"({"jsonrpc":"2.0","id":3,"result":{"experience":"X1","success":true,"features":["A","B","C"]}})";
    cJSON* root = cJSON_Parse(jsonStr);
    EXPECT_NE(root, nullptr);

    cJSON* arrayNode = getArrayNode(root);
    EXPECT_NE(arrayNode, nullptr);
    EXPECT_EQ(cJSON_GetArraySize(arrayNode), 3);

    cJSON_Delete(root);
}

TEST(utilsTest, iterateAndSaveArrayNodes) {
    
    const char* jsonStr = R"({"featureControl":{"features":[{"name":"SNMP2WL","effectiveImmediate":false,"enable":true,"configData":{},"listType":"SNMPIPv4","listSize":2,"SNMP IP4 WL":["128.82.34.17","10.0.0.32/6"]}]}})";
    const char *absolutePath = "/opt/secure/RFC/.RFC_LIST_%s.ini";
    int count = iterateAndSaveArrayNodes(absolutePath, jsonStr);
    EXPECT_EQ(count, 1);
}

TEST(utilsTest, saveToFile) {
    const char *format = "/opt/secure/RFC/.RFC_LIST_%s.ini";
    const char *name = "SNMP2WL";
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array, cJSON_CreateString("128.82.34.17"));
    cJSON_AddItemToArray(array, cJSON_CreateString("10.0.0.32/6"));
    int status = saveToFile(array, format, name);
    EXPECT_EQ(status, 1);

    cJSON_Delete(array);
}

TEST(utilsTest, saveIfNodeContainsLists) {
    const char *absolutePath = "/opt/secure/RFC/.RFC_LIST_%s.ini";
    const char *name = "SNMP2WL";
    cJSON *node = cJSON_CreateObject();
    cJSON_AddStringToObject(node, "name", "SNMP2WL");
    cJSON_AddStringToObject(node, "listType", "SNMPIPv4");
    cJSON *array = cJSON_CreateArray();
    cJSON_AddItemToArray(array, cJSON_CreateString("128.82.34.17"));
    cJSON_AddItemToArray(array, cJSON_CreateString("10.0.0.32/6"));
    cJSON_AddItemToObject(node, "SNMP IP4 WL", array);
    int status = saveIfNodeContainsLists(node, absolutePath);
    EXPECT_EQ(status, 1);

    cJSON_Delete(node);
}

TEST(utilsTest, getFilePath) {
    char *path = getFilePath();
    std::string expected = "/opt/secure/RFC/.RFC_LIST_%s.ini";
    EXPECT_STREQ(path, expected.c_str());
    
    delete[] path;
}

TEST(utilsTest, ParseargsHandlesAllOptions) {
    char* argv[] = { (char*)"tr181", (char*)"-s", (char*)"-c", (char*)"-t", (char*)"s",
                     (char*)"-v", (char*)"val", (char*)"-d", (char*)"-z",
                     (char*)"-n", (char*)"rfcdefaults", (char*)"Device.Test.Param" };
    EXPECT_EQ(getparseargsFunc()(12, argv), 0);
    setCallerId("rfcdefaults");
}

TEST(utilsTest, getParamTypeFailure) {
    UtilsCurlFailureGuard guard;
    char * const name = const_cast<char*>("Device.Unknown.Param.NoSuch");
    DATA_TYPE paramType;
    EXPECT_FALSE(getGetParamTypeFunc()(name, &paramType));
}

TEST(utilsTest, setAttributeFailureFallsBackToProvidedType) {
    setCallerId("rfcdefaults");
    UtilsCurlFailureGuard guard;
    char * const name = const_cast<char*>("Device.Unknown.Param.NoSuch");
    char value[] = "1";
    EXPECT_NE(getSetAttributeFunc()(name, 'i', value), 0);
}

TEST(utilsTest, clearAttributeFailure) {
    setCallerId("rfcdefaults");
    UtilsCurlFailureGuard guard;
    char * const name = const_cast<char*>("Device.Unknown.Param.NoSuch");
    EXPECT_NE(getClearAttributeFunc()(name), 0);
}

TEST(utilsTest, getAttributeFailure) {
    setCallerId("rfcdefaults");
    UtilsCurlFailureGuard guard;
    char * const name = const_cast<char*>("Device.Unknown.Param.NoSuch");
    EXPECT_NE(getGetAttributeFunc()(name), 0);
}

TEST(utilsTest, getAttributeLocalOnlyFailure) {
    setCallerId("localOnly");
    char * const name = const_cast<char*>("Device.Unknown.Param.NoSuch");
    EXPECT_NE(getGetAttributeFunc()(name), 0);
    setCallerId("rfcdefaults");
}

TEST(utilsTest, isNodeArrayType) {
    EXPECT_EQ(isNodeArrayType(nullptr), 0);
    cJSON *array = cJSON_CreateArray();
    cJSON *object = cJSON_CreateObject();
    EXPECT_NE(isNodeArrayType(array), 0);
    EXPECT_EQ(isNodeArrayType(object), 0);
    cJSON_Delete(array);
    cJSON_Delete(object);
}

TEST(utilsTest, saveToFileEmptyFormatFails) {
    cJSON *array = cJSON_CreateArray();
    EXPECT_EQ(saveToFile(array, "", "name"), 0);
    cJSON_Delete(array);
}

TEST(utilsTest, saveToFileUnopenablePathFails) {
    cJSON *array = cJSON_CreateArray();
    EXPECT_EQ(saveToFile(array, "/nonexistent_dir_rfc_ut/%s", "name"), 0);
    cJSON_Delete(array);
}

TEST(utilsTest, MuteStderrAndWriteValueLine) {
    int savedStderr = dup(STDERR_FILENO);
    ASSERT_GE(savedStderr, 0);
    getMuteStderrFunc()();
    getWriteValueLineFunc()("value");
    getWriteValueLineFunc()(nullptr);
    std::string longValue(5000, 'v');
    getWriteValueLineFunc()(longValue.c_str());
    dup2(savedStderr, STDERR_FILENO);
    close(savedStderr);
}

TEST(utilsTest, cleanAllFiles) {
    std::remove("/opt/secure/RFC/tr181store.ini");
    std::remove("/opt/secure/RFC/tr181localstore.ini");
    std::remove("/opt/secure/RFC/bootstrap.ini");
    std::remove("/etc/rfcdefaults/rfcdefaults.ini");
    std::remove("/opt/secure/RFC/.RFC_LIST_SNMP2WL.ini");
}


GTEST_API_ int main(int argc, char *argv[]){
    ::testing::InitGoogleTest(&argc, argv);

    cout << "Starting GTEST===========================>" << endl;
    return RUN_ALL_TESTS();
}
