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
    if not os.path.exists(RFC_PROPERTIES_PERSISTENCE_FILE):
        with open(RFC_PROPERTIES_PERSISTENCE_FILE, "w") as rfc_props:
            rfc_props.write(f'RFC_CONFIG_SERVER_URL={new_url}\n')
        return None
    with open(RFC_PROPERTIES_PERSISTENCE_FILE, "r+") as rfc_props:
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


def test_Set_DbgServices_value():
    command_to_check = "tr181 -d -s -t bool -v true Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Identity.DbgServices.Enable"
    result = run_shell_command(command_to_check)
    assert "Set operation success" in result, '"Set operation success" not found in the output'

def test_rfc_override_rfc_prop():
    """
    Test the communication between RFC Manager and XCONF.

    This function checks the creation of the TR181 INI file,
    verifies the firmware version update, and checks the key-value pair in the TR181 INI file.
    """
    if os.path.exists(TR181_INI_FILE):
        os.remove(TR181_INI_FILE)

    if os.path.exists(RFC_OLD_FW_FILE):
        rename_file(RFC_OLD_FW_FILE, RFC_OLD_FW_FILE + "_bak")

    modify_rfc_url(RFC_XCONF_OVERRIDE_URL) # update an unresolved URL to props file

    try:
        rfc_run_binary()
        RFC_FILE_PATH_MSG = f"Found Persistent file /opt/rfc.properties"
        XCONF_URL_MSG = f"_xconf_server_url: [https://mockxconf_opt_rfc_properties/featureControl/getSettings]"
        XCONF_REQ_MSG = f" Xconf Request : [https://mockxconf_opt_rfc_properties/featureControl/getSettings"
        OVERRIDE_MSG= f"Setting URL from local override to https://mockxconf_opt_rfc_properties/featureControl/getSettings"

        assert grep_log_file(RFC_LOG_FILE, RFC_FILE_PATH_MSG), f"Expected '{RFC_FILE_PATH_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, XCONF_URL_MSG), f"Expected '{XCONF_URL_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, XCONF_REQ_MSG), f"Expected '{XCONF_REQ_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, OVERRIDE_MSG), f"Expected '{OVERRIDE_MSG}' in log file."
    except Exception as e:
        print(f"Exception during Validate the Override function for rfc.properties file: {e}")
        assert False, f"Exception during Validate the Override function for rfc.properties file: {e}" 
