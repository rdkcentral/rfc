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

// Below macro is invoked if the getMtlscert API fails to retrieve all MTLS certificates.
#define CURL_MTLS_LOCAL_CERTPROBLEM 58

typedef enum {
    MTLS_CERT_FETCH_FAILURE = -1,          // Indicates general MTLS failure
    MTLS_CERT_FETCH_SUCCESS = 0            // Indicates success
} MtlsAuthStatus;

MtlsAuthStatus getMtlscert(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel);
#else
#define MTLS_SUCCESS 1
#define MTLS_FAILURE -1
int getMtlscert(MtlsAuth_t *sec);
#endif

#define CERT_DYNAMIC "/opt/certs/devicecert_1.pk12"
#define CERT_STATIC  "/etc/ssl/certs/staticXpkiCrt.pk12"

/**
 * Below macros are placeholders.
 * Users are free to override with their custom certs and verifier algorithms during compile time 
 **/
#define KEY_STATIC  ""
#define MTLS_CERT ""
#define MTLS_KEY ""
#define GET_CONFIG_FILE ""
#define SYS_CMD_GET_CONFIG_FILE ""



#if defined(RDKB_SUPPORT)
std::string getErouterMac();
std::string geteCMMac();
#endif

#ifdef __cplusplus
}
#endif


#endif /* VIDEO_REF_INTERFACE_MTLSUTILS_H_ */
