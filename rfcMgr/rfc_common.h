/**
 * @file rfc_common.h
 * @brief Common utilities, macros, and helpers for the RFC Manager.
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

#ifndef RFC_MGR_COMMON_H
#define RFC_MGR_COMMON_H

/** @addtogroup rfcMgr
 *  @{ */

#include "rdk_debug.h"
#include "rfcapi.h"
#include "rfc_mgr_key.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <list>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <secure_wrapper.h>

/** @name Macros
 *  @{ */
#define LOG_RFCMGR "LOG.RDK.RFCMGR" /**< RDK Logger module name. */

#define FAILURE            -1
#define SUCCESS             0
#define NO_RFC_UPDATE_REQUIRED 1

#define RFC_VALUE_BUF_SIZE  512
#define READ_RFC_SUCCESS    1
#define READ_RFC_FAILURE   -1
#define WRITE_RFC_SUCCESS   1
#define WRITE_RFC_FAILURE  -1
#define RETRY_COUNT         3
#define RFC_MAX_LEN         64

#define RFC_TMP_PATH "/tmp/RFC/tmp"
#define SECURE_RFC_PATH "/opt/secure/RFC"

#define DEFAULT_DL_ALLOC    1024

/** System command type selector for executeCommandAndGetOutput(). */
typedef enum {
    eRdkSsaCli,                          /**< Use rdkssacli binary. */
    eWpeFrameworkSecurityUtility         /**< Use WPEFrameworkSecurityUtility binary. */
} SYSCMD;

#define WPEFRAMEWORKSECURITYUTILITY     "/usr/bin/WPEFrameworkSecurityUtility"
#define RDKSSACLI_CMD                   "/usr/bin/rdkssacli %s"
#define KEY_GEN_BIN                     "/usr/bin/rdkssacli"

/** @} */ /* end Macros */

/**
 * @brief Execute a system binary and capture its stdout.
 * @param[in]  eSysCmd   Which binary to invoke.
 * @param[in]  pArgs     Arguments passed to the binary.
 * @param[out] result    Captured standard output.
 * @return SUCCESS (0) or FAILURE (-1).
 */
int executeCommandAndGetOutput(SYSCMD, const char *, std::string&);

/**
 * @brief Read an RFC parameter from the data store.
 * @param[in]  type       Caller ID / namespace (may be NULL on RDKB/RDKC).
 * @param[in]  key        TR181 parameter name.
 * @param[out] out_value  Buffer to receive the value.
 * @param[in]  datasize   Size of @p out_value buffer.
 * @return READ_RFC_SUCCESS (1) or READ_RFC_FAILURE (-1).
 */
int read_RFCProperty(const char* , const char* , char *, int );

/**
 * @brief Return true if @p str contains characters outside [a-zA-Z0-9._-].
 */
bool CheckSpecialCharacters(const std::string&);

/**
 * @brief Case-insensitive string comparison.
 */
bool StringCaseCompare(const std::string& , const std::string& );

/**
 * @brief Erase every occurrence of @p toRemove from @p str in-place.
 */
void RemoveSubstring(std::string& str, const std::string& toRemove);

/**
 * @brief Block until webconfig RFC blob processing completes (RDKB).
 */
void waitForRfcCompletion();

/**
 * @brief Retrieve the eRouter WAN IP address (IPv6 preferred).
 */
std::string getErouterIPAddress();

/**
 * @brief Query a sysevent key via the CLI.
 * @param[in] key  Sysevent key name.
 * @return Value string, or empty on failure.
 */
std::string getSyseventValue(const std::string& key);

/**
 * @brief Parse the cron schedule from DCM settings.
 * @return Five-field cron string.
 */
std::string getCronFromDCMSettings();

/** @} */ /* end rfcMgr group */

#endif
