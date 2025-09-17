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
#ifdef LIBRDKCONFIG_BUILD
#include "rdkconfig.h"
#endif

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
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "isInStateRed(): Yes Flag prsent:%s. Device is in statered\n", STATEREDFLAG);
        stateRed = 1;
    } else {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "isInStateRed(): No Flag Not prsent:%s. Device is not in statered\n", STATEREDFLAG);
    }
    return stateRed;
}

#ifdef LIBRDKCERTSELECTOR
/* Description: Use for get all mtls related certificate and key.
 * @param sec: This is a pointer hold the certificate, key and type of certificate.
 * @return :  MTLS_CERT_FETCH_SUCCESS on success, MTLS_CERT_FETCH_FAILURE on mtls cert failure , STATE_RED_CERT_FETCH_FAILURE on state red cert failure
 * */
MtlsAuthStatus getMtlscert(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel) {
    /*
            strncpy(sec->cert_name, STATE_RED_CERT, sizeof(sec->cert_name) - 1);
	    sec->cert_name[sizeof(sec->cert_name) - 1] = '\0';
            strncpy(sec->cert_type, "P12", sizeof(sec->cert_type) - 1);
	    sec->cert_type[sizeof(sec->cert_type) - 1] = '\0';
            strncpy(sec->key_pas, mtlsbuff, sizeof(sec->key_pas) - 1);
            sec->key_pas[sizeof(sec->key_pas) - 1] = '\0';
     * */
    (void)sec;
    (void)pthisCertSel;	
    return MTLS_CERT_FETCH_SUCCESS;
}
#else
/* Description: Use for get all mtls related certificate and key.
 * @param sec: This is a pointer hold the certificate, key and type of certificate.
 * @return : int Success 1 and failure -1
 * */
int getMtlscert(MtlsAuth_t *sec) {
    /*
            strncpy(sec->cert_name, STATE_RED_CERT, sizeof(sec->cert_name) - 1);
            sec->cert_name[sizeof(sec->cert_name) - 1] = '\0';
            strncpy(sec->cert_type, "P12", sizeof(sec->cert_type) - 1);
            sec->cert_type[sizeof(sec->cert_type) - 1] = '\0';
            strncpy(sec->key_pas, mtlsbuff, sizeof(sec->key_pas) - 1);
            sec->key_pas[sizeof(sec->key_pas) - 1] = '\0';
        */
    /* TODO: RDKE-419: temporary change until RDKE-419 gets proper solution. */
    (void)sec;	
    return MTLS_FAILURE;
}
#endif

#ifdef __cplusplus
}
#endif
