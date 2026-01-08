####################################################################################
# If not stated otherwise in this file or this component's Licenses file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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
####################################################################################

import os
from rfc_test_helper import *


def test_rfcmanager_rejects_empty_value():
    """
    Test the communication between RFC Manager and XCONF.

    This function checks the creation of the TR181 INI file,
    verifies the firmware version update, and checks the key-value pair in the TR181 INI file.
    """
    if os.path.exists(TR181_INI_FILE):
        os.remove(TR181_INI_FILE)

    if os.path.exists(RFC_OLD_FW_FILE):
        rename_file(RFC_OLD_FW_FILE, RFC_OLD_FW_FILE + "_bak")

    try:
        rfc_run_binary()
        XCONF_RESP_MSG_STATUS = f"EMPTY value for Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerProductName is rejected"

        assert grep_log_file(RFC_LOG_FILE, XCONF_RESP_MSG_STATUS), f"Expected '{XCONF_RESP_MSG_STATUS}' in log file."
    except Exception as e:
        print(f"Exception during Validate the XConf request and response: {e}")
        assert False, f"Exception during Validate the XConf request and response: {e}"

def test_set_empty_value():
    command_to_check = 'tr181 -d -s -t string -v "" Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName'
    result = run_shell_command(command_to_check)
    assert "Set operation success" in result, '"Set operation success" not found in the output'


def test_rfc_partnername_value():
    """
    Test the communication between RFC Manager and XCONF.

    This function checks the creation of the TR181 INI file,
    verifies the firmware version update, and checks the key-value pair in the TR181 INI file.
    """
    if os.path.exists(TR181_INI_FILE):
        os.remove(TR181_INI_FILE)

    if os.path.exists(RFC_OLD_FW_FILE):
        rename_file(RFC_OLD_FW_FILE, RFC_OLD_FW_FILE + "_bak")

    try:
        rfc_run_binary()
    except Exception as e:
        print(f"Exception during Validate the XConf request and response: {e}")
        assert False, f"Exception during Validate the XConf request and response: {e}"

def test_xconf_partnername_value():
    command_to_check = "tr181 -d -g Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.PartnerName"
    result = run_shell_command(command_to_check)
    assert "TestPartner" in result, '"TestPartner" not found in the output'

