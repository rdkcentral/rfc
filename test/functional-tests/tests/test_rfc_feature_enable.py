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


def get_rfc_old_FW() -> str | None:
    """
    Retrieve the old RFC firmware version from the specified file.

    Returns:
        str | None: The RFC firmware version if it exists, otherwise None.
    """
    if os.path.exists(RFC_OLD_FW_FILE):
        try:
            with open(RFC_OLD_FW_FILE, "r") as rfc_version_file:
                rfc_image_name = rfc_version_file.read().strip()
                return rfc_image_name if rfc_image_name else None
        except Exception as e:
            return None
    return None


def get_tr181_file_key_value() -> tuple[str | None, str | None]:
    """
    Extract the key and value from the TR181 INI file.

    Returns:
        tuple[str | None, str | None]: A tuple containing the key and value if found, otherwise (None, None).
    """
    if os.path.exists(TR181_INI_FILE):
        try:
            with open(TR181_INI_FILE, "r") as tr181_file:
                for line in tr181_file:
                    parts = line.split(None, 2)
                    if len(parts) >= 2:
                        return parts[1].strip(), parts[2].strip()
                return None, None
        except Exception as e:
            return None, None
    return None, None


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


def test_rfcMgr_xconf_304_communication() -> None:
    """
    Tests the behavior of the RFC manager when a 304 XCONF URL is set.

    This function modifies the RFC properties file to use a URL that is expected
    to return a 304 status code, runs a binary, and checks for a specific error 
    message in the log file indicating a 304 error.

    Returns:
        None
    """
    modify_rfc_url(RFC_XCONF_304_URL) # update an 304 URL to props file    
    try:
        rfc_run_binary()
    
        ERROR_MSG1 = "cURL Return : 0 HTTP Code : 304"
        RFC_FEATURE_STATUS_MSG = "[Features Enabled]-[ACTIVE]:"
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG1), f"Expected '{ERROR_MSG1}' in log file."
        assert grep_log_file(RFC_LOG_FILE, RFC_FEATURE_STATUS_MSG), f"Expected '{RFC_FEATURE_STATUS_MSG}' in log file."
    
    finally:
        modify_rfc_url(RFC_XCONF_URL)


