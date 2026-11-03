/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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


#ifndef RDKCERTSELECTOR_H
#define RDKCERTSELECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void* rdkcertselector_h;

typedef enum {
    certselectorOk = 0,
    certselectorError = -1
} rdkcertselectorStatus_t;

#ifndef TRY_ANOTHER
#define TRY_ANOTHER 1
#endif
#ifndef DEFAULT_CONFIG
#define DEFAULT_CONFIG ""
#endif
#ifndef DEFAULT_HROT
#define DEFAULT_HROT ""
#endif

rdkcertselector_h rdkcertselector_new(const char* config, const char* hrot, const char* certGroup);
rdkcertselectorStatus_t rdkcertselector_getCert(rdkcertselector_h selector, char** certUri, char** certPass);
void rdkcertselector_free(rdkcertselector_h* selector);
char* rdkcertselector_getEngine(rdkcertselector_h selector);
int rdkcertselector_setCurlStatus(rdkcertselector_h selector, int curlStatus, const char* url);

void rdkcertselector_mock_reset(void);
void rdkcertselector_mock_set_get_cert(rdkcertselectorStatus_t status, char* certUri, char* certPass);
void rdkcertselector_mock_set_engine(char* engine);
void rdkcertselector_mock_set_free_result(rdkcertselector_h selectorAfterFree);

#ifdef __cplusplus
}
#endif

#endif
