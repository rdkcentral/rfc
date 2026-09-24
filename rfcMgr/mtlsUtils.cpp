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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDKCERTSELECTOR
/**
 * @brief Fetch mTLS certificate and key via librdkcertselector.
 * @param[out] sec           Populated mTLS auth structure.
 * @param[out] pthisCertSel  Certificate-selector handle (caller frees).
 * @return MtlsAuthStatus result code.
 */
MtlsAuthStatus getMtlscert(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel) {
    char *certUri = NULL;
    char *certPass = NULL;
    char *engine = NULL;
    char *certFile = NULL;

    if (!sec || !pthisCertSel) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s:%d] Invalid parameters\n", __FUNCTION__, __LINE__);
        return MTLS_CERT_FETCH_FAILURE;
    }

    rdkcertselectorStatus_t certStat = rdkcertselector_getCert(*pthisCertSel, &certUri, &certPass);

    if (certStat != certselectorOk || certUri == NULL || certPass == NULL) {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s:%d] Failed to retrieve certificate for MTLS\n",
                   __FUNCTION__, __LINE__);

        rdkcertselector_free(pthisCertSel);

        if (*pthisCertSel == NULL) {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s:%d] Cert selector memory freed\n",
                       __FUNCTION__, __LINE__);
        } else {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s:%d] Cert selector memory free failed\n",
                       __FUNCTION__, __LINE__);
        }

        return MTLS_CERT_FETCH_FAILURE;
    }

    /* Handle file:// URI scheme */
    certFile = certUri;
    if (strncmp(certFile, FILESCHEME, sizeof(FILESCHEME) - 1) == 0) {
        certFile += (sizeof(FILESCHEME) - 1);
    }

    /* Populate certificate name */
    strncpy(sec->cert_name, certFile, sizeof(sec->cert_name) - 1);
    sec->cert_name[sizeof(sec->cert_name) - 1] = '\0';

    /* Populate certificate password */
    strncpy(sec->key_pas, certPass, sizeof(sec->key_pas) - 1);
    sec->key_pas[sizeof(sec->key_pas) - 1] = '\0';

    /* Get engine (optional) */
    engine = rdkcertselector_getEngine(*pthisCertSel);
    if (engine == NULL) {
        sec->engine[0] = '\0';
    } else {
        strncpy(sec->engine, engine, sizeof(sec->engine) - 1);
        sec->engine[sizeof(sec->engine) - 1] = '\0';
    }

    /* Set certificate type to P12 */
    strncpy(sec->cert_type, "P12", sizeof(sec->cert_type) - 1);
    sec->cert_type[sizeof(sec->cert_type) - 1] = '\0';

    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s:%d] MTLS cert success. cert=%s, type=%s, engine=%s\n",
               __FUNCTION__, __LINE__, sec->cert_name, sec->cert_type, sec->engine);

    return MTLS_CERT_FETCH_SUCCESS;
}
#else
/**
 * @brief Fetch mTLS certificate and key (legacy / non-certselector path).
 * @param[out] sec  Populated mTLS auth structure.
 * @return MTLS_SUCCESS (1) or MTLS_FAILURE (-1).
 */
int getMtlscert(MtlsAuth_t *sec) {
    /*
            strncpy(sec->cert_name, MTLS_CERT, sizeof(sec->cert_name) - 1);
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
