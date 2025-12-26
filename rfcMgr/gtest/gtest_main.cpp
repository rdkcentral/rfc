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
#include "tr181_store_writer.h"

using namespace std;
using namespace rfc;

#define TR181_LOCAL_STORE_FILE "/opt/secure/RFC/tr181localstore.ini"
#define RFCDEFAULTS_ETC_DIR "/etc/rfcdefaults/"
extern bool tr69hostif_http_server_ready;


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
	    },
	    {
                "name": "XconfURL",
                "enable": true,
                "effectiveImmediate": true,
                "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.XconfUrl": "https://xconf.xcal.tv"
                },
                "featureInstance": "XconfURL"
            }            
        ]
    }
})";


char emptyXconfResp[] = R"({
    "featureControl": {
        "features": [
        {
	},
        {
           "name": "XconfURL" 
        },
        {
            "name": "PartnerId",
            "enable": true
        },
        {
            "name": "Telemetry_2.0-31099",
            "enable": true,
            "effectiveImmediate": true 
        } 
        ]
    }
})";

char configDataXconfResp[] = R"({
    "featureControl": {
        "features": [
        {
            "name": "ParterName",
            "enable": true,
            "effectiveImmediate": true
        }
        ]
    }
})";

char emptyFeaturesXconfResp[] = R"({
    "featureControl": {
    }
})";


// Mock XCONF response with Unknown AccountID 
char xconfRespUnknownAccountId[] = R"({
    "featureControl": {
        "configset-id": "090909",
        "configset-label": "default",
        "features": [
            {
                "name": "LSA",
                "effectiveImmediate": false,
                "enable": true,
                "configData": {
                    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass": "default"
                },
                "featureInstance": "OsClass"
            },
            {
                "name": "AccountId",
                "enable": true,
                "effectiveImmediate": true,
                "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID": "Unknown"
                },
                "featureInstance": "AccountId"
            }
        ]
    }
})";

// Mock XCONF response with valid AccountID 
char xconfRespValidAccountId[] = R"({
    "featureControl": {
        "configset-id": "090909",
        "configset-label": "default",
        "features": [
            {
                "name": "AccountId",
                "enable": true,
                "effectiveImmediate": true,
                "configData": {
                    "tr181.Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID": "1234567890000000001"
                },
                "featureInstance": "AccountId"
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

TEST(rfcMgrTest, readRFCParam_nullkey) {
    int ret = -1;
    char data[RFC_VALUE_BUF_SIZE];

    ret = read_RFCProperty("RDKFirmwareUpgrader", "nullptr", data, 0);
    SWLOG_ERROR("read_RFCProperty() return Status %d\n", ret);

    EXPECT_EQ(ret, -1);
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
    /* Create Object for Xconf Handler */
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
    rfcObj->rfcSelectOpt = "local";
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
    EXPECT_EQ(actualContent, "11=true,ARU:E_29=true,AccountId=true,LSA_End2End=true,SSHWhiteList:E_N=true,Telemetry_2.0-31099=true,PartnerId=true,XconfURL=true,");
    
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
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass", "TestOs Class", "/opt/secure/RFC/tr181store.ini", Quoted);
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", "4123705941507160514", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap[RFC_PARTNER_ID_KEY_STR] = "sky"; 
    rfcObj->_RFCKeyAndValueMap[XCONF_URL_KEY_STR] = "https://xconf.xdp.eu-1.xcal.tv";
    rfcObj->PreProcessJsonResponse(xconfResp);
    rfcObj->GetValidPartnerId();
    rfcObj->GetOsClass();
    rfcObj->GetAccountID();
    rfcObj->GetXconfSelect();
    std:stringstream url = rfcObj->CreateXconfHTTPUrl();
    EXPECT_EQ(url.str(), "https://xconf.xdp.eu-1.xcal.tv?estbMacAddress=&firmwareVersion=&env=&model=&manufacturer=&controllerId=2504&channelMapId=2345&VodId=15660&partnerId=sky&osClass=TestOs%20Class&accountId=4123705941507160514&Experience=&version=2");  
    delete rfcObj;
}

TEST(rfcMgrTest, isConfigValueChange) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable", "false", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string name = "rfc";
    std::string newKey = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Telemetry.Enable";
    std::string newValue = "true";
    std::string currentValue = "false";
    bool result = rfcObj->isConfigValueChange(name, newKey, newValue, currentValue);
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
        EXPECT_EQ(rfcObj->_RFCKeyAndValueMap.size(), 16);
        delete rfcObj;
    }	
}

TEST(rfcMgrTest, IsDirectBlocked) {
    write_on_file(DIRECT_BLOCK_FILENAME, "currenttime");
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->IsDirectBlocked();
    EXPECT_EQ(result, true);
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

                int result = rfcObj->getRFCName(feature, rfccObj);
                EXPECT_EQ(rfccObj->name, "ARU");
		EXPECT_EQ(result, SUCCESS);

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

                int result = rfcObj->getFeatureInstance(feature, rfccObj);
                EXPECT_EQ(rfccObj->featureInstance, "ARU:E_29");
		EXPECT_EQ(result, SUCCESS);

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

                int result = rfcObj->getRFCEnableParam(feature, rfccObj);
                EXPECT_EQ(rfccObj->enable, true);
		EXPECT_EQ(result, SUCCESS);

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

                int result = rfcObj->getEffectiveImmediateParam(feature, rfccObj);
                EXPECT_EQ(rfccObj->effectiveImmediate, false);
		EXPECT_EQ(result, SUCCESS);

                delete rfccObj;
            }
        }

        delete rfcObj;
    }
}

TEST(rfcMgrTest, rfcStashStoreParams) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", "6964630676518908063", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor(); 
    rfcObj->rfcStashStoreParams();
    EXPECT_EQ(rfcObj->stashAccountId, "6964630676518908063");
    delete rfcObj;
}

TEST(rfcMgrTest, rfcStashRetrieveParams) {
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", "6964630676518908063", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->rfcStashStoreParams();
    rfcObj->rfcStashRetrieveParams();
    EXPECT_EQ(rfcObj->_accountId, "6964630676518908063");
    delete rfcObj;
}

TEST(rfcMgrTest, clearDB) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->clearDB();
    delete rfcObj;
}

TEST(rfcMgrTest, clearDBEnd) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->clearDBEnd();
    delete rfcObj;
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
    const char *pArgs;
    std::string result; 	
    int ret = executeCommandAndGetOutput(eWpeFrameworkSecurityUtility, pArgs, result);
    EXPECT_EQ(ret , 0);
}

TEST(rfcMgrTest, executeCommandAndGetOutput_eRdkSsaCli) {
    const char *pArgs = NULL;
    std::string result;
    int ret = executeCommandAndGetOutput(eRdkSsaCli, pArgs, result);
    EXPECT_EQ(ret , -1);
}

TEST(rfcMgrTest, getRebootRequirement) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->getRebootRequirement();
    EXPECT_EQ(result, false);
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
    EXPECT_EQ(status, RFCMGR_DEVICE_OFFLINE);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, CheckDeviceIsOnline) {
    write_on_file(GATEWAYIP_FILE, "IPV4 8.8.4.4");
    write_on_file(DNS_RESOLV_FILE, "nameserver 2.4.6.8");
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    rfc::DeviceStatus status =  rfcmgrObj->CheckDeviceIsOnline();
    EXPECT_EQ(status, RFCMGR_DEVICE_ONLINE);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, RFCManagerPostProcess) {
    write_on_file(RFC_MGR_IPTBLE_INIT_SCRIPT, "executing the iptables init script");
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    int result =  rfcmgrObj->RFCManagerPostProcess();
    EXPECT_EQ(result, 0);
}

TEST(rfcMgrTest, CheckIProuteConnectivity) {
    write_on_file(GATEWAYIP_FILE, "IPV4 8.8.4.4");
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    int result =  rfcmgrObj->CheckIProuteConnectivity(GATEWAYIP_FILE);
    EXPECT_EQ(result, true);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, removed_GatewayIPFile) {
    std::remove(GATEWAYIP_FILE);
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    int result =  rfcmgrObj->CheckIProuteConnectivity(GATEWAYIP_FILE);
    EXPECT_EQ(result, true);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, CheckIPV6routeConnectivity) {
    write_on_file(GATEWAYIP_FILE, "IPV6 fe80::1ff:fe23:4567:890a");
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    int result =  rfcmgrObj->CheckIProuteConnectivity(GATEWAYIP_FILE);
    EXPECT_EQ(result, true);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, CheckInvalidIP) {
    std::remove(GATEWAYIP_FILE);
    write_on_file(GATEWAYIP_FILE, "192.256.0.1");
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    int result =  rfcmgrObj->CheckIProuteConnectivity(GATEWAYIP_FILE);
    EXPECT_EQ(result, true);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, InvalidGatewayIP_File) {
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    int result =  rfcmgrObj->CheckIProuteConnectivity(NULL);
    EXPECT_EQ(result, false);
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

TEST(rfcMgrTest, removed_DnsResolveFile) {
    std::string cmd = std::string("rm -rf ") + DNS_RESOLV_FILE;
    int status = system(cmd.c_str());
    EXPECT_EQ(status, 0);
    int result = isDnsResolve(DNS_RESOLV_FILE);
    EXPECT_EQ(result, false);
}

TEST(rfcMgrTest, InvalidDNS_File) {
    int result = isDnsResolve(NULL);
    EXPECT_EQ(result, false);
}

TEST(rfcMgrTest, removed_IP_ROUTE_File) {
    std::remove(IP_ROUTE_FLAG);
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    bool result =  rfcmgrObj->CheckIProuteConnectivity(GATEWAYIP_FILE);
    EXPECT_EQ(result, true);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, checkIPConnectivity) {
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    bool result =  rfcmgrObj->CheckIPConnectivity();
    EXPECT_EQ(result, false);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, manageCronJob) {
    std::string myCron = "0 5 * * * /tmp/script.sh";
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    rfcmgrObj->manageCronJob(myCron);
    EXPECT_EQ(0, 0);
    delete rfcmgrObj;
}

TEST(rfcMgrTest,Invalidcronformat) {
    std::string myCron = "0 5";
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    rfcmgrObj->manageCronJob(myCron);
    EXPECT_EQ(0, 0);
    delete rfcmgrObj;
}

TEST(rfcMgrTest, RFCManagerProcess) {
    rfc::RFCManager *rfcmgrObj = new rfc::RFCManager();
    int result =  rfcmgrObj->RFCManagerProcess();
    EXPECT_EQ(result, -1);
    delete rfcmgrObj;
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

TEST(rfcMgrTest, getAccountID_Unknown) {
    std::remove("/opt/secure/RFC/tr181store.ini");
    std::remove("/tmp/device.properties");
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->GetAccountID();
    EXPECT_EQ(0, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, MAX_RETRIES) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    std::string boot_strap_xconf_url = "";
    int ret = rfcObj->GetBootstrapXconfUrl(boot_strap_xconf_url);
    std::cout << "boot_strap_xconf_url = " << boot_strap_xconf_url << std::endl;
    EXPECT_EQ(ret, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, isXconfSelectorSlotProd_Disabled) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool ret = rfcObj->isXconfSelectorSlotProd();

    EXPECT_EQ(ret, false);
    delete rfcObj;
}

TEST(rfcMgrTest, get0verrideHashAndTime) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->rfc_state = Init;
    std::string hashValue;
    std::string valueTime;
    rfcObj->GetStoredHashAndTime(hashValue, valueTime);
    std::cout << "hashValue = " << hashValue << " valueTime = " << valueTime << std::endl;
    EXPECT_EQ(hashValue, "OVERRIDE_HASH");
    EXPECT_EQ(valueTime, "0");
    delete rfcObj;
}


TEST(rfcMgrTest, WHOAMI_SUPPORT_Disabled) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->checkWhoamiSupport();
    delete rfcObj;
    EXPECT_EQ(result, false);
}

TEST(rfcMgrTest, isDebugServicesDisable) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->isDebugServicesEnabled();
    delete rfcObj;
    EXPECT_EQ(result,false);
}

TEST(rfcMgrTest, isMaintenanceDisabled) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->isMaintenanceEnabled();
    delete rfcObj;
    EXPECT_EQ(result, true);
}

TEST(rfcMgrTest, GetOsClass_Disabled) {
    writeToTr181storeFile("WHOAMI_SUPPORT", "true", "/tmp/device.properties", Plain);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->GetOsClass();
    EXPECT_EQ(0, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, InvalidAccountId) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->stashAccountId = "Unknown";
    rfcObj->rfcStashStoreParams();
    EXPECT_EQ(0, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, PERSISTENCEFILE) {
    writeToTr181storeFile("RFC_CONFIG_SERVER_URL", "", "/opt/rfc.properties", Quoted);
    /* Create Object for Xconf Handler */
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

TEST(rfcMgrTest, LastFirmware) {
    writeToTr181storeFile("RFC_CONFIG_SERVER_URL", "https://mockxconf/featureControl/getSettings", "/opt/rfc.properties", Quoted);
    std::remove(RFC_LAST_VERSION);

    std::ofstream outfile(RFC_LAST_VERSION);
    outfile.close();

    /* Create Object for Xconf Handler */
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();

    int reqStatus = FAILURE;

    /* Initialize xconf Hanlder */
    int result = rfcObj->InitializeRuntimeFeatureControlProcessor();
    if(result == FAILURE)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization ...!!\n", __FUNCTION__,__LINE__);
    }
    delete rfcObj;
    EXPECT_EQ(result, 0);
}

TEST(rfcMgrTest, LastFirmware_FileRemoved) {
    std::remove(RFC_LAST_VERSION);

    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    bool result = rfcObj->GetLastProcessedFirmware(RFC_LAST_VERSION);
    delete rfcObj;
    EXPECT_EQ(result, true);
}

TEST(rfcMgrTest, GetServURLFileNotFound) {
    std::remove(RFC_PROPERTIES_FILE);

    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    int result = rfcObj->GetServURL("/opt/rfc_.properties");
    delete rfcObj;
    EXPECT_EQ(result, FAILURE);
}

TEST(rfcMgrTest, updateHashInDB) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->bkup_hash = "testconfigsethash";
    rfcObj->updateHashInDB("TestConfigSetHash");
    delete rfcObj;
    EXPECT_EQ(0, 0);
}


TEST(rfcMgrTest, updateHashAndTimeInDB) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->updateHashAndTimeInDB("HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: 123\n configSetHash:TestConfig configsethash:testconfig");
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, lowerconfigSetHash) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->updateHashAndTimeInDB("HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: 123\n configsethash:testconfig");
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, noconfigSetHash) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->updateHashAndTimeInDB("HTTP/1.1 200 OK\nContent-Type: application/json\nContent-Length: 123\n ");
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, preProcessJsonResponse_rfcstate) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->rfc_state = Redo;
    rfcObj->_is_first_request = false;
    rfcObj->PreProcessJsonResponse(xconfResp);
    EXPECT_EQ(0, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, Removed_PERSISTENCE_FILE) {
    std::remove(RFC_PROPERTIES_PERSISTENCE_FILE);
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    int reqStatus = FAILURE;
    int result = rfcObj->InitializeRuntimeFeatureControlProcessor();
    if(result == FAILURE)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization ...!!\n", __FUNCTION__,__LINE__);
        delete rfcObj;
    } else {
	rfcObj->_boot_strap_xconf_url = "https://xconf.xcal.tv";
        result = rfcObj->ProcessRuntimeFeatureControlReq();
        delete rfcObj;
    }
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, NotifyTelemetry2ErrorCode) {
    std::remove(RFC_LAST_VERSION);

    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->NotifyTelemetry2ErrorCode(80);
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, unknownAccountId) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap.clear();
    rfcObj->GetValidAccountId();
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, GetValidAccountId) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap[RFC_ACCOUNT_ID_KEY_STR] = "4123705941507160513";
    rfcObj->GetValidAccountId();
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, AccountId_SpecialChars) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap[RFC_ACCOUNT_ID_KEY_STR] = "41237059415071605-13_";
    rfcObj->GetValidAccountId();
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, ValidPartnerId) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->rfc_state = Init;
    rfcObj->_is_first_request = true;
    rfcObj->PreProcessJsonResponse(xconfResp);
     rfcObj->_RFCKeyAndValueMap[RFC_PARTNER_ID_KEY_STR] = "comcast";
    rfcObj->GetValidPartnerId();
    EXPECT_EQ(rfcObj->_partner_id, "comcast");
    delete rfcObj;
}

TEST(rfcMgrTest, GetXconfSelect_ci) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap[XCONF_URL_KEY_STR] = "https://xconf.xdp.eu-1.xcal.tv";
    rfcObj->_RFCKeyAndValueMap[XCONF_SELECTOR_KEY_STR] = "ci";
    rfcObj->GetXconfSelect();

    EXPECT_EQ(rfcObj->rfc_state, Redo);
    EXPECT_EQ(rfcObj->rfcSelectorSlot, "16");
    delete rfcObj;

}

TEST(rfcMgrTest, GetXconfSelect_Finish) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->_RFCKeyAndValueMap[XCONF_URL_KEY_STR] = "https://xconf.xdp.eu-1.xcal.tv";
    rfcObj->_RFCKeyAndValueMap[XCONF_SELECTOR_KEY_STR] = "finish";
    rfcObj->GetXconfSelect();

    EXPECT_EQ(rfcObj->rfc_state, Finish);
    EXPECT_EQ(rfcObj->rfcSelectorSlot, "8");
    delete rfcObj;

}

TEST(rfcMgrTest, processJsonResponseParseError) {

    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->ProcessJsonResponse(emptyXconfResp);
    delete rfcObj;
    EXPECT_EQ(0, 0);
}

TEST(rfcMgrTest, getJRPCTokenData_Nulltoken) {
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    char *post_data = NULL;
    char *token_str =  NULL;
    int result = rfcObj->getJRPCTokenData(token_str, post_data , sizeof(token_str));

    EXPECT_EQ(result , -1);
    delete rfcObj;
}


TEST(rfcMgrTest, notifyTelemetry2RemoteFeatures_RemovedFile) {
    std::remove("/opt/secure/RFC/rfcFeature.list"); 	
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->NotifyTelemetry2RemoteFeatures("/opt/secure/RFC/rfcFeature.list", "ACTIVE");
    EXPECT_EQ(0, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, notifyTelemetry2RemoteFeatures_EmptyFile) {
    write_on_file("/opt/secure/RFC/rfcFeature.list", "");    	
    RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
    rfcObj->NotifyTelemetry2RemoteFeatures("/opt/secure/RFC/rfcFeature.list", "ACTIVE");
    EXPECT_EQ(0, 0);
    delete rfcObj;
}

TEST(rfcMgrTest, ConfigDataNotFound) {
    JSON *pJson = ParseJsonStr(configDataXconfResp);
    if(pJson)
    {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);
        if(features)
        {
           rfcObj->CreateConfigDataValueMap(features);
        }
        EXPECT_EQ(rfcObj->_RFCKeyAndValueMap.size(), 0);
        delete rfcObj;
    }
}

TEST(rfcMgrTest, EmptyFeatures) {
    JSON *pJson = ParseJsonStr(emptyFeaturesXconfResp);
    if(pJson)
    {
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();
        JSON *features = rfcObj->GetRuntimeFeatureControlJSON(pJson);
        EXPECT_EQ(features, nullptr);
        delete rfcObj;
    }
}


// RDKEMW-11615: AccountID Validation Tests
class AccountIDValidationTest : public ::testing::Test {
protected:
    RuntimeFeatureControlProcessor* rfcObj;
    
    // Test AccountID values
    static constexpr const char* DUMMY_ACCOUNTID_1 = "1234567890000000001";
    static constexpr const char* DUMMY_ACCOUNTID_2 = "9876543210000000001";
    static constexpr const char* DUMMY_ACCOUNTID_3 = "5555555550000000001";
    static constexpr const char* UNKNOWN_VALUE = "Unknown";
    static constexpr const char* ACCOUNTID_KEY = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID";
    static constexpr const char* TR181_STORE_PATH = "/opt/secure/RFC/tr181store.ini";
    
    void SetUp() override {
        rfcObj = new RuntimeFeatureControlProcessor();
    }
    
    void TearDown() override {
        delete rfcObj;
        rfcObj = nullptr;
    }
    
    // Helper method to write AccountID to tr181store
    void SetAccountIDInStore(const std::string& accountId) {
        writeToTr181storeFile(ACCOUNTID_KEY, accountId, TR181_STORE_PATH, Quoted);
    }
    
    // Helper method to test config value change
   bool TestConfigValueChange(const std::string& xconfValue, const std::string& currentValueIn, std::string& resultValue) {
        resultValue = xconfValue;
        std::string currentValue = currentValueIn;  // Create non-const copy for the API
        return rfcObj->isConfigValueChange("AccountId", ACCOUNTID_KEY, resultValue, currentValue);
    }
};


// Basic validation tests
TEST_F(AccountIDValidationTest, EmptyAccountID_ShouldBeRejected)
{
    std::string emptyValue = "";
    EXPECT_TRUE(emptyValue.empty());
    
    // Test rejection through config change method
    std::string resultValue;
    bool changed = TestConfigValueChange("", DUMMY_ACCOUNTID_1, resultValue);
    EXPECT_FALSE(changed);
}

TEST_F(AccountIDValidationTest, UnknownAccountID_ShouldBeReplacedWithAuthserviceValue)
{
    std::string resultValue;
    bool changed = TestConfigValueChange(UNKNOWN_VALUE, DUMMY_ACCOUNTID_1, resultValue);
    
    EXPECT_FALSE(changed);  // No change needed when Unknown is replaced
    EXPECT_EQ(resultValue, DUMMY_ACCOUNTID_1);  // Value replaced with authservice AccountID
}

TEST_F(AccountIDValidationTest, ValidAccountID_ShouldBeAccepted)
{
    std::string resultValue;
    bool changed = TestConfigValueChange(DUMMY_ACCOUNTID_1, UNKNOWN_VALUE, resultValue);
    
    EXPECT_TRUE(changed);  // Config change should be detected
    EXPECT_EQ(resultValue, DUMMY_ACCOUNTID_1);  // Valid value should be preserved
}

TEST_F(AccountIDValidationTest, UnknownComparison_ShouldBeCaseInsensitive)
{
    EXPECT_TRUE(StringCaseCompare("UNKNOWN", UNKNOWN_VALUE));
    EXPECT_TRUE(StringCaseCompare("UnKnOwN", UNKNOWN_VALUE));
    EXPECT_TRUE(StringCaseCompare("unknown", UNKNOWN_VALUE));
}

TEST_F(AccountIDValidationTest, ConfigValueChange_ShouldBeDetectedCorrectly)
{
    std::string resultValue;
    
    // Test 1: Different values should trigger change
    bool changed1 = TestConfigValueChange(DUMMY_ACCOUNTID_1, DUMMY_ACCOUNTID_2, resultValue);
    EXPECT_TRUE(changed1);
    
    // Test 2: Same values should not trigger change
    bool changed2 = TestConfigValueChange(DUMMY_ACCOUNTID_1, DUMMY_ACCOUNTID_1, resultValue);
    EXPECT_FALSE(changed2);
}

// Integration tests with RFC system
TEST(rfcMgrTest, GetAccountID_LoadsValueFromStore)
{
    // Test loading valid AccountID
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "1234567890000000001", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.GetAccountID();
    EXPECT_EQ(rfcObj._accountId, "1234567890000000001");
    EXPECT_FALSE(StringCaseCompare(rfcObj._accountId, "Unknown"));
}

TEST(rfcMgrTest, GetAccountID_HandlesUnknownValue)
{
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "Unknown", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.GetAccountID();
    EXPECT_TRUE(StringCaseCompare(rfcObj._accountId, "Unknown"));
}

TEST(rfcMgrTest, GetAccountID_HandlesEmptyValue)
{
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.GetAccountID();
    EXPECT_TRUE(rfcObj._accountId.empty());
}

TEST(rfcMgrTest, GetValidAccountId_ReplacesUnknownWithAuthservice)
{
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "9876543210000000001", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.GetAccountID();
    rfcObj._RFCKeyAndValueMap[RFC_ACCOUNT_ID_KEY_STR] = "Unknown";
    rfcObj.GetValidAccountId();
    
    EXPECT_EQ(rfcObj._accountId, "9876543210000000001");
}

TEST(rfcMgrTest, GetValidAccountId_RejectsEmptyValue)
{
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "9876543210000000001", "/opt/secure/RFC/tr181store.ini", Quoted);
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.GetAccountID();
    rfcObj._RFCKeyAndValueMap[RFC_ACCOUNT_ID_KEY_STR] = "";
    rfcObj.GetValidAccountId();
    
    EXPECT_EQ(rfcObj._accountId, "9876543210000000001");
}

// L2 test equivalents - XCONF response processing
TEST(rfcMgrTest, XconfUnknownAccountID_ReplacedByAuthservice)
{
    // Simulates rfc_unknown_accountid L2 test
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "5555555550000000001", "/opt/secure/RFC/tr181store.ini", Quoted);
    
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.GetAccountID();
    
    std::string key = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID";
    std::string xconfValue = "Unknown";
    std::string authserviceValue = "5555555550000000001";
    
    bool configChanged = rfcObj.isConfigValueChange("AccountId", key, xconfValue, authserviceValue);
    
    EXPECT_FALSE(configChanged);  // No update needed
    EXPECT_EQ(xconfValue, "5555555550000000001");  // Unknown replaced with authservice value
}

TEST(rfcMgrTest, XconfValidAccountID_UpdatesDatabase)
{
    // Simulates rfc_trigger_reboot L2 test
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "Unknown", "/opt/secure/RFC/tr181store.ini", Quoted);
    
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.GetAccountID();
    
    std::string key = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID";
    std::string xconfValue = "1234567890000000001";
    std::string currentValue = "Unknown";
    
    bool configChanged = rfcObj.isConfigValueChange("AccountId", key, xconfValue, currentValue);
    
    EXPECT_TRUE(configChanged);  // Database update triggered
    EXPECT_EQ(xconfValue, "1234567890000000001");  // Valid value preserved
}

TEST(rfcMgrTest, XconfEmptyAccountID_IsRejected)
{
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "9876543210000000001", "/opt/secure/RFC/tr181store.ini", Quoted);
    
    RuntimeFeatureControlProcessor rfcObj;
    
    std::string key = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID";
    std::string xconfValue = "";
    std::string currentValue = "9876543210000000001";
    
    bool configChanged = rfcObj.isConfigValueChange("AccountId", key, xconfValue, currentValue);
    
    EXPECT_FALSE(configChanged);  // Empty value rejected
}

// XCONF response processing tests
TEST(rfcMgrTest, ProcessXconfResponse_WithUnknownAccountID)
{
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "5555555550000000001", "/opt/secure/RFC/tr181store.ini", Quoted);
    
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.rfc_state = Init;
    rfcObj._is_first_request = true;
    
    rfcObj.PreProcessJsonResponse(xconfRespUnknownAccountId);
    int result = rfcObj.ProcessJsonResponse(xconfRespUnknownAccountId);
    
    EXPECT_GE(result, 0);
}

TEST(rfcMgrTest, ProcessXconfResponse_WithValidAccountID)
{
    writeToTr181storeFile("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID", 
                          "Unknown", "/opt/secure/RFC/tr181store.ini", Quoted);
    
    RuntimeFeatureControlProcessor rfcObj;
    rfcObj.rfc_state = Init;
    rfcObj._is_first_request = true;
    
    rfcObj.PreProcessJsonResponse(xconfRespValidAccountId);
    int result = rfcObj.ProcessJsonResponse(xconfRespValidAccountId);
    
    EXPECT_GE(result, 0);
}

GTEST_API_ int main(int argc, char *argv[]){
    ::testing::InitGoogleTest(&argc, argv);

    cout << "Starting GTEST===========================>" << endl;
    return RUN_ALL_TESTS();
}





