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


#ifdef LIBRDKCERTSELECTOR
/* Description: Use for get all mtls related certificate and key.
 * @param sec: This is a pointer hold the certificate, key and type of certificate.
 * @return :  MTLS_CERT_FETCH_SUCCESS on success, MTLS_CERT_FETCH_FAILURE on mtls cert failure
 * */
MtlsAuthStatus getMtlscert(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel) {
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
    /* TODO: RDKE-419: temporary change until RDKE-419 gets proper solution. */
    (void)sec;	
    return MTLS_FAILURE;
}
#endif

#ifdef __cplusplus
}
#endif
