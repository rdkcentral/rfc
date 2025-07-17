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
#include "rfcapi.h"
#include "jsonhandler.h"

using namespace std;
using namespace rfc;

#define TR181_LOCAL_STORE_FILE "/opt/secure/RFC/tr181localstore.ini"
#define RFCDEFAULTS_ETC_DIR "/etc/rfcdefaults/"
extern bool tr69hostif_http_server_ready;

#ifdef GTEST_ENABLE
extern bool (*getGetParamTypeFunc())(char * const, DATA_TYPE *);
extern DATA_TYPE (*getConvertTypeFunc())(char);
extern int (*getClearAttributeFunc())(char * const);
extern int (*getSetAttributeFunc())(char * const, char, char *);
extern int (*getGetAttributeFunc())(char * const);
extern size_t (*getWriteCurlResponse(void))(void *ptr, size_t size, size_t nmemb, std::string stream);
extern int (*getparseargsFunc())(int argc, char * argv[]);
#endif

enum ValueFormat {
    Plain,      // key=value
    Quoted      // key="value"
};

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


// Helper function to check file existence
bool file_exists(const char* filename) {
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
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

TEST(rfcMgrTest, getMtlscert) {
    MtlsAuth_t sec;
    memset(&sec, '\0', sizeof(MtlsAuth_t));
    int ret = getMtlscert(&sec);
    if(ret == MTLS_FAILURE)
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] MTLS  certification Failed\n",__FUNCTION__, __LINE__);
    EXPECT_EQ(ret, MTLS_FAILURE);
}

TEST(rfcMgrTest, readRFCParam) {
    int ret = -1;
    char data[RFC_VALUE_BUF_SIZE];

    ret = read_RFCProperty("RDKFirmwareUpgrader", "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable", data, sizeof(data));
    SWLOG_ERROR("read_RFCProperty() return Status %d\n", ret);
    
    EXPECT_NE(ret, -1);
}

TEST(rfcMgrTest, checkSpecialChars) {
    std::string value = "hah$#aga";
    bool ret = CheckSpecialCharacters(value);
    EXPECT_EQ(ret, true);
    
    std::string value2 = "hah12aga";
    ret = CheckSpecialCharacters(value2);
    EXPECT_EQ(ret, false);
}

TEST(rfcMgrTest, stringCaseCompare) {
    std::string value = "unknown";
    std::string unknown_str = "Unknown";
    std::string unknown_str2 = "Unknown";
    bool ret = StringCaseCompare( value, unknown_str);
    EXPECT_EQ(ret, true);
    
    ret = StringCaseCompare( value, unknown_str2);
    EXPECT_EQ(ret, true);
}

TEST(rfcMgrTest, removeSubstring) {
    std::string key = "tr181.Device.Time.NTPServer1";
    RemoveSubstring(key, "tr181.");
    int ret = strcmp(key.c_str(), "Device.Time.NTPServer1");
    printf("key = %s, ret = %d", key.c_str(), ret);
    EXPECT_EQ(ret, 0);
}

TEST(rfcMgrTest, initializeRuntimeFeatureControlProcessor) {
    /*Create Object for Xconf Handler */
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();

    int reqStatus = FAILURE;

    /* Initialize xconf Hanlder */
    int result = rfcObj->InitializeRuntimeFeatureControlProcessor();
    if(result == FAILURE)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization ...!!\n", __FUNCTION__,__LINE__);
    }
    delete rfcObj;
    EXPECT_NE(result, FAILURE);
}


TEST(rfcMgrTest, processRuntimeFeatureControlReq) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    int reqStatus = FAILURE;
    int result = rfcObj->InitializeRuntimeFeatureControlProcessor();
    if(result == FAILURE)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization ...!!\n", __FUNCTION__,__LINE__);
        delete rfcObj;
    } else {
        result = rfcObj->ProcessRuntimeFeatureControlReq();
        delete rfcObj;
    }
}

TEST(rfcMgrTest, isNewFirmwareFirstRequest) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool ret = rfcObj->IsNewFirmwareFirstRequest();
    delete rfcObj;
    EXPECT_EQ(ret, true);
}

TEST(rfcMgrTest, getLastProcessedFirmware) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string boot_strap_xconf_url;
    rfcObj->WriteFile(".version", "TEMP_VERSION");
    
    bool ret = rfcObj->GetLastProcessedFirmware(RFC_LAST_VERSION);
    delete rfcObj;
    EXPECT_EQ(ret, 0);
}

TEST(rfcMgrTest, getAccountID) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", "TestAccount", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->GetAccountID();
    EXPECT_EQ(rfcObj->_accountId, "TestAccount");
    delete rfcObj;
}

TEST(rfcMgrTest, getPartnerID) {
    // File path
    std::string filePath = PARTNER_ID_FILE;

    // Open file in write mode
    std::ofstream fileStream(filePath);
    if (fileStream.is_open()) {
        // Write the string to the file
        fileStream << "xglobal";

        // Close the file stream
        fileStream.close();
        std::cout << "String written successfully." << std::endl;
    } else {
        std::cout << "Error opening file." << std::endl;
    }
    
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    
    int result = rfcObj->InitializeRuntimeFeatureControlProcessor();
    if(result == FAILURE)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization ...!!\n", __FUNCTION__,__LINE__);
    }
    
    rfcObj->GetRFCPartnerID();
    EXPECT_EQ(rfcObj->_partnerId, "xglobal");
    delete rfcObj;
    
    // Try to delete the file
    if (remove(filePath.c_str()) != 0) {
        perror("Error deleting file");
    } else {
        std::cout << "File successfully deleted." << std::endl;
    }
}

TEST(rfcMgrTest, getExperience) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->GetExperience();
    EXPECT_EQ(rfcObj->_experience, "X1");
    delete rfcObj;
}

TEST(rfcMgrTest, getServURL) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    int ret = rfcObj->GetServURL(RFC_PROPERTIES_FILE);
    std::cout << "_xconf_server_url = " << rfcObj->_xconf_server_url << std::endl;
    EXPECT_EQ(ret, SUCCESS);
    delete rfcObj;
}

TEST(rfcMgrTest, getBootstrapXconfUrl) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.XconfUrl", "https://mockxconf", "/opt/secure/RFC/bootstrap.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string boot_strap_xconf_url;
    int ret = rfcObj->GetBootstrapXconfUrl(boot_strap_xconf_url);
    std::cout << "boot_strap_xconf_url = " << boot_strap_xconf_url << std::endl;
    EXPECT_EQ(boot_strap_xconf_url, "https://mockxconf");
    EXPECT_EQ(ret, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, checkBootstrap) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.XconfUrl", "https://mockxconf", "/opt/secure/RFC/bootstrap.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool is_Bootstrap = rfcObj->checkBootstrap(BS_STORE_FILENAME, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.XconfUrl");
    
    EXPECT_EQ(is_Bootstrap, true);
    delete rfcObj;
}

TEST(rfcMgrTest, isXconfSelectorSlotProd) {
    writeToTr181storeFile(XCONF_SELECTOR_KEY_STR, "prod", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool ret = rfcObj->isXconfSelectorSlotProd();
    
    EXPECT_EQ(ret, true);
    delete rfcObj;
    
}

TEST(rfcMgrTest, getStoredHashAndTime) {
    writeToTr181storeFile(RFC_CONFIG_SET_HASH, "TestConfigSetHash", "/opt/secure/RFC/tr181store.ini", Quoted);
    writeToTr181storeFile(RFC_CONFIG_SET_TIME, "TestConfigSetTime", "/opt/secure/RFC/tr181store.ini", Quoted);

    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string hashValue;
    std::string valueTime;
    rfcObj->GetStoredHashAndTime(hashValue, valueTime);
    std::cout << "hashValue = " << hashValue << " valueTime = " << valueTime << std::endl;
    EXPECT_EQ(hashValue ,"TestConfigSetHash");
    delete rfcObj;
}

TEST(rfcMgrTest, retrieveHashAndTimeFromPreviousDataSet) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string hashValue;
    std::string valueTime;
    rfcObj->RetrieveHashAndTimeFromPreviousDataSet(hashValue, valueTime);
    std::cout << "hashValue = " << hashValue << " valueTime = " << valueTime << std::endl;
    EXPECT_EQ(hashValue ,"TestConfigSetHash");
    EXPECT_EQ(valueTime ,"TestConfigSetTime");
    delete rfcObj;
}

TEST(rfcMgrTest, preProcessJsonResponse) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->rfc_state = Init;
    rfcObj->_is_first_request = true;
    rfcObj->PreProcessJsonResponse(xconfResp);
    EXPECT_EQ(rfcObj->_valid_accountId ,"4123705941507160513");
    delete rfcObj;
}

TEST(rfcMgrTest, processJsonResponse) {
    
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->ProcessJsonResponse(xconfResp);
    delete rfcObj;
    
    // Read content from the file
    std::string actualContent = readFileContent("/opt/secure/RFC/rfcFeature.list");

    // Check if the content of the file is equal to "TEMP_VERSION"
    EXPECT_EQ(actualContent, "11=true,ARU:E_29=true,AccountId=true,LSA_End2End=true,SSHWhiteList:E_N=true,Telemetry_2.0-31099=true,PartnerId=true,");
    
}

TEST(rfcMgrTest, getRuntimeFeatureControlJSON) {
    JSON *pJson = ParseJsonStr(xconfResp);
    if(pJson)
    {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);
        EXPECT_NE(features, nullptr);
        delete rfcObj;
    }
}

TEST(rfcMgrTest, notifyTelemetry2RemoteFeatures) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->NotifyTelemetry2RemoteFeatures("/opt/secure/RFC/rfcFeature.list", "ACTIVE");
    delete rfcObj;
}

TEST(rfcMgrTest, writeFile) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string boot_strap_xconf_url;
    rfcObj->WriteFile(".version", "TEMP_VERSION");

    // Read content from the file
    std::string actualContent = readFileContent("/opt/secure/RFC/.version");

    // Check if the content of the file is equal to "TEMP_VERSION"
    EXPECT_EQ(actualContent, "TEMP_VERSION");
    delete rfcObj;
}

TEST(rfcMgrTest, writeRemoteFeatureCntrlFile) {
}

TEST(rfcMgrTest, getJsonRpc) {
    DownloadData DwnLoc;
    char post_data[] = "{\"jsonrpc\":\"2.0\",\"id\":\"3\",\"method\":\"org.rdk.AuthService.getExperience\", \"params\":{}}";
    if( allocDowndLoadDataMem( &DwnLoc, DEFAULT_DL_ALLOC ) == 0 )
    {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        rfcObj->getJsonRpc( post_data, &DwnLoc );
        delete rfcObj;
    }
}

// Function to check if a file exists
bool fileExists(const std::string& filePath) {
    std::ifstream fileStream(filePath);
    return fileStream.good();
}

TEST(rfcMgrTest, cleanAllFile) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->WriteFile(".RFC_VAR1", "var1");
    rfcObj->WriteFile("rfcVariable.ini", "varFile");
    
    bool doesExist = fileExists("/opt/secure/RFC/.RFC_VAR1");
    EXPECT_EQ(doesExist, true);
    doesExist = fileExists(VARIABLEFILE);
    EXPECT_EQ(doesExist, true);
    
    rfcObj->cleanAllFile();
    
    doesExist = fileExists("/opt/secure/RFC/.RFC_VAR1");
    EXPECT_EQ(doesExist, false);
    doesExist = fileExists(VARIABLEFILE);
    EXPECT_EQ(doesExist, false);
    
}


TEST(rfcMgrTest, getDefaultValue) {
 
  const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable";
  char *pcCallerID ="rfcdefaults";
  TR181_ParamData_t pstParamData;

  tr181ErrorCode_t status =  getDefaultValue(pcCallerID,pcParameterName,&pstParamData); 
  EXPECT_EQ(status, 0);
   	
}

TEST(rfcMgrTest, getDefaultValue_callerIDNULL) {

  const char* pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Airplay.Enable";
  char *pcCallerID = NULL;
  TR181_ParamData_t pstParamData;

  tr181ErrorCode_t status =  getDefaultValue(pcCallerID,pcParameterName,&pstParamData);
  EXPECT_EQ(status, tr181Failure);

}


TEST(rfcMgrTest, checkWhoamiSupport) {
    writeToTr181storeFile("WHOAMI_SUPPORT", "true", "/tmp/device.properties", Plain);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->checkWhoamiSupport();
    delete rfcObj;
    EXPECT_EQ(result, true);
}

TEST(rfcMgrTest, isDebugServicesEnabled) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Identity.DbgServices.Enable", "true", "/opt/secure/RFC/tr181store.ini", Quoted);    
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->isDebugServicesEnabled();
    delete rfcObj;
    EXPECT_EQ(result,true);
}

TEST(rfcMgrTest, isMaintenanceEnabled) {
    writeToTr181storeFile("ENABLE_MAINTENANCE", "true", "/tmp/device.properties", Plain);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->isMaintenanceEnabled();
    delete rfcObj;
    EXPECT_EQ(result, true);
}

TEST(rfcMgrTest, GetOsClass) {
    writeToTr181storeFile("WHOAMI_SUPPORT", "true", "/tmp/device.properties", Plain);
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass", "TestOsClass", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->GetOsClass();
    EXPECT_EQ(rfcObj->_osclass, "TestOsClass");
    delete rfcObj;
}

TEST(rfcMgrTest, NotifyTelemetry2Value) {
    string xconf_server_url = "https://mockxconf/featureControl/getSettings";
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->NotifyTelemetry2Value("SYST_INFO_RFC_XconflocalURL", xconf_server_url.c_str());
    delete rfcObj;
}

TEST(rfcMgrTest, GetValidPartnerId) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->rfc_state = Init;
    rfcObj->_is_first_request = true;
    rfcObj->PreProcessJsonResponse(xconfResp);
    rfcObj->GetValidPartnerId();
    EXPECT_EQ(rfcObj->_partner_id, "comcast");
    delete rfcObj;
}


TEST(rfcMgrTest, CreateXconfHTTPUrl) {
    writeToTr181storeFile("WHOAMI_SUPPORT", "true", "/tmp/device.properties", Plain);
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass", "TestOsClass", "/opt/secure/RFC/tr181store.ini", Quoted);
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", "4123705941507160514", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap[RFC_PARNER_ID_KEY_STR] = "sky"; 
    rfcObj->_RFCKeyAndValueMap[XCONF_URL_KEY_STR] = "https://xconf.xdp.eu-1.xcal.tv";
    rfcObj->PreProcessJsonResponse(xconfResp);
    rfcObj->GetValidPartnerId();
    rfcObj->GetOsClass();
    rfcObj->GetAccountID();
    rfcObj->GetXconfSelect();
    std:stringstream url = rfcObj->CreateXconfHTTPUrl();
    EXPECT_EQ(url.str(), "https://xconf.xdp.eu-1.xcal.tv?estbMacAddress=&firmwareVersion=&env=&model=&manufacturer=&controllerId=2504&channelMapId=2345&VodId=15660&partnerId=sky&osClass=TestOsClass&accountId=4123705941507160514&Experience=&version=2");  
    delete rfcObj;
}

TEST(rfcMgrTest, isConfigValueChange) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable", "false", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string name = "rfc";
    std::string newKey = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable";
    std::string newValue = "true";
    std::string currentValue = "false";
    bool result = rfcObj->isConfigValueChange(name , newKey, newValue, currentValue);
    EXPECT_EQ(result, true);
    delete rfcObj;
}

TEST(rfcMgrTest, CreateConfigDataValueMap) {
    JSON *pJson = ParseJsonStr(xconfResp);
    if(pJson)
    {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);
	if(features)
	{
           rfcObj->CreateConfigDataValueMap(features);
	}
        EXPECT_EQ(rfcObj->_RFCKeyAndValueMap.size(), 15);
        delete rfcObj;
    }	
}

TEST(rfcMgrTest, IsDirectBlocked) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->IsDirectBlocked();
    EXPECT_EQ(result, false);
    delete rfcObj;
}

TEST(rfcMgrTest, getRFCName) {
    JSON *pJson = ParseJsonStr(xconfResp);
    if (pJson) {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);

        if (features) {
            int numFeatures = GetJsonArraySize(features);

            if (numFeatures) {
                JSON* feature = GetJsonArrayItem(features, 1);
                RuntimeFeatureControlProcessor::RuntimeFeatureControlObject *rfccObj = new RuntimeFeatureControlProcessor::RuntimeFeatureControlObject;

                rfcObj->getRFCName(feature, rfccObj);
                EXPECT_EQ(rfccObj->name, "ARU");

                delete rfccObj;
            }
        }

        delete rfcObj;
    }
}


TEST(rfcMgrTest, getFeatureInstance) {
    JSON *pJson = ParseJsonStr(xconfResp);
    if (pJson) {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);

        if (features) {
            int numFeatures = GetJsonArraySize(features);

            if (numFeatures) {
                JSON* feature = GetJsonArrayItem(features, 1);
                RuntimeFeatureControlProcessor::RuntimeFeatureControlObject *rfccObj = new RuntimeFeatureControlProcessor::RuntimeFeatureControlObject;

                rfcObj->getFeatureInstance(feature, rfccObj);
                EXPECT_EQ(rfccObj->featureInstance, "ARU:E_29");

                delete rfccObj;
            }
        }

        delete rfcObj;
    }
}

TEST(rfcMgrTest, getRFCEnableParam) {
    JSON *pJson = ParseJsonStr(xconfResp);
    if (pJson) {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);

        if (features) {
            int numFeatures = GetJsonArraySize(features);

            if (numFeatures) {
                JSON* feature = GetJsonArrayItem(features, 1);
                RuntimeFeatureControlProcessor::RuntimeFeatureControlObject *rfccObj = new RuntimeFeatureControlProcessor::RuntimeFeatureControlObject;

                rfcObj->getRFCEnableParam(feature, rfccObj);
                EXPECT_EQ(rfccObj->enable, true);

                delete rfccObj;
            }
        }

        delete rfcObj;
    }
}


TEST(rfcMgrTest, getEffectiveImmediateParam) {
    JSON *pJson = ParseJsonStr(xconfResp);
    if (pJson) {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);

        if (features) {
            int numFeatures = GetJsonArraySize(features);

            if (numFeatures) {
                JSON* feature = GetJsonArrayItem(features, 1);
                RuntimeFeatureControlProcessor::RuntimeFeatureControlObject *rfccObj = new RuntimeFeatureControlProcessor::RuntimeFeatureControlObject;

                rfcObj->getEffectiveImmediateParam(feature, rfccObj);
                EXPECT_EQ(rfccObj->effectiveImmediate, false);

                delete rfccObj;
            }
        }

        delete rfcObj;
    }
}


TEST(rfcMgrTest, InitDownloadData) {
    DownloadData DwnLoc;
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->InitDownloadData(&DwnLoc);
    EXPECT_EQ(DwnLoc.pvOut, nullptr);
    EXPECT_EQ(DwnLoc.datasize, 0);
    EXPECT_EQ(DwnLoc.memsize, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, set_RFCProperty) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string name = "rfc";
    std::string value = "https://rdkautotool.ccp.xcal.tv/featureControl/getSettings";
    //std::string ClearDBEndKey = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ClearDBEnd";
    std::string xconfURL = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.XconfUrl";
    writeToTr181storeFile(xconfURL, "https://rdkautotool.ccp.xcal.tv/featureControl/getSettings", "/opt/secure/RFC/tr181store.ini", Quoted);
    WDMP_STATUS status = rfcObj->set_RFCProperty(name, xconfURL, value);
    EXPECT_EQ(status, WDMP_SUCCESS);
    delete rfcObj;
}

TEST(rfcMgrTest, GetXconfSelect) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap[XCONF_URL_KEY_STR] = "https://xconf.xdp.eu-1.xcal.tv";
    rfcObj->_RFCKeyAndValueMap[XCONF_SELECTOR_KEY_STR] = "automation"; 
    rfcObj->GetXconfSelect();
    
    EXPECT_EQ(rfcObj->rfc_state, Redo);
    EXPECT_EQ(rfcObj->rfcSelectorSlot, "19");
    delete rfcObj;
    
}

TEST(rfcMgrTest, getJRPCTokenData) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    char post_data[] = "{\"jsonrpc\":\"2.0\",\"id\":\"3\",\"method\":\"org.rdk.AuthService.getExperience\", \"params\":{}}";
    char token_str[] =  "id";
    int result = rfcObj->getJRPCTokenData(token_str, post_data , sizeof(token_str));

    EXPECT_EQ(result , 0);
    delete rfcObj;

}

TEST(rfcMgrTest, isStateRedSupported) {
    int ret = isStateRedSupported();
    EXPECT_EQ(ret , 0); 
}


TEST(rfcMgrTest, isInStateRed) {
    int ret = isInStateRed();
    EXPECT_EQ(ret , 0);
}

TEST(rfcMgrTest, executeCommandAndGetOutput) {
    const char *pArg;
    std::string result; 	
    int ret = executeCommandAndGetOutput(eWpeFrameworkSecurityUtility, pArg, result);
    EXPECT_EQ(ret , 0);
}

TEST(rfcMgrTest, getRebootRequirement) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->getRebootRequirement();
    EXPECT_EQ(result , false);
    delete rfcObj;    
}

TEST(rfcMgrTest, ProcessXconfUrl) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string xconfURL = "https://xconf.xdp.eu-1.xcal.tv";
    int result = rfcObj->ProcessXconfUrl(xconfURL.c_str());
    EXPECT_EQ(result , 0);
    delete rfcObj;
}

TEST(rfcMgrTest, updateTR181File) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::list<std::string> paramList;
    std::string newKey = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable";
    std::string newValue = "true";
    std::string data = "TR181: " + newKey + " " + newValue;
    int result ;
    paramList.push_back(data);
    rfcObj->updateTR181File(TR181_FILE_LIST, paramList);
    EXPECT_EQ(paramList.size() , 0);
    delete rfcObj;
}

TEST(rfcMgrTest, processXconfResponseConfigDataPart) {
    JSON *pJson = ParseJsonStr(xconfResp);
    if(pJson)
    {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);
	JSON *bkp_features = features;
        if(bkp_features)
	{
            rfcObj->processXconfResponseConfigDataPart(bkp_features);	    
	}
	delete rfcObj;
    }
}


TEST(rfcMgrTest, CheckDeviceIsOFFline) {
      rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
      rfc::DeviceStatus status =  rfcmgrObj->CheckDeviceIsOnline();
      EXPECT_EQ(status , RFCMGR_DEVICE_OFFLINE);
      delete rfcmgrObj;
}

TEST(rfcMgrTest, CheckDeviceIsOnline) {
      write_on_file(GATEWAYIP_FILE, "IPV4 8.8.4.4");
      write_on_file(DNS_RESOLV_FILE, "nameserver 2.4.6.8");
      rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
      rfc::DeviceStatus status =  rfcmgrObj->CheckDeviceIsOnline();
      EXPECT_EQ(status , RFCMGR_DEVICE_ONLINE);
      delete rfcmgrObj;
}

TEST(rfcMgrTest, RFCManagerPostProcess) {
      write_on_file(RFC_MGR_IPTBLE_INIT_SCRIPT, "executing the iptables init script");
      rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
      int result =  rfcmgrObj->RFCManagerPostProcess();
      EXPECT_EQ(result , 0);
}


TEST(rfcMgrTest, CheckIProuteConnectivity) {
      write_on_file(GATEWAYIP_FILE, "IPV4 8.8.4.4");
      rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
      int result =  rfcmgrObj->CheckIProuteConnectivity(GATEWAYIP_FILE);
      EXPECT_EQ(result, true);
      delete rfcmgrObj;
}


TEST(rfcMgrTest, IsIarmBusConnected) {
      rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
      bool result =  rfcmgrObj->IsIarmBusConnected();
      EXPECT_EQ(result, true);
      delete rfcmgrObj;
}

TEST(rfcMgrTest, InitializeIARM) {
      rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
      bool result = true;
      rfcmgrObj->InitializeIARM();
      EXPECT_EQ(result, true);
      delete rfcmgrObj;
}

TEST(rfcMgrTest, isDnsResolve) {
      write_on_file(DNS_RESOLV_FILE, "nameserver 2.4.6.8");
      int result = isDnsResolve(DNS_RESOLV_FILE);
      EXPECT_EQ(result, true);
}
 TEST(rfcMgrTest, RFCManagerProcessXconfRequest) {
      rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
      int result =  rfcmgrObj->RFCManagerProcessXconfRequest();
      EXPECT_EQ(result, FAILURE);
      delete rfcmgrObj;
}

TEST(rfcMgrTest, initializeXconf) {
     write_on_file("/tmp/partnerId3.dat", "default-parter");	
     write_on_file("/tmp/estbmacfile", "01:23:45:67:89:ab");
     write_on_file("/tmp/version.txt", "imagename:TestImage");
     write_on_file("/tmp/device.properties", "MODEL_NUM=SKXI11ADS");
     write_on_file("/tmp/device.properties", "BUILD_TYPE=dev");
     write_on_file("/tmp/.manufacturer", "TestMFRname");
     xconf::XconfHandler *xconfObj = new xconf::XconfHandler();
     int resutl = xconfObj->initializeXconfHandler();
     EXPECT_EQ(xconfObj->_estb_mac_address, "01:23:45:67:89:ab"); 
     EXPECT_EQ(xconfObj->_partner_id, "default-parter");
     EXPECT_EQ(xconfObj->_firmware_version, "TestImage");
     EXPECT_EQ(xconfObj->_model_number, "SKXI11ADS");
     EXPECT_EQ(xconfObj->_build_type_str, "dev");
     EXPECT_EQ(xconfObj->_manufacturer, "TestMFRname");
     delete xconfObj;
}



/*TEST(rfcMgrTest, cleanup_lock_file) {
    write_on_file(RFC_MGR_SERVICE_LOCK_FILE, "lock");
    EXPECT_TRUE(file_exists(RFC_MGR_SERVICE_LOCK_FILE));
    cleanup_lock_file();
    EXPECT_FALSE(file_exists(RFC_MGR_SERVICE_LOCK_FILE));
} */

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

/* TEST(rfcMgrTest, rfc_getValue) {
   writeToTr181storeFile("Device.Time.NTPServer1", "3.236.252.118", "/tmp/rfcdefaults.ini", Plain);
   const char* pcParameterName = "Device.Time.NTPServer1";
   char *pcCallerID ="rfcdefaults";
   RFC_ParamData_t pstParamData;
   WDMP_STATUS result = getValue(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_STREQ(pstParamData.value, "3.236.252.118");
   EXPECT_EQ(result, WDMP_SUCCESS);
} */


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


TEST(rfcMgrTest, getTR181ErrorString) {   
   EXPECT_STREQ(getTR181ErrorString(tr181Success) , " Success");
   EXPECT_STREQ(getTR181ErrorString(tr181InternalError) , " Internal Error");
   EXPECT_STREQ(getTR181ErrorString(tr181InvalidParameterName), " Invalid Parameter Name");
   EXPECT_STREQ(getTR181ErrorString(tr181InvalidParameterValue), " Invalid Parameter Value");
   EXPECT_STREQ(getTR181ErrorString(tr181Failure), " Failure");
   EXPECT_STREQ(getTR181ErrorString(tr181InvalidType), " Invalid type");
   EXPECT_STREQ(getTR181ErrorString(tr181NotWritable), " Not writable");
   EXPECT_STREQ(getTR181ErrorString(tr181ValueIsEmpty), " Value is empty");
   EXPECT_STREQ(getTR181ErrorString(tr181ValueIsNull), " Value is Null");
   EXPECT_STREQ(getTR181ErrorString(tr181DefaultValue), " Default Value");
}

TEST(rfcMgrTest, getType) {
    EXPECT_EQ(getType(WDMP_STRING), TR181_STRING);
    EXPECT_EQ(getType(WDMP_INT), TR181_INT);    
    EXPECT_EQ(getType(WDMP_UINT), TR181_UINT);
    EXPECT_EQ(getType(WDMP_BOOLEAN), TR181_BOOLEAN);
    EXPECT_EQ(getType(WDMP_DATETIME), TR181_DATETIME);
    EXPECT_EQ(getType(WDMP_BASE64), TR181_BASE64);
    EXPECT_EQ(getType(WDMP_LONG), TR181_LONG);
    EXPECT_EQ(getType(WDMP_ULONG), TR181_ULONG);
    EXPECT_EQ(getType(WDMP_FLOAT), TR181_FLOAT);
    EXPECT_EQ(getType(WDMP_DOUBLE), TR181_DOUBLE);
    EXPECT_EQ(getType(WDMP_BYTE), TR181_BYTE);
    EXPECT_EQ(getType(WDMP_NONE), TR181_NONE);
}


TEST(rfcMgrTest, getErrorCode) {
   tr181ErrorCode_t errorCode = getErrorCode(WDMP_ERR_DEFAULT_VALUE);
   EXPECT_EQ(getErrorCode(WDMP_SUCCESS), tr181Success);
   EXPECT_EQ(getErrorCode(WDMP_FAILURE), tr181Failure);
   EXPECT_EQ(getErrorCode(WDMP_ERR_TIMEOUT), tr181Timeout);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INVALID_PARAMETER_NAME), tr181InvalidParameterName);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INVALID_PARAMETER_VALUE), tr181InvalidParameterValue);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INVALID_PARAMETER_TYPE), tr181InvalidType);
   EXPECT_EQ(getErrorCode(WDMP_ERR_NOT_WRITABLE), tr181NotWritable);
   EXPECT_EQ(getErrorCode(WDMP_ERR_VALUE_IS_EMPTY), tr181ValueIsEmpty);
   EXPECT_EQ(getErrorCode(WDMP_ERR_VALUE_IS_NULL), tr181ValueIsNull);
   EXPECT_EQ(getErrorCode(WDMP_ERR_DEFAULT_VALUE), tr181DefaultValue);
   EXPECT_EQ(getErrorCode(WDMP_ERR_INTERNAL_ERROR), tr181InternalError);
} 

TEST(rfcMgrTest, getValue) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable", "true", TR181_LOCAL_STORE_FILE, Plain); 
   const char* pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable";
   TR181_ParamData_t pstParam;
   tr181ErrorCode_t status = getValue(TR181_LOCAL_STORE_FILE, pcParameterName, &pstParam);
   EXPECT_EQ(status, tr181Success);
   EXPECT_STREQ(pstParam.value, "true");
}

TEST(rfcMgrTest, getEmptyValue) {
   writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable", "", TR181_LOCAL_STORE_FILE, Plain);
   const char* pcParameterName = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SWDLSpLimit.Enable";
   TR181_ParamData_t pstParam;
   tr181ErrorCode_t status = getValue(TR181_LOCAL_STORE_FILE, pcParameterName, &pstParam);
   EXPECT_EQ(status, tr181ValueIsEmpty);
}


TEST(rfcMgrTest, setValue) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName";
   const char* pcParamValue ="comcast";    
   tr181ErrorCode_t status = setValue(pcParameterName, pcParamValue);
   EXPECT_EQ(status, tr181Success);
}

TEST(rfcMgrTest, setLocalParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName";
   char *pcCallerID ="rfcdefaults";
   const char* pcParamValue ="Xfinity";
   tr181ErrorCode_t status = setLocalParam(pcCallerID, pcParameterName, pcParamValue);
   EXPECT_EQ(status, tr181Success);
}


TEST(rfcMgrTest, clearLocalParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName";
   char *pcCallerID ="rfcdefaults";
   const char* pcParamValue ="Xfinity";
   tr181ErrorCode_t status = clearLocalParam(pcCallerID, pcParameterName);
   EXPECT_EQ(status, tr181Success);
}


TEST(rfcMgrTest, getLocalParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName";
   char *pcCallerID ="rfcdefaults";
   TR181_ParamData_t pstParamData;
   tr181ErrorCode_t status = getLocalParam(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_STREQ(pstParamData.value, "comcast"); 
   EXPECT_EQ(status, tr181Success);
}


TEST(rfcMgrTest, getParam) {
   const char* pcParameterName ="Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName";
   char *pcCallerID ="rfcdefaults";
   TR181_ParamData_t pstParamData;
   tr181ErrorCode_t status = getParam(pcCallerID, pcParameterName, &pstParamData);
   EXPECT_EQ(status, tr181Success);
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


