/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2023 RDK Management
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
**/

#ifndef RFC_XCONF_HANDLER_H
#define RFC_XCONF_HANDLER_H

#include "xconf_handler.h"
#include "rfc_common.h"
#include "rfc_mgr_key.h"
#include "rfc_mgr_json.h"

#if defined(GTEST_ENABLE)
#include <gtest/gtest.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "urlHelper.h"
#include <json_parse.h>
#include <rdk_fwdl_utils.h>
#include <downloadUtil.h>

#ifndef GTEST_ENABLE
#define BOOTSTRAP_FILE          "/opt/secure/RFC/bootstrap.ini"
#define PARTNER_ID_FILE         "/opt/www/authService/partnerId3.dat"
#define DEVICE_PROPERTIES_FILE  "/etc/device.properties"
#else
#define BOOTSTRAP_FILE          "/tmp/bootstrap.ini"
#define PARTNER_ID_FILE         "/tmp/partnerId3.dat"
#define DEVICE_PROPERTIES_FILE  "/tmp/device.properties"
#endif

#define RFC_PROPERTIES_FILE                "/etc/rfc.properties"
#if defined(RDKB_SUPPORT)
#define RFC_PROPERTIES_PERSISTENCE_FILE    "/nvram/rfc.properties"
#define RDKB_RETRY_DELAY                   30
#else	
#define RFC_PROPERTIES_PERSISTENCE_FILE    "/opt/rfc.properties"
#endif		
#define WPEFRAMEWORKSECURITYUTILITY        "/usr/bin/WPEFrameworkSecurityUtility"
#define DIRECTORY_PATH                     "/opt/secure/RFC/"
#define VARFILE                            "rfcVariable.ini"
#define FEATURE_FILE_LIST                  "rfcFeature.list"
#define TR181_FILE_LIST                    "tr181.list"
#define BS_STORE_FILENAME                  "/opt/secure/RFC/bootstrap.ini"
#define RFC_LAST_VERSION                   "/opt/secure/RFC/.version"
#define VARIABLEFILE                       "/opt/secure/RFC/rfcVariable.ini"
#define TR181LISTFILE                      "/opt/secure/RFC/tr181.list"
#define TR181STOREFILE                      "/opt/secure/RFC/tr181store.ini" 
#define DIRECT_BLOCK_FILENAME              "/tmp/.lastdirectfail_rfc"
#define RFC_DEBUGSRV                       "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Identity.DbgServices.Enable"

#define RFC_VIDEO_CONTROL_ID               2504
#define RFC_VIDEO_VOD_ID                   15660
#define RFC_CHANNEL_MAP_ID                 2345
#define RETRY_DELAY                        10

#define RFC_SYNC_DONE                      "/tmp/.rfcSyncDone"

typedef enum
{
   Invalid, 
   Init,
   Local,
   Redo,
   Redo_With_Valid_Data,
   Finish
} RfcState;

#if defined(RDKB_SUPPORT)
typedef enum {
    WDMP_SUCCESS = 0,
    WDMP_FAILURE,
} WDMP_STATUS;
#endif

class RuntimeFeatureControlProcessor : public xconf::XconfHandler
{
        public :

        RuntimeFeatureControlProcessor() 
        {
            isRebootRequired = false;
        }
        // We do not allow this class to be copied !!
        RuntimeFeatureControlProcessor(const RuntimeFeatureControlProcessor&) = delete;
        
        int  InitializeRuntimeFeatureControlProcessor(void);
        int ProcessRuntimeFeatureControlReq();
        bool getRebootRequirement();
        void NotifyTelemetry2Count(std ::string markerName);
        void NotifyTelemetry2Value(std ::string markerName, std ::string value);

	private:

        typedef struct RuntimeFeatureControlObject {
               std::string name;
               std::string featureInstance;
               bool enable;
               bool effectiveImmediate;
        }RuntimeFeatureControlObject;
		
	std::map<std::string, std::string> _RFCKeyAndValueMap;
	RfcState     rfc_state; /* RFC State */
	std::string _last_firmware; /* Last Firmware Version */
	std::string _xconf_server_url; /* Xconf server URL */
        std::string _boot_strap_xconf_url; /* Bootstrap XConf URL */
	std::string _valid_accountId; /* Valid Account ID*/
	std::string _valid_partnerId; /* Valid Partner ID*/
	std::string _accountId; /* Device Account ID */
        std::string stashAccountId;
        std::string _partnerId; /* Device Partner ID */
        std::string _bkPartnerId; /* Device Partner ID */
        std::string _experience;
        std::string _osclass;
        bool isRebootRequired;
	std::string bkup_hash;
        bool _is_first_request = false;
        bool _url_validation_in_progress = false;
        std::string  rfcSelectOpt;
        std:: string rfcSelectorSlot;
		 
        bool checkWhoamiSupport();
        bool IsNewFirmwareFirstRequest(void);
        int GetLastProcessedFirmware(const char *lastVesrionFile);
        void GetAccountID();
        void GetRFCPartnerID();
        bool isMaintenanceEnabled();
        void GetOsClass( void );
        int GetExperience( void );
        int GetServURL(const char *rfcPropertiesFile);
        int GetBootstrapXconfUrl(std ::string &XconfUrl); 
        bool checkBootstrap(const std::string& filename, const std::string& target);
        
        int getEffectiveImmediateParam(JSON *feature, RuntimeFeatureControlObject *rfcObj);
        int getRFCEnableParam(JSON *feature, RuntimeFeatureControlObject *rfcObj);
        int getFeatureInstance(JSON *feature, RuntimeFeatureControlObject *rfcObj);
        int getRFCName(JSON *feature, RuntimeFeatureControlObject *rfcObj);
        bool isXconfSelectorSlotProd();
        //void rfcSetHashAndTime(const char *Time, const char *Hash);
        void updateHashInDB(std::string configSetHash);
        void updateTimeInDB(std::string timestampString);
        void updateHashAndTimeInDB(char *curlHeaderResp);
        bool IsDirectBlocked();
        void clearDB();
        void clearDBEnd();	
        void rfcStashStoreParams(void);
	void rfcStashRetrieveParams(void);


        std::stringstream CreateXconfHTTPUrl(); 
        void GetStoredHashAndTime( std ::string &valueHash, std::string &valueTime ); 
        void RetrieveHashAndTimeFromPreviousDataSet(std ::string &valueHash, std::string &valueTime); 
        void InitDownloadData(DownloadData *pDwnData);
        int DownloadRuntimeFeatutres(DownloadData *pDwnLoc, DownloadData *pHeaderDwnLoc, const std::string& url_str); 
        void NotifyTelemetry2ErrorCode(int CurlReturn);
        void PreProcessJsonResponse(char *xconfResp);
#if defined(RDKB_SUPPORT)
        bool ExecuteCommand(const std::string& command, std::string& output);
	bool ParseConfigValue(const std::string& configKey, const std::string& configValue, int rebootValue, bool& rfcRebootCronNeeded);
        int ProcessJsonResponseB(char* featureXConfMsg);
        void saveAccountIdToFile(const std::string& accountId, const std::string& paramName, const std::string& paramType);
        std::string readAccountIdFromFile();
        void rfcCheckAccountId();
        void HandleScheduledReboot(bool rfcRebootCronNeeded);
#endif	
        void GetValidAccountId();
        void GetValidPartnerId();
        void GetXconfSelect();
        int ProcessJsonResponse(char *featureXConfMsg);
        JSON* GetRuntimeFeatureControlJSON(JSON *);
        void processXconfResponseConfigDataPart(JSON *features);
        void CreateConfigDataValueMap(JSON *features);
        bool isConfigValueChange(std ::string name, std ::string key, std ::string &value, std ::string &paramValue);
        WDMP_STATUS set_RFCProperty(std ::string name, std ::string key, std ::string value);
        void updateTR181File(const std::string& filename, std::list<std::string>& paramList); 
	void NotifyTelemetry2RemoteFeatures(const char *rfcFeatureList, std ::string rfcstatus);
        void WriteFile(const std::string& filename, const std::string& data); 
        void writeRemoteFeatureCntrlFile(const std::string& filename, RuntimeFeatureControlObject *feature);
	int getJsonRpc(char *, DownloadData* );
	int getJRPCTokenData( char *, char *, unsigned int );
	void cleanAllFile();
        int ProcessXconfUrl(const char *XconfUrl);
	bool isDebugServicesEnabled(void);

#if defined(GTEST_ENABLE)
    FRIEND_TEST(rfcMgrTest, isNewFirmwareFirstRequest);
    FRIEND_TEST(rfcMgrTest, getLastProcessedFirmware);
    FRIEND_TEST(rfcMgrTest, getAccountID);
    FRIEND_TEST(rfcMgrTest, getPartnerID);
    FRIEND_TEST(rfcMgrTest, getExperience);
    FRIEND_TEST(rfcMgrTest, getServURL);
    FRIEND_TEST(rfcMgrTest, getBootstrapXconfUrl);
    FRIEND_TEST(rfcMgrTest, checkBootstrap);
    FRIEND_TEST(rfcMgrTest, isXconfSelectorSlotProd);
    FRIEND_TEST(rfcMgrTest, getStoredHashAndTime);
    FRIEND_TEST(rfcMgrTest, retrieveHashAndTimeFromPreviousDataSet);
    FRIEND_TEST(rfcMgrTest, preProcessJsonResponse);
    FRIEND_TEST(rfcMgrTest, processJsonResponse);
    FRIEND_TEST(rfcMgrTest, getRuntimeFeatureControlJSON);
    FRIEND_TEST(rfcMgrTest, notifyTelemetry2RemoteFeatures);
    FRIEND_TEST(rfcMgrTest, writeFile);
    FRIEND_TEST(rfcMgrTest, writeRemoteFeatureCntrlFile);
    FRIEND_TEST(rfcMgrTest, getJsonRpc);
    FRIEND_TEST(rfcMgrTest, cleanAllFile);
    FRIEND_TEST(rfcMgrTest, checkWhoamiSupport);
    FRIEND_TEST(rfcMgrTest, isDebugServicesEnabled);
    FRIEND_TEST(rfcMgrTest, isMaintenanceEnabled);
    FRIEND_TEST(rfcMgrTest, GetOsClass);
    FRIEND_TEST(rfcMgrTest, set_RFCProperty);
    FRIEND_TEST(rfcMgrTest, GetValidPartnerId);
    FRIEND_TEST(rfcMgrTest, GetValidAccountId);
    FRIEND_TEST(rfcMgrTest, CreateXconfHTTPUrl);
    FRIEND_TEST(rfcMgrTest, isConfigValueChange);
    FRIEND_TEST(rfcMgrTest, IsDirectBlocked);
    FRIEND_TEST(rfcMgrTest, CreateConfigDataValueMap);
    FRIEND_TEST(rfcMgrTest, GetRuntimeFeatureControlJSON);
    FRIEND_TEST(rfcMgrTest, InitDownloadData);
    FRIEND_TEST(rfcMgrTest, getRFCName);
    FRIEND_TEST(rfcMgrTest, getFeatureInstance);
    FRIEND_TEST(rfcMgrTest, getRFCEnableParam);
    FRIEND_TEST(rfcMgrTest, getEffectiveImmediateParam);
    FRIEND_TEST(rfcMgrTest, GetXconfSelect);
    FRIEND_TEST(rfcMgrTest, updateHashInDB);
    FRIEND_TEST(rfcMgrTest, updateTimeInDB);
    FRIEND_TEST(rfcMgrTest, getJRPCTokenData);
    FRIEND_TEST(rfcMgrTest, ProcessXconfUrl);
    FRIEND_TEST(rfcMgrTest, updateTR181File);
    FRIEND_TEST(rfcMgrTest, processXconfResponseConfigDataPart);
    FRIEND_TEST(rfcMgrTest, rfcStashStoreParams);
    FRIEND_TEST(rfcMgrTest, rfcStashRetrieveParams);
    FRIEND_TEST(rfcMgrTest, clearDBEnd);
    FRIEND_TEST(rfcMgrTest, clearDB);
    FRIEND_TEST(rfcMgrTest, getAccountID_Unknown);
    FRIEND_TEST(rfcMgrTest, MAX_RETRIES);
    FRIEND_TEST(rfcMgrTest, GetRFCPartnerID);
    FRIEND_TEST(rfcMgrTest, GetOsClass_Disabled);
    FRIEND_TEST(rfcMgrTest, InvalidAccountId);
    FRIEND_TEST(rfcMgrTest, LastFirmware_FileRemoved);
    FRIEND_TEST(rfcMgrTest, GetServURL_FileRemoved);
    FRIEND_TEST(rfcMgrTest, isXconfSelectorSlotProd_Disabled);
    FRIEND_TEST(rfcMgrTest, isDebugServicesDisable);
    FRIEND_TEST(rfcMgrTest, WHOAMI_SUPPORT_Disabled);
    FRIEND_TEST(rfcMgrTest, isMaintenanceDisabled);
    FRIEND_TEST(rfcMgrTest, GetServURLEmpty);
    FRIEND_TEST(rfcMgrTest, NotifyTelemetry2ErrorCode);
    FRIEND_TEST(rfcMgrTest, unknownAccountId);
    FRIEND_TEST(rfcMgrTest, GetXconfSelect_ci);
    FRIEND_TEST(rfcMgrTest, GetXconfSelect_Finish);
    FRIEND_TEST(rfcMgrTest, getRFCName_Invalid);
    FRIEND_TEST(rfcMgrTest, getRFCEnableParam_Invalid);
    FRIEND_TEST(rfcMgrTest, getEffectiveImmediateParam_Invalid);
    FRIEND_TEST(rfcMgrTest, get0verrideHashAndTime);
    FRIEND_TEST(rfcMgrTest, getUpgradeHashAndTime);
    FRIEND_TEST(rfcMgrTest, GetServURLFileNotFound);
    FRIEND_TEST(rfcMgrTest, processJsonResponseParseError);
    FRIEND_TEST(rfcMgrTest, getJRPCTokenData_Nulltoken);
    FRIEND_TEST(rfcMgrTest, notifyTelemetry2RemoteFeatures_RemovedFile);
    FRIEND_TEST(rfcMgrTest, notifyTelemetry2RemoteFeatures_EmptyFile);
    FRIEND_TEST(rfcMgrTest, ConfigDataNotFound);
    FRIEND_TEST(rfcMgrTest, features_NotFound);
    FRIEND_TEST(rfcMgrTest, AccountId_SpecialChars);
    FRIEND_TEST(rfcMgrTest, preProcessJsonResponse_rfcstate);
    FRIEND_TEST(rfcMgrTest, updateHashAndTimeInDB);
    FRIEND_TEST(rfcMgrTest, GetServURLFileNotFound);
    FRIEND_TEST(rfcMgrTest, ValidPartnerId);
    FRIEND_TEST(rfcMgrTest, Removed_PERSISTENCE_FILE);
    FRIEND_TEST(rfcMgrTest, EmptyFeatures);

#endif
};

#ifdef __cplusplus
}
#endif
#endif
