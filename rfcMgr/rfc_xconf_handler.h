/**
 * @file rfc_xconf_handler.h
 * @brief RuntimeFeatureControlProcessor — Xconf query, JSON parsing, and RFC application.
 *
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
 */

#ifndef RFC_XCONF_HANDLER_H
#define RFC_XCONF_HANDLER_H

#include "xconf_handler.h"
#include "rfc_common.h"
#include "rfc_mgr_key.h"
#include "rfc_mgr_json.h"

#ifdef RDKC
#include <set>
#endif

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

#ifdef __cplusplus
}
#endif

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
#define RFC_DEVICETYPE                     "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Identity.DeviceType"

#define RFC_VIDEO_CONTROL_ID               2504
#define RFC_VIDEO_VOD_ID                   15660
#define RFC_CHANNEL_MAP_ID                 2345
#define RETRY_DELAY                        10

#define RFC_SYNC_DONE                      "/tmp/.rfcSyncDone"

/**
 * @brief RFC state-machine phases.
 */
typedef enum
{
   Invalid,             /**< Not initialised. */
   Init,                /**< Initial request. */
   Local,               /**< Local override. */
   Redo,                /**< Re-attempt needed. */
   Redo_With_Valid_Data, /**< Re-attempt with updated data. */
   Finish               /**< Processing complete. */
} RfcState;

#if defined(RDKB_SUPPORT) || defined(RDKC)
/**
 * @brief Minimal WDMP status codes for RDKB / RDKC builds.
 */
typedef enum {
    WDMP_SUCCESS = 0,  /**< Operation succeeded. */
    WDMP_FAILURE,      /**< Operation failed. */
} WDMP_STATUS;
#endif

/**
 * @class RuntimeFeatureControlProcessor
 * @brief Queries Xconf, parses feature JSON, and applies RFC parameters.
 *
 * Inherits device-identity primitives from XconfHandler.  On RDKC builds,
 * virtual methods allow the RdkcRuntimeFeatureControlProcessor subclass
 * to override URL building, hash/time storage, and endpoint metadata.
 *
 * Non-copyable.
 */
class RuntimeFeatureControlProcessor : public xconf::XconfHandler
{
        public :

        /** @brief Construct with default state (reboot not required). */
        RuntimeFeatureControlProcessor() 
        {
            isRebootRequired = false;
        }
#ifdef RDKC
        virtual ~RuntimeFeatureControlProcessor() = default;
#else
        ~RuntimeFeatureControlProcessor() = default;
#endif
        RuntimeFeatureControlProcessor(const RuntimeFeatureControlProcessor&) = delete; /**< Copy disabled. */
        
        /**
         * @brief Initialise the RFC processor (load device identity, server URL, etc.).
         * @return SUCCESS (0) or FAILURE (-1).
         */
        int  InitializeRuntimeFeatureControlProcessor(void);

        /**
         * @brief Execute the full Xconf feature-control request/response cycle.
         * @return SUCCESS (0) or FAILURE (-1).
         */
        int ProcessRuntimeFeatureControlReq();

        /**
         * @brief Check whether an Xconf response flagged a required reboot.
         * @retval true  Reboot needed.
         * @retval false No reboot.
         */
        bool getRebootRequirement();
#ifdef RDKC
        /** @brief On RDKC, query if the reboot cron job should be scheduled. */
        bool getRfcRebootCronNeeded() const { return _rfcRebootCronNeeded; }
#endif
        /** @brief Send a telemetry-2 count marker. */
        void NotifyTelemetry2Count(std ::string markerName);
        /** @brief Send a telemetry-2 key/value marker. */
        void NotifyTelemetry2Value(std ::string markerName, std ::string value);

#ifdef RDKC
    protected:
#else
    private:
#endif
        /* ---------------------------------------------------------------
         * Members and methods accessible to platform-specific subclasses.
         * Subclasses (e.g. RdkcRuntimeFeatureControlProcessor) override
         * the virtual methods below to handle device-specific behaviour
         * without duplicating the shared RFC request/response logic.
         * --------------------------------------------------------------- */

        std::string _accountId;        /**< Device Account ID. */
        std::string _experience;        /**< Device experience string. */
        std::string _osclass;           /**< OS class identifier. */
        std::string _xconf_server_url;  /**< Active Xconf server URL. */
        std::string rfcSelectOpt;       /**< Xconf selector option. */
        std::string bkup_hash;          /**< Backup configSetHash. */
#ifdef RDKC
        bool _rfcRebootCronNeeded = false;                 /**< RDKC: schedule reboot cron. */
        std::set<std::string> _effectiveImmediateParams;   /**< RDKC: params with effectiveImmediate=true. */
#endif

        /** Build the full HTTP query URL sent to the Xconf server.
         *  Override to change device-specific query parameters. */
#ifdef RDKC
        virtual std::stringstream CreateXconfHTTPUrl();
#else
        std::stringstream CreateXconfHTTPUrl();
#endif

        /** Read configSetHash / configSetTime from the backing store.
         *  Override to use a different storage medium (e.g. RAM files). */
#ifdef RDKC
        virtual void RetrieveHashAndTimeFromPreviousDataSet(std::string &valueHash,
                                                            std::string &valueTime);
#else
        void RetrieveHashAndTimeFromPreviousDataSet(std::string &valueHash,
                                                    std::string &valueTime);
#endif

        /** Persist XconfSelector and XconfUrl after a successful sync.
         *  Override as a no-op for platforms that must not write these. */
#ifdef RDKC
        virtual void StoreXconfEndpointMetadata();
#else
        void StoreXconfEndpointMetadata();
#endif

       private:

        /** @brief Per-feature RFC object parsed from Xconf JSON. */
        typedef struct RuntimeFeatureControlObject {
               std::string name;              /**< Feature name. */
               std::string featureInstance;    /**< Feature instance ID. */
               bool enable;                    /**< Feature enable flag. */
               bool effectiveImmediate;        /**< Apply without reboot. */
        }RuntimeFeatureControlObject;
		
	std::map<std::string, std::string> _RFCKeyAndValueMap; /**< Parsed config key-value map. */
	RfcState     rfc_state;              /**< Current RFC state-machine phase. */
	std::string _last_firmware;          /**< Last firmware version processed. */
        std::string _boot_strap_xconf_url;   /**< Bootstrap Xconf URL. */
	std::string _valid_accountId;        /**< Validated account ID. */
	std::string _valid_partnerId;        /**< Validated partner ID. */
        std::string stashAccountId;          /**< Stashed account ID for comparison. */
        std::string _partnerId;              /**< Device partner ID. */
        std::string _bkPartnerId;            /**< Backup partner ID. */
        std::string _accountMgmt;            /**< Account management flag. */
        std::string _serialNumber;           /**< Device serial number. */
        std::string _extendermacAddress;     /**< Extender MAC address. */
        bool isRebootRequired;               /**< True if Xconf requested reboot. */
        bool _is_first_request = false;      /**< First request after FW upgrade. */
        bool _url_validation_in_progress = false; /**< URL validation in flight. */
        std:: string rfcSelectorSlot;        /**< Xconf selector slot (prod/ci). */
		 
        bool checkWhoamiSupport();
        bool IsNewFirmwareFirstRequest(void);
        int GetLastProcessedFirmware(const char *lastVesrionFile);
        void GetAccountID();
        void GetRFCPartnerID();
        bool isMaintenanceEnabled();
        void GetOsClass( void );
        void GetSerialNumber( void );
        void GetExtenderMacAddress( void );
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


        void GetStoredHashAndTime( std ::string &valueHash, std::string &valueTime ); 
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
    void getDeviceTypeRFC(char *deviceType, size_t size);
    bool isSecureDbgSrvUnlocked(void);

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
    FRIEND_TEST(rfcMgrTest, isSecureDbgSrvUnlocked_dev);
    FRIEND_TEST(rfcMgrTest, isSecureDbgSrvUnlocked_labsigned_true);
    FRIEND_TEST(rfcMgrTest, isSecureDbgSrvUnlocked_prod);
    FRIEND_TEST(rfcMgrTest, isSecureDbgSrvUnlocked_dType_prod);
    FRIEND_TEST(rfcMgrTest, isSecureDbgSrvUnlocked_labsigned_DbgSrv_false);
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
    FRIEND_TEST(rfcMgrTest, GetAccountID_LoadsValueFromStore);
    FRIEND_TEST(rfcMgrTest, GetAccountID_HandlesUnknownValue);
    FRIEND_TEST(rfcMgrTest, GetAccountID_HandlesEmptyValue);
    FRIEND_TEST(rfcMgrTest, GetValidAccountId_ReplacesUnknownWithAuthservice);
    FRIEND_TEST(rfcMgrTest, GetValidAccountId_RejectsEmptyValue);
    FRIEND_TEST(rfcMgrTest, XconfUnknownAccountID_ReplacedByAuthservice);
    FRIEND_TEST(rfcMgrTest, XconfValidAccountID_UpdatesDatabase);
    FRIEND_TEST(rfcMgrTest, XconfEmptyAccountID_IsRejected);
    FRIEND_TEST(rfcMgrTest, ProcessXconfResponse_WithUnknownAccountID);
    FRIEND_TEST(rfcMgrTest, ProcessXconfResponse_WithValidAccountID);
    FRIEND_TEST(rfcMgrTest, preProcessJsonResponse_rfcstate);
    FRIEND_TEST(rfcMgrTest, updateHashAndTimeInDB);
    FRIEND_TEST(rfcMgrTest, lowerconfigSetHash);
    FRIEND_TEST(rfcMgrTest, noconfigSetHash);
    FRIEND_TEST(rfcMgrTest, GetServURLFileNotFound);
    FRIEND_TEST(rfcMgrTest, ValidPartnerId);
    FRIEND_TEST(rfcMgrTest, Removed_PERSISTENCE_FILE);
    FRIEND_TEST(rfcMgrTest, EmptyFeatures);

#endif
};

#endif
