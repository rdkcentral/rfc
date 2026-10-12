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


#include "rfc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PLACEHOLDER_STRING
#define PLACEHOLDER_STRING "sample"
#endif

#ifndef PLACEHOLDER_MAC
#define PLACEHOLDER_MAC "00:11:22:33:44:55"
#endif

#ifndef PLACEHOLDER_INTERFACE
#define PLACEHOLDER_INTERFACE "wan0"
#endif

#if defined(RDKB_SUPPORT)
std::string getWanInterfaceName()
{
    return PLACEHOLDER_INTERFACE;
}

std::string getWanMacInterfaceName()
{
    return PLACEHOLDER_MAC;
}

std::string getBoxTypeFromFile()
{
    return PLACEHOLDER_STRING;
}

std::string getErouterMac()
{
    return PLACEHOLDER_MAC;
}

std::string geteCMMac()
{
    return PLACEHOLDER_MAC;
}
#endif

#ifdef __cplusplus
}
#endif
