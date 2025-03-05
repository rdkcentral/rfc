/*##############################################################################
 # If not stated otherwise in this file or this component's LICENSE file the
 # following copyright and licenses apply:
 #
 # Copyright 2020 RDK Management
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 # http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 ##############################################################################
 */

#include "mtlsUtils.h"
#include "rfc_common.h"
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Description: Checking state red support is present or not
 * return :1 = if state red support is present
 * return :0 = if state red support is not present */
int isStateRedSupported(void) {
    int ret = -1;
    ret = filePresentCheck(STATE_RED_SPRT_FILE);
    if(ret == RDK_API_SUCCESS) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "isStateRedSupported(): Yes file present:%s\n", STATE_RED_SPRT_FILE);
        return 1;
    }
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "isStateRedSupported(): No:%s\n", STATE_RED_SPRT_FILE);
    return 0;
}

/* Description: Checking either device is in state red or not.
 * return 1: In state red
 *        0: Not in state red
 * */
int isInStateRed(void) {
    int ret = -1;
    int stateRed = 0;
    ret = isStateRedSupported();
    if(ret == 0) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"isInStateRed(): No ret:%d\n", stateRed);
        return stateRed;
    }
    ret = filePresentCheck(STATEREDFLAG);
    if(ret == RDK_API_SUCCESS) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "isInStateRed(): Yes Flag prsent:%s. Device is in statered\n", STATEREDFLAG);
        stateRed = 1;
    } else {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "isInStateRed(): No Flag Not prsent:%s. Device is not in statered\n", STATEREDFLAG);
    }
    return stateRed;
}

/* Description: Placeholder functions for implementing MTLS certs retrieval logic from compile time overrides
 * @param sec: This is a pointer hold the certificate, key and type of certificate.
 * @return : int Success 1 and failure -1
 * */
int getMtlscert(MtlsAuth_t *sec) 
{
    if (NULL == sec) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"getMtlscert(): the sec buffer is empty or null.");
        return MTLS_FAILURE;
    }

#if defined(RDKB_SUPPORT)
    // Broadband-specific MTLS certificate handling
    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "Using Broadband MTLS certificates");
    
    struct stat st;
    if (stat("/nvram/certs/devicecert_1.pk12", &st) == 0) {
        sec->certFile = strdup("/nvram/certs/devicecert_1.pk12");
        sec->keyFile = NULL; // PK12 contains key
        sec->c_cert = 1;     // PK12 format
        return MTLS_SUCCESS;
    }
#endif
    return MTLS_FAILURE;
}
#ifdef __cplusplus
}
#endif
