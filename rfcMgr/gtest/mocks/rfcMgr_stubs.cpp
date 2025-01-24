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

#include "libIBus.h"
#include <cstdlib> 

IARM_Result_t IARM_Bus_Init(const char *name)
{
    IARM_Result_t retCode = IARM_RESULT_SUCCESS;
    if (name == NULL) {
        retCode = IARM_RESULT_INVALID_PARAM;
    }
    return retCode;
}

IARM_Result_t IARM_Bus_Connect(void)
{
    IARM_Result_t retCode = IARM_RESULT_SUCCESS;
    return retCode;
}

IARM_Result_t IARM_Bus_Disconnect(void)
{
        IARM_Result_t retCode = IARM_RESULT_SUCCESS;
        return retCode;
}

IARM_Result_t IARM_Bus_Term(void)
{
        IARM_Result_t retCode = IARM_RESULT_SUCCESS;
        return retCode;
}

IARM_Result_t IARM_Bus_IsConnected(const char *memberName, int *isRegistered)
{
    IARM_Result_t retCode = IARM_RESULT_SUCCESS;
    if (memberName == NULL) {
        retCode = IARM_RESULT_INVALID_PARAM;
        *isRegistered = 0;
    } else {
        /*Always return - registered as true*/
        *isRegistered = 1;
    }

    return retCode;
}

IARM_Result_t IARM_Bus_RegisterEventHandler(const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler)
{
    IARM_Result_t retCode = IARM_RESULT_SUCCESS;
    return retCode;
}
IARM_Result_t IARM_Bus_UnRegisterEventHandler(const char* ownerName, IARM_EventId_t eventId)
{
    IARM_Result_t retCode = IARM_RESULT_SUCCESS;
    return retCode;
}
