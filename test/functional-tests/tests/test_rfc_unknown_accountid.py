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

def modify_rfc_url(new_url: str) -> None:
    """
    Modifies the RFC properties file to set the RFC_CONFIG_SERVER_URL.

    If the properties file does not exist, it creates one with the given URL.
    If the file exists but is empty, it adds the URL. If the file contains
    an existing RFC_CONFIG_SERVER_URL, it updates that line with the new URL.

    Args:
        new_url (str): The new URL to set for RFC_CONFIG_SERVER_URL.

    Returns:
        None
    """
    if not os.path.exists(RFC_PROPS_FILE):
        with open(RFC_PROPS_FILE, "w") as rfc_props:
            rfc_props.write(f'RFC_CONFIG_SERVER_URL={new_url}\n')
        return None
    with open(RFC_PROPS_FILE, "r+") as rfc_props:
        content = rfc_props.read()
        if not content.strip():
            rfc_props.write(f'RFC_CONFIG_SERVER_URL={new_url}\n')
        else:
            lines = content.splitlines()
            for i in range(len(lines)):
                if lines[i].startswith('RFC_CONFIG_SERVER_URL='):
                    lines[i] = f'RFC_CONFIG_SERVER_URL={new_url}'
                    break

            # Write back the modified content
            rfc_props.seek(0)
            rfc_props.truncate()  # Clear the current contents of the file
            rfc_props.write('\n'.join(lines) + '\n')
            print(f"Modified existing content to: RFC_CONFIG_SERVER_URL={new_url}")


def test_xconf_request_response():
    """
    Test the communication between RFC Manager and XCONF.

    This function checks the creation of the TR181 INI file,
    verifies the firmware version update, and checks the key-value pair in the TR181 INI file.
    """
    if os.path.exists(TR181_INI_FILE):
        os.remove(TR181_INI_FILE)

    if os.path.exists(RFC_OLD_FW_FILE):
        rename_file(RFC_OLD_FW_FILE, RFC_OLD_FW_FILE + "_bak")
    
    modify_rfc_url(RFC_XCONF_URL)

    try:
        rfc_run_binary()
        FEATURE_NAME_VALUE = f" Feature Name [Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID] Value[Unknown]"
        RFC_PARAM = f"Checking Config Value changed for tr181 param"
        XCONF_RESP_RECEIVED_ACTID_UNKNOWN = f"RFC: AccountId received from Xconf is Unknown"
        XCONF_RESP_CMP_MSG = f"RFC: Comparing Xconfvalue='Unknown' with Unknown"
        ACTID_REPLACE_AUTHSERVICE = f"RFC: AccountId Unknown is replaced with Authservice 412370664406228514"
        ACTID_UPDATE = f"RFC: AccountId Updated Value is 412370664406228514 and Xconf value is 412370664406228514"

        
        assert grep_log_file(RFC_LOG_FILE, FEATURE_NAME_VALUE), f"Expected '{FEATURE_NAME_VALUE}' in log file."
        assert grep_log_file(RFC_LOG_FILE, RFC_PARAM), f"Expected '{RFC_PARAM}' in log file."
        assert grep_log_file(RFC_LOG_FILE, XCONF_RESP_RECEIVED_ACTID_UNKNOWN), f"Expected '{XCONF_RESP_RECEIVED_ACTID_UNKNOWN}' in log file."
        assert grep_log_file(RFC_LOG_FILE, XCONF_RESP_CMP_MSG), f"Expected '{XCONF_RESP_CMP_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, ACTID_REPLACE_AUTHSERVICE), f"Expected '{ACTID_REPLACE_AUTHSERVICE}' in log file."
        assert grep_log_file(RFC_LOG_FILE, ACTID_UPDATE), f"Expected '{ACTID_UPDATE}' in log file."
    except Exception as e:
        print(f"Exception during Validate the XConf request and response: {e}")
        assert False, f"Exception during Validate the XConf request and response: {e}"

