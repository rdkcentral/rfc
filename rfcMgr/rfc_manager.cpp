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
        rdk_logger_init(0 == access("/opt/debug.ini", R_OK) ? "/opt/debug.ini" : "/etc/debug.ini");

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
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] CheckIPRoute Waiting for Route Config %s file\n", __FUNCTION__,__LINE__, IP_ROUTE_FLAG);
        while (IpRouteCnt--) 
        {
            if (RDK_API_SUCCESS == (filePresentCheck(IP_ROUTE_FLAG))) {
                break;
            }
            sleep(15);
        }
        if (IpRouteCnt == 0 && (RDK_API_SUCCESS == (filePresentCheck(IP_ROUTE_FLAG)))) {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] route flag=%s not present\n", __FUNCTION__,__LINE__, IP_ROUTE_FLAG);
            return ip_status;
        }
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] CheckIPRoute Received Route Config file\n", __FUNCTION__,__LINE__);
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
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] ip address=%s\n", __FUNCTION__,__LINE__, tbuff);
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
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] isDnsResolve(): parameter is NULL\n", __FUNCTION__,__LINE__);
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
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] dns resolve file:%s not present\n", __FUNCTION__,__LINE__, dns_file_name);
        }
        return dns_status;
    }

    DeviceStatus RFCManager ::CheckDeviceIsOnline() 
    {
        RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] Checking IP and Route configuration\n", __FUNCTION__,__LINE__);
        rfc::DeviceStatus result = RFCMGR_DEVICE_OFFLINE;
        if (true == CheckIProuteConnectivity(GATEWAYIP_FILE)) 
        {
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] Checking IP and Route configuration found\n", __FUNCTION__,__LINE__);
            RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] Checking DNS Nameserver configuration\n", __FUNCTION__,__LINE__);
            if (true == (isDnsResolve(DNS_RESOLV_FILE))) 
            {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR, "[%s][%d] DNS Nameservers are available\n",__FUNCTION__,__LINE__);
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
        return result;
    }

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

    int RFCManager::RFCManagerPostProcess()
    {
        if(-1 == v_secure_system("sh %s",RFC_MGR_IPTBLE_INIT_SCRIPT))
	{
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR, "[%s][%d] Script[%s] Execution Failed ...!!\n", __FUNCTION__,__LINE__,RFC_MGR_IPTBLE_INIT_SCRIPT);
	    return FAILURE;
	}
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
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] Xconf Initialization ...!!\n", __FUNCTION__,__LINE__);
	    delete rfcObj;
            return reqStatus;
        }

        result = rfcObj->ProcessRuntimeFeatureControlReq();

        if(result == SUCCESS)
        {
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_COMPLETE); 

            bool isRebootRequired = rfcObj->getRebootRequirement();
            if(isRebootRequired == true)
            {
                RDK_LOG(RDK_LOG_DEBUG, LOG_RFCMGR,"[%s][%d] RFC: Posting Reboot Required Event to MaintenanceMGR\n", __FUNCTION__,__LINE__);
		t2CountNotify("SYST_ERR_RFC_Reboot", 1);
                SendEventToMaintenanceManager("MaintenanceMGR", MAINT_CRITICAL_UPDATE);
                SendEventToMaintenanceManager("MaintenanceMGR", MAINT_REBOOT_REQUIRED);
            }
            reqStatus = SUCCESS;
        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR, LOG_RFCMGR,"[%s][%d] RFC: Posting RFC Error Event to MaintenanceMGR\n", __FUNCTION__,__LINE__);
            SendEventToMaintenanceManager("MaintenanceMGR", MAINT_RFC_ERROR);
        }
        
	int post_process_result = RFCManagerPostProcess();
	if(post_process_result == SUCCESS)
	{
            RDK_LOG(RDK_LOG_INFO, LOG_RFCMGR,"[%s][%d] RFC:Post Processing Successfully Completed\n", __FUNCTION__,__LINE__);
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
} // namespace RFC

