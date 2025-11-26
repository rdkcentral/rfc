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


def test_xconf_request_params():
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
        MAC_ADDR_MSG = f"?estbMacAddress=01:23:45:67:89:ab"
        FW_VER_MSG = f"&firmwareVersion="
        ENV_MSG = f"&env="
        MODEL_MSG = f"&model=L2CNTR"
        MFR_MSG = f"&manufacturer="
        CTRL_ID_MSG = f"&controllerId="
        CH_MAP_ID_MSG = f"&channelMapId="
        VOD_ID_MSG = f"&VodId="
        PARTNER_ID_MSG = f"&partnerId=global"
        OSCLASS_MSG = f"&osClass="
        ACC_ID_MSG =f"&accountId="
        EXP_MSG = f"&Experience=X1"
        VER_MSG = f"&version=2"

        assert grep_log_file(RFC_LOG_FILE, MAC_ADDR_MSG), f"Expected '{MAC_ADDR_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, FW_VER_MSG), f"Expected '{FW_VER_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, ENV_MSG), f"Expected '{ENV_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, MODEL_MSG), f"Expected '{MODEL_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, MFR_MSG), f"Expected '{MFR_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CTRL_ID_MSG), f"Expected '{CTRL_ID_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CH_MAP_ID_MSG), f"Expected '{CH_MAP_ID_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, VOD_ID_MSG), f"Expected '{VOD_ID_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, PARTNER_ID_MSG), f"Expected '{PARTNER_ID_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, OSCLASS_MSG), f"Expected '{OSCLASS_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, ACC_ID_MSG), f"Expected '{ACC_ID_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, EXP_MSG), f"Expected '{EXP_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, VER_MSG), f"Expected '{VER_MSG}' in log file."
    except Exception as e:
        print(f"Exception during Validating the parameters in RFC curl Request to xconf: {e}")
        assert False, f"Exception during Validating the parameters in RFC curl Request to xconf: {e}"
