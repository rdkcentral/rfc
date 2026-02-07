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


def test_set_invalid_accountid_value():
    command_to_check = "tr181 -d -s -t string -v 306045!@#06186635988 Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
    result = run_shell_command(command_to_check)
    assert "Set operation success" in result, '"Set operation success" not found in the output'

def test_xconf_request_response():
    """
    Test the communication between RFC Manager and XCONF.

    This function checks the creation of the TR181 INI file,
    verifies the firmware version update, and checks the key-value pair in the TR181 INI file.
    """
    try:
        rfc_run_binary()
        invalid_accid_msg_status = "Invalid characters in newly received accountId"

        assert grep_log_file(RFC_LOG_FILE, invalid_accid_msg_status), f"Expected '{invalid_accid_msg_status}' in log file."
    except Exception as e:
        print(f"Exception during Validate the XConf request and response: {e}")
        assert False, f"Exception during Validate the XConf request and response: {e}"

