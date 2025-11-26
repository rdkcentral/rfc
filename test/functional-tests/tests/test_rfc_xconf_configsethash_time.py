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

def test_Set_ConfigSetTime_value():
    command_to_check = "tr181 -d -s -t uint32 -v 1763118860 Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetTime"
    result = run_shell_command(command_to_check)
    assert "Set operation success" in result, '"Set operation success" not found in the output'

def test_rfcMgr_xconf_communication() -> None:
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
        print(f"Exception during rfc binary execution: {e}")
        assert False, f"Exception during rfc binary execution: {e}"

    finally:
        if os.path.exists(RFC_OLD_FW_FILE + "_bak"):
            rename_file(RFC_OLD_FW_FILE + "_bak", RFC_OLD_FW_FILE)

def test_rfcMgr_xconf_communication_configSetHash_Time() -> None:
    try:
        rfc_run_binary()

        CFG_SET_HASH_MSG = "ConfigSetHash: Hash = 1KM7h9ommUuUoyVm8oAvp2JCC19zyVJAsp"
        CFG_SET_TIME_MSG = "ConfigSetTime: Set Time = "
        CFG_SET_HASH_VALUE_MSG = "configSetHash value: 1KM7h9ommUuUoyVm8oAvp2JCC19zyVJAsp"
        CFG_SET_HASH = "Config Set Hash = 1KM7h9ommUuUoyVm8oAvp2JCC19zyVJAsp"
        TS_MSG = "Timestamp as string: = "

        assert grep_log_file(RFC_LOG_FILE, CFG_SET_HASH_MSG), f"Expected '{CFG_SET_HASH_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CFG_SET_TIME_MSG), f"Expected '{CFG_SET_TIME_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CFG_SET_HASH_VALUE_MSG), f"Expected '{CFG_SET_HASH_VALUE_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CFG_SET_HASH), f"Expected '{CFG_SET_HASH}' in log file."
        assert grep_log_file(RFC_LOG_FILE, TS_MSG), f"Expected '{TS_MSG}' in log file."
    except Exception as e:
        print(f"Exception during Validating the ConfigHash and ConfigSetTime are populated for each RFC query updates: {e}")
        assert False, f"Exception during Validating the ConfigHash and ConfigSetTime are populated for each RFC query updates: {e}"
