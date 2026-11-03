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


#include "rdkcertselector.h"

namespace {
rdkcertselectorStatus_t getCertStatus = certselectorOk;
char* certificateUri = nullptr;
char* certificatePassword = nullptr;
char* engineName = nullptr;
rdkcertselector_h freeResult = nullptr;
}

extern "C" {

rdkcertselector_h rdkcertselector_new(const char*, const char*, const char*)
{
    return reinterpret_cast<rdkcertselector_h>(0x1);
}

rdkcertselectorStatus_t rdkcertselector_getCert(rdkcertselector_h, char** certUri, char** certPass)
{
    *certUri = certificateUri;
    *certPass = certificatePassword;
    return getCertStatus;
}

void rdkcertselector_free(rdkcertselector_h* selector)
{
    *selector = freeResult;
}

char* rdkcertselector_getEngine(rdkcertselector_h)
{
    return engineName;
}

int rdkcertselector_setCurlStatus(rdkcertselector_h, int, const char*)
{
    return 0;
}

void rdkcertselector_mock_reset(void)
{
    getCertStatus = certselectorOk;
    certificateUri = nullptr;
    certificatePassword = nullptr;
    engineName = nullptr;
    freeResult = nullptr;
}

void rdkcertselector_mock_set_get_cert(rdkcertselectorStatus_t status, char* certUri, char* certPass)
{
    getCertStatus = status;
    certificateUri = certUri;
    certificatePassword = certPass;
}

void rdkcertselector_mock_set_engine(char* engine)
{
    engineName = engine;
}

void rdkcertselector_mock_set_free_result(rdkcertselector_h selectorAfterFree)
{
    freeResult = selectorAfterFree;
}

}
