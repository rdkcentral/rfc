/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
 *
 * rdkc_rfc_xconf_handler.cpp
 *
 * RDKC-specific overrides of RuntimeFeatureControlProcessor.
 * See rdkc_rfc_xconf_handler.h for the design rationale.
 **/

#ifdef RDKC

#include "rdkc_rfc_xconf_handler.h"
#include "rfc_common.h"   /* read_RFCProperty, RFC_VALUE_BUF_SIZE, LOG_RFCMGR */
#include "rdk_debug.h"
#include <fstream>
#include <unistd.h>

/* -------------------------------------------------------------------------
 * Constants
 * ---------------------------------------------------------------------- */

/** TR181 parameter holding the MD5 account hash on camera devices. */
#define RFC_MD5_ACCOUNT_HASH \
    "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.MD5AccountHash"

/** RAM-backed directory used for transient RFC state on all platforms. */
#define RFC_RAM_PATH "/tmp/RFC"

/* =========================================================================
 * GetAccountHash — lazy fetch, cached in _accountHash
 * ====================================================================== */

/**
 * Read the MD5 account hash from the RFC parameter store.
 * Called once (lazily) from CreateXconfHTTPUrl() before building the URL.
 *
 * Shell equivalent (XHC1 branch in RFCbase.sh):
 *   . $RDK_PATH/getAccountHash.sh
 *   JSONSTR='...&accountHash='$(getAccountHash)'&...'
 */
void RdkcRuntimeFeatureControlProcessor::GetAccountHash()
{
    if (!_accountHash.empty())
        return; /* already populated */

    /* RDKC: read account hash from /opt/usr_config/accounthash.txt.
     * Shell equivalent: . $RDK_PATH/getAccountHash.sh → getAccountHash() */
    std::ifstream ifs("/opt/usr_config/accounthash.txt");
    if (ifs.is_open())
    {
        std::getline(ifs, _accountHash);
        ifs.close();
        if (!_accountHash.empty())
        {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,
                    "[%s][%d] RDKC AccountHash: %s\n",
                    __FUNCTION__, __LINE__, _accountHash.c_str());
            return;
        }
    }

    RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR,
            "[%s][%d] RDKC: /opt/usr_config/accounthash.txt not found or empty\n",
            __FUNCTION__, __LINE__);
}

/* =========================================================================
 * CreateXconfHTTPUrl — camera-specific query parameters
 * ====================================================================== */

/**
 * Build the Xconf HTTP request URL for camera (XHC1) devices.
 *
 * Differences from the base implementation:
 *   - Adds   : accountHash  (camera-specific identity field)
 *   - Omits  : manufacturer, ecmMacAddress, osClass, controllerId,
 *              channelMapId, vodId  (not applicable on camera)
 *
 * Shell equivalent (XHC1 branch in sendHttpRequestToServer):
 *   JSONSTR='estbMacAddress='$(getEstbMacAddress)
 *           '&firmwareVersion='$(getFWVersion)
 *           '&env='$(getBuildType)
 *           '&model='$(getModel)
 *           '&accountHash='$(getAccountHash)
 *           '&partnerId='$(getPartnerId)
 *           '&accountId='$(getAccountId)
 *           '&experience='$(getExperience)
 *           '&version=2'
 */
std::stringstream RdkcRuntimeFeatureControlProcessor::CreateXconfHTTPUrl()
{
    GetAccountHash();

    std::stringstream url;
    url << _xconf_server_url       << "?";
    url << "estbMacAddress="       << _estb_mac_address << "&";
    url << "firmwareVersion="      << _firmware_version << "&";
    url << "env="                  << _build_type_str   << "&";
    url << "model="                << _model_number     << "&";
    url << "accountHash="          << _accountHash      << "&";
    url << "partnerId="            << _partner_id       << "&";
    url << "accountId="            << _accountId        << "&";
    url << "experience="           << _experience       << "&";
    url << "version=2";

    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,
            "[%s][%d] RDKC Xconf request URL built\n",
            __FUNCTION__, __LINE__);

    return url;
}

/* =========================================================================
 * RetrieveHashAndTimeFromPreviousDataSet — RAM files only
 * ====================================================================== */

/**
 * Read configSetHash and configSetTime from RAM-backed files.
 *
 * RDKC devices (XHC1) never use the TR181 parameter database for
 * hash/time storage.  Both values are always written to and read from
 * /tmp/RFC/.hashValue and /tmp/RFC/.timeValue.
 *
 * Shell equivalent (rfcGetHashAndTime, XHC1 branch):
 *   if [ "$DEVICE_TYPE" = "XHC1" ]; then
 *       valueHash=`cat $RFC_RAM_PATH/.hashValue`
 *       valueTime=`cat $RFC_RAM_PATH/.timeValue`
 *   fi
 */
void RdkcRuntimeFeatureControlProcessor::RetrieveHashAndTimeFromPreviousDataSet(
    std::string &valueHash, std::string &valueTime)
{
    valueHash = "UPGRADE_HASH";
    valueTime = "0";

    const std::string hashFile = std::string(RFC_RAM_PATH) + "/.hashValue";
    if (access(hashFile.c_str(), R_OK) == 0)
    {
        std::ifstream ifs(hashFile);
        if (ifs.is_open())
        {
            std::getline(ifs, valueHash);
            ifs.close();
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,
                    "[%s][%d] RDKC ConfigSetHash: %s\n",
                    __FUNCTION__, __LINE__, valueHash.c_str());
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,
                "[%s][%d] RDKC: hash file not found, using default\n",
                __FUNCTION__, __LINE__);
    }

    const std::string timeFile = std::string(RFC_RAM_PATH) + "/.timeValue";
    if (access(timeFile.c_str(), R_OK) == 0)
    {
        std::ifstream ifs(timeFile);
        if (ifs.is_open())
        {
            std::getline(ifs, valueTime);
            ifs.close();
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,
                    "[%s][%d] RDKC ConfigSetTime: %s\n",
                    __FUNCTION__, __LINE__, valueTime.c_str());
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,
                "[%s][%d] RDKC: time file not found, using default\n",
                __FUNCTION__, __LINE__);
    }

    bkup_hash = valueHash; /* keep backup consistent */
}

/* =========================================================================
 * StoreXconfEndpointMetadata — no-op for camera
 * ====================================================================== */

/**
 * RDKC devices (XHC1) must NOT write XconfSelector or XconfUrl back
 * to the parameter store after a successful Xconf sync.
 *
 * Shell equivalent (processJsonResponseV / sendHttpRequestToServer):
 *   if [ "$DEVICE_TYPE" != "XHC1" ]; then
 *       rfcSet ${XCONF_SELECTOR_TR181_NAME} string "$rfcSelectOpt"
 *       rfcSet ${XCONF_URL_TR181_NAME}     string "$rfcSelectUrl"
 *   fi
 */
void RdkcRuntimeFeatureControlProcessor::StoreXconfEndpointMetadata()
{
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,
            "[%s][%d] RDKC (XHC1): skipping XconfSelector/XconfUrl storage\n",
            __FUNCTION__, __LINE__);
    /* Intentional no-op */
}

#endif /* RDKC */
