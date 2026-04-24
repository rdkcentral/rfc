/**
 * @file mtlsUtils.cpp
 * @brief mTLS certificate retrieval and state-red recovery implementation.
 *
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "mtlsUtils.h"
#include "rfc_common.h"
#ifdef LIBRDKCONFIG_BUILD
#include "rdkconfig.h"
#endif
#ifdef RDKC
#include <sys/stat.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Check if state-red recovery is supported on this device.
 * @retval 1  Supported (support file exists).
 * @retval 0  Not supported.
 */
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

/**
 * @brief Check if the device is currently in state-red.
 * @retval 1  Device is in state-red.
 * @retval 0  Device is not in state-red.
 */
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
/**
 * @brief Fetch mTLS certificate and key via librdkcertselector.
 * @param[out] sec           Populated mTLS auth structure.
 * @param[out] pthisCertSel  Certificate-selector handle (caller frees).
 * @return MtlsAuthStatus result code.
 */
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
#elif defined(RDKC)
/**
 * @brief RDKC camera mTLS certificate retrieval.
 *
 * Mirrors mtlsUtils.sh / checkxpki flow from RFCbase.sh:
 *   1. Try dynamic XPKI cert (CERT_DYNAMIC).
 *   2. Fall back to static XPKI cert (CERT_STATIC).
 *   3. If neither available, return MTLS_FAILURE.
 */
static int runShellCmd(const char *cmd, char *outBuf, size_t bufLen)
{
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }
    if (fgets(outBuf, bufLen, fp) != NULL) {
        size_t len = strlen(outBuf);
        while (len > 0 && (outBuf[len - 1] == '\n' || outBuf[len - 1] == '\r')) {
            outBuf[--len] = '\0';
        }
        pclose(fp);
        return (len > 0) ? 0 : -1;
    }
    pclose(fp);
    return -1;
}

int getMtlscert(MtlsAuth_t *sec) {
    struct stat st;

    if (sec == NULL) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "getMtlscert(): NULL pointer\n");
        return MTLS_FAILURE;
    }
    memset(sec, '\0', sizeof(MtlsAuth_t));

    /* Try dynamic XPKI cert first — matches checkxpki() in utils.sh */
    if (stat(CERT_DYNAMIC, &st) == 0 && st.st_size > 0) {
        strncpy(sec->cert_name, CERT_DYNAMIC, sizeof(sec->cert_name) - 1);
        sec->cert_name[sizeof(sec->cert_name) - 1] = '\0';
        strncpy(sec->cert_type, "P12", sizeof(sec->cert_type) - 1);
        sec->cert_type[sizeof(sec->cert_type) - 1] = '\0';
        /* Password via rdkssacli — same as getxpkiPass() in utils.sh */
        if (runShellCmd(". /lib/rdk/utils.sh && getxpkiPass", sec->key_pas, sizeof(sec->key_pas)) == 0) {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "getMtlscert(): RDKC dynamic XPKI cert: %s\n", CERT_DYNAMIC);
            return MTLS_SUCCESS;
        }
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "getMtlscert(): RDKC dynamic cert found but password retrieval failed\n");
    }

    /* Fall back to static XPKI cert — matches static path in checkxpki() */
    if (stat(CERT_STATIC, &st) == 0 && st.st_size > 0) {
        strncpy(sec->cert_name, CERT_STATIC, sizeof(sec->cert_name) - 1);
        sec->cert_name[sizeof(sec->cert_name) - 1] = '\0';
        strncpy(sec->cert_type, "P12", sizeof(sec->cert_type) - 1);
        sec->cert_type[sizeof(sec->cert_type) - 1] = '\0';
        /* Password via GetConfigFile — same as getstaticxpkiPass() in utils.sh */
        if (runShellCmd(". /lib/rdk/utils.sh && getstaticxpkiPass", sec->key_pas, sizeof(sec->key_pas)) == 0) {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "getMtlscert(): RDKC static XPKI cert: %s\n", CERT_STATIC);
            return MTLS_SUCCESS;
        }
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "getMtlscert(): RDKC static cert found but password retrieval failed\n");
    }

    /* No cert available — models without XPKI proceed without mTLS */
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "getMtlscert(): RDKC no XPKI cert available, proceeding without mTLS\n");
    return MTLS_FAILURE;
}
#else
/**
 * @brief Fetch mTLS certificate and key (legacy / non-certselector path).
 * @param[out] sec  Populated mTLS auth structure.
 * @return MTLS_SUCCESS (1) or MTLS_FAILURE (-1).
 */
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
