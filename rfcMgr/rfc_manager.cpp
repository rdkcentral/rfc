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

#include "rfc_manager.h"
#include "rfc_common.h"
#include "rfc_mgr_iarm.h"
#include "rfc_xconf_handler.h"
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace rfc {
#if defined(USE_IARMBUS)
    void rfcMgrEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len) 
    {
        /* This is place holder for future */
        owner = owner;
        eventId = eventId;
        data = data;
        len = len;
        return;
    }
#endif
    RFCManager ::RFCManager() {
        /* Initialize RDK Logger */
        rdk_logger_init(0 == access(OVERIDE_DEBUG_INI_FILE, R_OK) ? OVERIDE_DEBUG_INI_FILE : DEBUG_INI_FILE);	    

        /* Initialize IARM Bus */
        InitializeIARM();
    }

    /** Description: Check if Module is connected to IARM
     *
     *  @param cur_event_name: event name.
     *  @param event_status: Status Of the event.
     *  @return void.
     */
    bool RFCManager ::IsIarmBusConnected() {
#if defined(USE_IARMBUS)
        IARM_Result_t res;
        int isRegistered = 0;
        res = IARM_Bus_IsConnected(IARM_RFCMANAGER_EVENT, &isRegistered);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_IsConnected: %d (%d)", __FUNCTION__, __LINE__, res, isRegistered);

        if (isRegistered == 1) 
        {
            return true;
        } 
        else 
        {
            return false;
        }
#else
	return true;
#endif
    }

    void RFCManager ::InitializeIARM(void) {
#if defined(USE_IARMBUS)
        IARM_Result_t res;
        if (IsIarmBusConnected()) 
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM already connected\n",__FUNCTION__, __LINE__);
        } 
        else 
        {
            res = IARM_Bus_Init(IARM_RFCMANAGER_EVENT);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Init: %d\n", __FUNCTION__, __LINE__, res);
            if (res == IARM_RESULT_SUCCESS || res == IARM_RESULT_INVALID_STATE) {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] SUCCESS: IARM_Bus_Init done!\n", __FUNCTION__, __LINE__);

                res = IARM_Bus_Connect();
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Connect: %d\n", __FUNCTION__, __LINE__, res);
                if (res == IARM_RESULT_SUCCESS || res == IARM_RESULT_INVALID_STATE) 
                {
                    bool result = IsIarmBusConnected();
                    if(result)
                    {
                        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM connected\n", __FUNCTION__,__LINE__);
                    }
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] SUCCESS: IARM_Bus_Connect done!\n", __FUNCTION__,__LINE__);
                } 
                else 
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Connect failure: %d\n", __FUNCTION__,__LINE__, res);
                }
            } else {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_Init failure: %d\n", __FUNCTION__,__LINE__, res);
            }

            res = IARM_Bus_RegisterEventHandler(IARM_BUS_RDKVRFC_MGR_NAME, IARM_BUS_RDK_RFCMGR_DWNLD_CONFIGURATION, rfcMgrEventHandler);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] IARM_Bus_RegisterEventHandler ret=%d\n", __FUNCTION__,__LINE__, res);

        }
#endif
    }

    /** Description: This API UnRegister IARM event handlers in order to release
     * bus-facing resources.
     *
     *  @param void
     *  @return 0.
     */
    int term_event_handler(void) {
#if defined(USE_IARMBUS)
        IARM_Result_t res;
        res = IARM_Bus_UnRegisterEventHandler(IARM_BUS_RDKVRFC_MGR_NAME, IARM_BUS_RDK_RFCMGR_DWNLD_CONFIGURATION);
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully terminated all event handlers:%d\n", __FUNCTION__,__LINE__, res);
        IARM_Bus_Disconnect();
        IARM_Bus_Term();
#endif
        return 0;
    }

    std::string RFCManager::getErouterIPAddress() {
        std::string address;

        // Try to get IPv6 address first
        FILE* pipe = popen("dmcli eRT retv Device.DeviceInfo.X_COMCAST-COM_WAN_IPv6", "r");
        if (pipe) {
            char buffer[128] = {0};
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                address = buffer;
                // Trim trailing newline
                if (!address.empty() && address.back() == '\n') {
                    address.pop_back();
                }
            }
            pclose(pipe);
        }

        // If IPv6 not available, get IPv4
        if (address.empty()) {
            FILE* pipe = popen("dmcli eRT retv Device.DeviceInfo.X_COMCAST-COM_WAN_IP", "r");
            if (pipe) {
                char buffer[128] = {0};
                if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    address = buffer;
                    // Trim trailing newline
                    if (!address.empty() && address.back() == '\n') {
                        address.pop_back();
                    }
                }
                pclose(pipe);
            }
        }

        return address;
    }

    bool RFCManager::CheckIPConnectivity(void)
    {
        bool ip_status = false;

        std::string ip_address = getErouterIPAddress();

        if (!ip_address.empty()) {
            ip_status = true;
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Successfully got eRouter IP address\n", __FUNCTION__, __LINE__);
        } else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to get eRouter IP address\n", __FUNCTION__, __LINE__);
        }

        // Log the IP address and connection status
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] eRouter IP Address: '%s'\n", __FUNCTION__, __LINE__, ip_address.c_str());
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] IP Connectivity Status: %s\n", __FUNCTION__, __LINE__, ip_status ? "Connected" : "Disconnected");

        return ip_status;
    }

    /* Description: Checking IP route address and device is online or not.
     *              Use IARM event provided by net service manager to check either
     *              device is online or not.
     * @param: file_name : pointer to gateway iproute config file name
     * return :true = success
     * return :false = failure */
    bool RFCManager ::CheckIProuteConnectivity(const char *file_name) {
        bool ip_status = false;
        bool string_check = false;
        FILE *fp = NULL;
        char tbuff[80] = {0};
        char *tmp;
        int IpRouteCnt = 5;

        if (file_name == NULL) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Incorrect File Name", __FUNCTION__,__LINE__);
            return ip_status;
        }
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Waiting for Route Config %s file\n", __FUNCTION__,__LINE__, IP_ROUTE_FLAG);
        while (IpRouteCnt--) 
        {
            if (RDK_API_SUCCESS == (filePresentCheck(IP_ROUTE_FLAG))) {
                break;
            }
            sleep(15);
        }
        if (IpRouteCnt == 0 && (RDK_API_SUCCESS == (filePresentCheck(IP_ROUTE_FLAG)))) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] route flag=%s not present\n", __FUNCTION__,__LINE__, IP_ROUTE_FLAG);
            return ip_status;
        }
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Received Route Config file\n", __FUNCTION__,__LINE__);
        fp = fopen(file_name, "r");
        if (fp != NULL) 
        {
            while (NULL != (fgets(tbuff, sizeof(tbuff), fp))) 
            {
                if (NULL != (strstr(tbuff, "IPV"))) {
                    string_check = true;
                    break;
                }
            }
            if (string_check == true) {
                tmp = strstr(tbuff, "IPV");
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] ip address=%s\n", __FUNCTION__,__LINE__, tbuff);
                if (tmp != NULL && (NULL != strstr(tmp, "IPV4"))) 
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] default router Link Local IPV4 address present=%s\n", __FUNCTION__,__LINE__, tmp);
                } 
                else if (tmp != NULL && (NULL != strstr(tmp, "IPV6"))) 
                {
                    RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] default router Link Local IPV6 address present=%s\n", __FUNCTION__,__LINE__ ,tmp);
                } 
                else 
                {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] IP address type does not found\n", __FUNCTION__,__LINE__);
                }
            } 
            else 
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] File %s does not have IP address in proper format\n", __FUNCTION__,__LINE__, file_name);
            }
            fclose(fp);
        } 
        else 
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] ip route file:%s not present\n", __FUNCTION__,__LINE__, file_name);
        }
        ip_status = true;

        /*if (true == checkDeviceInternetConnection(RFC_MGR_INTERNET_CHECK_TIMEOUT))
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Device is online\n", __FUNCTION__,__LINE__);
            ip_status = true;
        } 
        else 
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Device is not online\n", __FUNCTION__,__LINE__);
            ip_status = false;
        }*/
        return ip_status;
    }

    /* Description: Checking dns nameserver ip is present or not.
     * @param: dns_file_name : pointer to dns config file name
     * return :true = success
     * return :false = failure */
    bool isDnsResolve(const char *dns_file_name)
    {
        bool dns_status = false;
        bool string_check = false;
        FILE *fp = NULL;
        char tbuff[80] = {0};
        char *tmp;
        if (dns_file_name == NULL) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Input parameter is NULL\n", __FUNCTION__,__LINE__);
            return dns_status;
        }
        fp = fopen(dns_file_name , "r");
        if (fp != NULL) 
        {
            while (NULL != (fgets(tbuff, sizeof(tbuff), fp))) {
                if (NULL != (strstr(tbuff, "nameserver"))) {
                    string_check = true;
                    break;
                }
            }
            if (string_check == true) 
            {
                tmp = strstr(tbuff, "nameserver");
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] dns resolve data=%s\n", __FUNCTION__,__LINE__, tbuff);
                if (tmp != NULL) 
                {
                    tmp = tmp + 10;
                    if (*tmp != '\0' && *tmp != '\n') 
                    {
                        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] dns nameserver present.\n", __FUNCTION__,__LINE__);
                        dns_status = true;
                    }
                }
            }
            fclose(fp);
        }
        else 
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] dns resolve file:%s not present\n", __FUNCTION__,__LINE__, dns_file_name);
        }
        return dns_status;
    }

    DeviceStatus RFCManager ::CheckDeviceIsOnline()
    {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Checking IP and Route configuration\n", __FUNCTION__,__LINE__);
        rfc::DeviceStatus result = RFCMGR_DEVICE_OFFLINE;
#if !defined(RDKB_SUPPORT)
        if (true == CheckIProuteConnectivity(GATEWAYIP_FILE))
        {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Checking IP and Route configuration found\n", __FUNCTION__,__LINE__);
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] Checking DNS Nameserver configuration\n", __FUNCTION__,__LINE__);
            if (true == (isDnsResolve(DNS_RESOLV_FILE)))
            {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] DNS Nameservers are available\n",__FUNCTION__,__LINE__);
                result = RFCMGR_DEVICE_ONLINE;
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] DNS Nameservers missing..!!\n",__FUNCTION__,__LINE__);
            }
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] IP and Route configuration not found...!!\n", __FUNCTION__,__LINE__);
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_ERROR);
        }
#else
        if (true == CheckIPConnectivity()){
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] IP configuration found...\n", __FUNCTION__,__LINE__);
            result = RFCMGR_DEVICE_ONLINE;
        }
        else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] IP configuration not found!!\n", __FUNCTION__,__LINE__);
        }
#endif
        return result;
    }

#if !defined(RDKB_SUPPORT)
    /** Description: Send event to iarm event manager
     *
     *  @param cur_event_name: event name.
     *  @param main_mgr_event: Status Of the event.
     *  @return void.
     */
    void RFCManager::SendEventToMaintenanceManager(const char *cur_event_name, unsigned int main_mgr_event)
    {
        cur_event_name = cur_event_name;
        main_mgr_event = main_mgr_event;
#ifdef EN_MAINTENANCE_MANAGER
        if ( !(strncmp(cur_event_name,"MaintenanceMGR", 14)) )
        {
            IARM_Bus_MaintMGR_EventData_t infoStatus;
            IARM_Result_t ret_code = IARM_RESULT_SUCCESS;

            memset( &infoStatus, 0, sizeof(IARM_Bus_MaintMGR_EventData_t) );
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] >>>>> Identified MaintenanceMGR with event value=%u\n", __FUNCTION__,__LINE__, main_mgr_event);
            infoStatus.data.maintenance_module_status.status = (IARM_Maint_module_status_t)main_mgr_event;
            ret_code=IARM_Bus_BroadcastEvent(IARM_BUS_MAINTENANCE_MGR_NAME,(IARM_EventId_t)IARM_BUS_MAINTENANCEMGR_EVENT_UPDATE, (void *)&infoStatus, sizeof(infoStatus));
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] >>>>> IARM %s  Event  = %d\n",__FUNCTION__,__LINE__ , (ret_code == IARM_RESULT_SUCCESS) ? "SUCCESS" : "FAILURE",infoStatus.data.maintenance_module_status.status);
        }
#endif
    }
#endif

    int RFCManager::RFCManagerPostProcess()
    {
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] POSTPROCESSING IS RUN NOW !!! \n", __FUNCTION__, __LINE__);

#if defined(RDKB_SUPPORT)
        if (access(RFC_SSH_FILE, F_OK) == 0) {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC File for SSH present. Refreshing Firewall\n", __FUNCTION__, __LINE__);
            if(-1 == v_secure_system("sysevent set firewall-restart"))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to execute sysevent set firewall-restart\n", __FUNCTION__, __LINE__);
            }
        } else {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFC File for SSH is not present or empty\n", __FUNCTION__, __LINE__);
        }

        if(-1 == v_secure_system("dmcli eRT setv Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Snmpv3DHKickstart.RFCUpdateDone bool true"))
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to set RFCUpdateDone\n", __FUNCTION__, __LINE__);
        } else {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] RFCUpdateDone set to true\n", __FUNCTION__, __LINE__);
        }

        if (access(SSH_WHITELIST_SCRIPT, F_OK) == 0) {
            std::string sshscriptcmd = "sh " + std::string(SSH_WHITELIST_SCRIPT) + " &";
            std::string output;
            if(!ExecuteCommand(sshscriptcmd, output))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to execute %s script\n", __FUNCTION__, __LINE__, SSH_WHITELIST_SCRIPT);
            } else {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Triggered %s to execute successfully in background\n", __FUNCTION__, __LINE__, SSH_WHITELIST_SCRIPT);
            }
        } else {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] %s script does not exist, skipping execution\n", __FUNCTION__, __LINE__, SSH_WHITELIST_SCRIPT);
        }

#else
        if (access(IPTABLE_INIT_SCRIPT, F_OK) == 0) {
            std::string sshrefreshcmd = "sh " + std::string(IPTABLE_INIT_SCRIPT) + " SSH_Refresh &";
            std::string output;
            if(!ExecuteCommand(sshrefreshcmd, output))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to execute %s SSH_Refresh\n", __FUNCTION__, __LINE__, IPTABLE_INIT_SCRIPT);
            } else {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] %s SSH_Refresh executed successfully in background\n", __FUNCTION__, __LINE__, IPTABLE_INIT_SCRIPT);
            }

            // Execute iptables_init SNMP_Refresh in background
            std::string snmprefreshcmd = "sh " + std::string(IPTABLE_INIT_SCRIPT) + " SNMP_Refresh &";
            std::string output;
            if(!ExecuteCommand(snmprefreshcmd, output))
            {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to execute %s SNMP_Refresh\n", __FUNCTION__, __LINE__, IPTABLE_INIT_SCRIPT);
            } else {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] %s SNMP_Refresh executed successfully in background\n", __FUNCTION__, __LINE__,IPTABLE_INIT_SCRIPT);
            }
        } else {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Script[%s] does not exist, skipping execution\n", __FUNCTION__, __LINE__, IPTABLE_INIT_SCRIPT);
        }
#endif
        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] POSTPROCESSING IS COMPLETE !!!\n", __FUNCTION__, __LINE__);
        return SUCCESS;
    }

    int RFCManager::RFCManagerProcess()
    {
        /* Create Object for Xconf Handler */
        RuntimeFeatureControlProcessor *rfcObj = new RuntimeFeatureControlProcessor();

        int reqStatus = FAILURE;

        /* Initialize xconf Hanlder */
        int result = rfcObj->InitializeRuntimeFeatureControlProcessor();
        if(result == FAILURE)
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization Failed...!!\n", __FUNCTION__,__LINE__);
            delete rfcObj;
            return reqStatus;
        }

        result = rfcObj->ProcessRuntimeFeatureControlReq();

#if !defined(RDKB_SUPPORT)
        if(result == SUCCESS)
        {
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_COMPLETE);

            bool isRebootRequired = rfcObj->getRebootRequirement();
            if(isRebootRequired == true)
            {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC: Posting Reboot Required Event to MaintenanceMGR\n", __FUNCTION__,__LINE__);
                            rfcObj->NotifyTelemetry2Count("SYST_INFO_RFC_Reboot");
                SendEventToMaintenanceManager("MaintenanceMGR", MAINT_CRITICAL_UPDATE);
                SendEventToMaintenanceManager("MaintenanceMGR", MAINT_REBOOT_REQUIRED);
            }
            reqStatus = SUCCESS;
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC: Posting RFC Error Event to MaintenanceMGR\n", __FUNCTION__,__LINE__);
            rfcObj->NotifyTelemetry2Count("SYST_INFO_RFC_Error");
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_ERROR);
        }
#endif
        int post_process_result = RFCManagerPostProcess();
        if(post_process_result == SUCCESS)
        {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC:Post Processing Successfully Completed\n", __FUNCTION__,__LINE__);
            rfcObj->NotifyTelemetry2Count("SYST_INFO_RFC_PostProcess_Success");
        }
        else
        {
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC:Post Processing Failed\n", __FUNCTION__,__LINE__);
        }

        delete rfcObj;
        return reqStatus;
    }

    int RFCManager ::RFCManagerProcessXconfRequest() 
    {
        int ret_status = FAILURE;

        ret_status = RFCManagerProcess();

        return ret_status;
    }

    void rfc::RFCManager::manageCronJob(const std::string& cron)
    {
        std::string tempFile = "/tmp/cron_tab_tmp_file";
        std::string crontabPath = "/var/spool/cron/crontabs/";
        bool tempFileCreated = false;

        std::istringstream iss(cron);
        std::vector<std::string> cronParts;
        std::string part;
        while (iss >> part && cronParts.size() < 5) {
            cronParts.push_back(part);
        }

        if (cronParts.size() < 5) {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Invalid cron format: not enough components\n", __FUNCTION__, __LINE__);
            return;
        }

        // Parse cron values with validation
        std::array<int, 5> cronValues = {0, 0, 1, 1, 0};
        for (size_t i = 0; i < 5; i++) {
            if (cronParts[i] != "*") {
                try {
                    cronValues[i] = std::stoi(cronParts[i]);
                } catch (const std::exception& e) {
                    RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Invalid cron component '%s', using defaults\n", __FUNCTION__, __LINE__, cronParts[i].c_str());
                    cronValues = {0, 0, 1, 1, 0}; // Reset to defaults
                    break;
                }
            }
        }

        // Adjust time by 3 minutes
        if (cronValues[0] > 2) {
            cronValues[0] -= 3;
        } else {
            cronValues[0] += 57;
            cronValues[1] = (cronValues[1] == 0) ? 23 : cronValues[1] - 1;
        }

        std::string adjustedCron;
        for (size_t i = 0; i < 5; i++) {
            if (i > 0) adjustedCron += " ";
            
            // Use original cronParts if it was a wildcard, otherwise use adjusted cronValues
            if (cronParts[i] == "*") {
                adjustedCron += "*";
            } else {
                adjustedCron += std::to_string(cronValues[i]);
            }
        }

        RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Configuring cron job: %s\n", __FUNCTION__, __LINE__, adjustedCron.c_str());

        std::string cronEntry = adjustedCron + " /usr/bin/rfcMgr >> /rdklogs/logs/dcmrfc.log.0 2>&1";

        // Export existing crontab using popen to avoid redirection issues
        std::string exportCmd = "crontab -l -c " + crontabPath + " 2>&1";
        FILE* pipe = popen(exportCmd.c_str(), "r");

        std::ofstream tempFileOut(tempFile);
        if (pipe && tempFileOut.is_open()) {
            tempFileCreated = true;
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                tempFileOut << buffer;
            }
            tempFileOut.close();
            int exportResult = pclose(pipe);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Crontab export completed with result: %d\n", __FUNCTION__, __LINE__, exportResult);
        } else {
            if (pipe) pclose(pipe);
            if (tempFileOut.is_open()) tempFileOut.close();
            RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR, "[%s][%d] Failed to export crontab, creating empty file\n", __FUNCTION__, __LINE__);
            // Create empty file
            std::ofstream emptyFile(tempFile);
            if (emptyFile.is_open()) {
                tempFileCreated = true;
                emptyFile.close();
            } else {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to create temp file\n", __FUNCTION__, __LINE__);
                return;
            }
        }

        std::ifstream cronContent(tempFile);
        if (cronContent.is_open()) {
            std::string line;
            bool hasContent = false;

            // Check if file has any content while reading
            while (std::getline(cronContent, line)) {
                if (!hasContent) {
                    RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Existing cron entries:\n", __FUNCTION__, __LINE__);
                    hasContent = true;
                }
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] %s\n", __FUNCTION__, __LINE__, line.c_str());
            }
            cronContent.close();

            if (!hasContent) {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] No existing crontab content found\n", __FUNCTION__, __LINE__);
            }
        } else {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] Existing crontab content found empty\n", __FUNCTION__, __LINE__);
        }

        int sedResult = v_secure_system("sed -i '/rfcMgr/d; /RFCbase\\.sh/d' %s", tempFile.c_str());
        if (sedResult != 0) {
            RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR, "[%s][%d] No existing crontab found for RFC script \n", __FUNCTION__, __LINE__);
        }

        // Add new cron entry
        std::ofstream cronFile(tempFile, std::ios::app);
        if (cronFile.is_open()) {
            cronFile << cronEntry << std::endl;
            cronFile.close();

            // Apply crontab
            int applyResult = v_secure_system("crontab %s -c %s", tempFile.c_str(), crontabPath.c_str());
            if (applyResult == 0) {
                RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR, "[%s][%d] Successfully configured cron job\n", __FUNCTION__, __LINE__);
            } else {
                RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to apply crontab with error code %d\n", __FUNCTION__, __LINE__, applyResult);
            }
        } else {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Failed to open temp cron file for writing\n", __FUNCTION__, __LINE__);
        }

        if (tempFileCreated) {
            if (unlink(tempFile.c_str()) != 0) {
                RDK_LOG(RDK_LOG_WARN, LOG_RFCMGR, "[%s][%d] Warning: Failed to remove temp file %s\n", __FUNCTION__, __LINE__, tempFile.c_str());
            }
        }
    }

} // namespace RFC

