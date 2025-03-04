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

#ifndef RFC_MGR_COMMON_H
#define RFC_MGR_COMMON_H

/*----------------------------------------------------------------------------*/
/*                                   Header File                              */
/*----------------------------------------------------------------------------*/
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
#include <filesystem>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <secure_wrapper.h>
/*----------------------------------------------------------------------------*/
/*                                   Macros                                   */
/*----------------------------------------------------------------------------*/
#define LOG_RFCMGR "LOG.RDK.RFCMGR"

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

#define DEFAULT_DL_ALLOC    1024

typedef enum {
    eRdkSsaCli,
    eWpeFrameworkSecurityUtility
} SYSCMD;

#define WPEFRAMEWORKSECURITYUTILITY     "/usr/bin/WPEFrameworkSecurityUtility"
#define RDKSSACLI_CMD                   "/usr/bin/rdkssacli %s"
#define KEY_GEN_BIN                     "/usr/bin/rdkssacli"

int executeCommandAndGetOutput(SYSCMD, const char *, std::string&);
int read_RFCProperty(const char* , const char* , char *, int );
bool CheckSpecialCharacters(const std::string&);
bool StringCaseCompare(const std::string& , const std::string& );
void RemoveSubstring(std::string& str, const std::string& toRemove);

#endif
