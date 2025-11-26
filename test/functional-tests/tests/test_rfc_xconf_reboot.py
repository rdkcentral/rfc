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

def test_Set_AccId_value():
    command_to_check = "tr181 -d -s -t string -v 3060457086186635988 Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID"
    result = run_shell_command(command_to_check)
    assert "Set operation success" in result, '"Set operation success" not found in the output'

    
def test_xconf_reboot():
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
        REBOOT_REQUIRED_MSG = f"RFC: Posting Reboot Required Event to MaintenanceMGR"

        assert grep_log_file(RFC_LOG_FILE, REBOOT_REQUIRED_MSG), f"Expected '{REBOOT_REQUIRED_MSG}' in log file."
    except Exception as e:
        print(f"Exception during Validating the parameters in RFC curl Request to xconf: {e}")
        assert False, f"Exception during Validating the parameters in RFC curl Request to xconf: {e}"
