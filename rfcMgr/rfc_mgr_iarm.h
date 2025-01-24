/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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
 **/

#ifndef RFC_MGR_IARM_H
#define RFC_MGR_IARM_H

#if defined(USE_IARMBUS) || defined(USE_IARM_BUS)
#include "libIARM.h"
#include "libIARMCore.h"
#include "libIBus.h"
#include "libIBusDaemon.h"

#define IARM_RFCMANAGER_EVENT "RDKVRFCMgrEvent"
#define IARM_BUS_RDKVRFC_MGR_NAME "RdkvRFCMgr"
#define IARM_BUS_RDK_RFCMGR_DWNLD_CONFIGURATION 0
#define IARM_BUS_NETSRVMGR_API_IsIARMBusConnectedToInternet "IsIARMBusConnectedToInternet"
#define IARM_BUS_NM_SRV_MGR_NAME "NET_SRV_MGR"
#endif

#endif
