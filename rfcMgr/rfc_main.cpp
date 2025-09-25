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

#include "rfc_common.h"
#include "rfc_manager.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "common_device_api.h"
#include <unistd.h>

#ifdef INCLUDE_BREAKPAD
#include "breakpad_wrapper.h"
#endif

#include <signal.h>

// Cleanup function
void cleanup_lock_file(void)
{
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC: Completed service, deleting lock\n", __FUNCTION__, __LINE__);
    unlink(RFC_MGR_SERVICE_LOCK_FILE);
}

// Signal handler for graceful shutdown
void signal_handler(int sig)
{
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "RFC: Received signal %d, cleaning up lock file\n", sig);	
    cleanup_lock_file();
    exit(0);
}

bool createDirectoryIfNotExists(const char* path) {
    if (!path || *path == '\0') {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Invalid path\n", __FUNCTION__, __LINE__);
        return false;
    }

    if (mkdir(path, 0755) == 0) {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Directory created: %s\n", __FUNCTION__, __LINE__, path);
        return true;
    }

    if (errno == EEXIST) {
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            return true;
        }
    }

    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to create directory: %s (%s)\n", __FUNCTION__, __LINE__, path, strerror(errno));
    return false;
}

int main()
{
    pid_t pid;
    pid = fork(); // Fork off the parent process
    
    if (pid < 0)
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFC:Fork Failed.\n", __FUNCTION__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
    atexit(cleanup_lock_file);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);	
	
    rfc::RFCManager* rfcMgr = new rfc::RFCManager();

    if (!createDirectoryIfNotExists(SECURE_RFC_PATH)) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFC: Failed to create %s RFC directory\n", __FUNCTION__, __LINE__,SECURE_RFC_PATH);
        exit(EXIT_FAILURE);
    }

    if (!createDirectoryIfNotExists(RFC_RAM_PATH)) {
        RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] RFC: Failed to create %s RFC directory\n", __FUNCTION__, __LINE__, RFC_RAM_PATH);
        exit(EXIT_FAILURE);
    }
	
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC: Starting service, creating lock \n", __FUNCTION__, __LINE__);

#if defined(RDKB_SUPPORT)
    waitForRfcCompletion();
#endif

     /* Abort if another instance of rfcMgr is already running */
    if (CurrentRunningInst(RFC_MGR_SERVICE_LOCK_FILE))
    {
#if !defined(RDKB_SUPPORT)	    
	rfcMgr->SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_INPROGRESS);
#endif	
	RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC: rfcMgr process in progress, New instance not allowed as file %s is locked!\n", __FUNCTION__, __LINE__, RFC_MGR_SERVICE_LOCK_FILE);
        delete rfcMgr;
        return 1;
    }
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Waiting for IP Acquistion\n", __FUNCTION__, __LINE__);
    rfc::DeviceStatus isDeviceOnline = rfcMgr->CheckDeviceIsOnline();
    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Starting execution of RFCManager\n", __FUNCTION__, __LINE__);    
    if (isDeviceOnline == rfc::RFCMGR_DEVICE_ONLINE) 
    {
        int status = FAILURE;
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC:Device is Online\n", __FUNCTION__, __LINE__);
        status = rfcMgr->RFCManagerProcessXconfRequest();
        if(status == SUCCESS)
        {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC:Xconf Request Processed successfully\n", __FUNCTION__, __LINE__);  
        }
#if defined(RDKB_SUPPORT)
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d]START CONFIGURING RFC CRON \n", __FUNCTION__, __LINE__);

        std::string cronConfig = getCronFromDCMSettings();

        // If no cron was found in DCM settings, use a default value
        if (cronConfig.empty()) {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] No cron found in DCM settings, skipping..\n", __FUNCTION__, __LINE__);
        } else {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Using cron from DCM settings: %s\n",  __FUNCTION__, __LINE__, cronConfig.c_str());
        }
        rfcMgr->manageCronJob(cronConfig);
#endif	
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC:Device is Offline\n", __FUNCTION__, __LINE__);
    }

#ifdef INCLUDE_BREAKPAD
     breakpad_ExceptionHandler();
#endif

    delete rfcMgr;

    return 0;
}
#ifdef __cplusplus
}
#endif
