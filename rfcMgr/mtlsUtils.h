/**
 * @file mtlsUtils.h
 * @brief mTLS certificate retrieval and state-red recovery utilities.
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

#ifndef VIDEO_REF_INTERFACE_MTLSUTILS_H_
#define VIDEO_REF_INTERFACE_MTLSUTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include <system_utils.h>
#include <urlHelper.h>

#ifdef LIBRDKCERTSELECTOR	
#include "rdkcertselector.h"

/** cURL error returned when the local certificate file is missing/broken. */
#define CURL_MTLS_LOCAL_CERTPROBLEM 58

/**
 * @brief mTLS certificate fetch status codes (librdkcertselector path).
 */
typedef enum {
    STATE_RED_CERT_FETCH_FAILURE = -2, /**< State-red recovery itself failed. */
    MTLS_CERT_FETCH_FAILURE = -1,      /**< General mTLS cert fetch failure. */
    MTLS_CERT_FETCH_SUCCESS = 0        /**< Certificate fetched successfully. */
} MtlsAuthStatus;

/**
 * @brief Fetch mTLS certificate and key via librdkcertselector.
 * @param[out] sec           Populated mTLS auth structure.
 * @param[out] pthisCertSel  Certificate-selector handle (caller frees).
 * @return MtlsAuthStatus result code.
 */
MtlsAuthStatus getMtlscert(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel);
#else
#define MTLS_SUCCESS  1  /**< mTLS success (non-librdkcertselector path). */
#define MTLS_FAILURE -1  /**< mTLS failure (non-librdkcertselector path). */

/**
 * @brief Fetch mTLS certificate (legacy / non-certselector path).
 * @param[out] sec  Populated mTLS auth structure.
 * @return MTLS_SUCCESS or MTLS_FAILURE.
 */
int getMtlscert(MtlsAuth_t *sec);
#endif

#define CERT_DYNAMIC "/opt/certs/devicecert_1.pk12"  /**< Dynamic XPKI certificate path. */
#define CERT_STATIC  "/etc/ssl/certs/staticXpkiCrt.pk12" /**< Static XPKI certificate path. */

/**
 * @name Certificate / key placeholders.
 * Override at compile time with device-specific paths.
 * @{ */
#define KEY_STATIC  ""
#define STATE_RED_CERT ""
#define STATE_RED_KEY ""
#define MTLS_CERT ""
#define MTLS_KEY ""
#define GET_CONFIG_FILE ""
#define SYS_CMD_GET_CONFIG_FILE ""
#define STATE_RED_SPRT_FILE ""
#define STATEREDFLAG ""
/** @} */

#if defined(GTEST_ENABLE)
/** @brief Check if state-red recovery is supported on this build. */
int isStateRedSupported(void);
/** @brief Check if the device is currently in state-red. */
int isInStateRed(void);
#endif

#if defined(RDKB_SUPPORT)
/** @brief Get the eRouter MAC address string. */
std::string getErouterMac();
/** @brief Get the eCM (cable modem) MAC address string. */
std::string geteCMMac();
#endif

#ifdef __cplusplus
}
#endif


#endif /* VIDEO_REF_INTERFACE_MTLSUTILS_H_ */
