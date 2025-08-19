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
import urllib.parse



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


def test_rfcMgr_unresolved_xconf_url() -> None:
    """
    Tests the behavior of the RFC manager when an unresolved XCONF URL is set.

    This function modifies the RFC properties file to use an unresolved URL,
    runs a binary that is expected to fail due to hostname resolution issues,
    and checks for specific error messages in the log file.

    Returns:
        None
    """
    modify_rfc_url(RFC_XCONF_UNRESOLVED_URL) # update an unresolved URL to props file
    
    try:
        rfc_run_binary()
        
        ERROR_MSG1 = "Couldn't resolve host name"
        ERROR_MSG2 = "cURL Return : 6 HTTP Code : 0"
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG1), f"Expected '{ERROR_MSG1}' in log file."
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG2), f"Expected '{ERROR_MSG2}' in log file."
    
    finally:
        modify_rfc_url(RFC_XCONF_URL)
    
  
def test_rfcMgr_xconf_404_communication() -> None:
    """
    Tests the behavior of the RFC manager when a 404 XCONF URL is set.

    This function modifies the RFC properties file to use a URL that is expected
    to return a 404 status code, runs a binary, and checks for a specific error 
    message in the log file indicating a 404 error.

    Returns:
        None
    """
    modify_rfc_url(RFC_XCONF_404_URL) # update an 404 URL to props file
    
    try:
        rfc_run_binary()
    
        ERROR_MSG1 = "cURL Return : 0 HTTP Code : 404"
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG1), f"Expected '{ERROR_MSG1}' in log file."
    
    finally:
        modify_rfc_url(RFC_XCONF_URL)


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

    pre_rfc_run_tr181_File = os.path.exists(TR181_INI_FILE)  # returns false as the file would not exist
    pre_rfc_run_rfc_version = get_rfc_old_FW()
    device_fw_version = get_FWversion()

    try:
        rfc_run_binary()
        post_rfc_run_tr181_File = os.path.exists(TR181_INI_FILE)  # will be true after rfcMgr run
        post_rfc_run_rfc_version = get_rfc_old_FW()
        rfc_key1, rfc_value1 = get_tr181_file_key_value()
        ERROR_MSG1 = f"COMPLETED RFC PASS"
        
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG1), f"Expected '{ERROR_MSG1}' in log file."
        assert (post_rfc_run_tr181_File != pre_rfc_run_tr181_File), f"'{TR181_INI_FILE}' creation success on rfc run."
        assert (pre_rfc_run_rfc_version != post_rfc_run_rfc_version) and (device_fw_version == post_rfc_run_rfc_version), f"'{device_fw_version}' updated in '{TR181_INI_FILE}'"
        assert (rfc_key1 == TEST_RFC_PARAM_KEY1) and (rfc_value1 == TEST_RFC_PARAM_VAL1), f"'{TEST_RFC_PARAM_KEY1}' key is set with '{TEST_RFC_PARAM_VAL1} in {TR181_INI_FILE}'"

        SEARCH_MSG = f"Encoding is enabled plain URL: "
        plain_url = search_log_file(RFC_LOG_FILE,SEARCH_MSG)
        SEARCH_MSG_ENCODED = f"Xconf Request : "
        encoded_url = search_log_file(RFC_LOG_FILE,SEARCH_MSG_ENCODED)
        try:
            url = plain_url.split(SEARCH_MSG,1)[1]
            coded_url = encoded_url.split(SEARCH_MSG_ENCODED,1)[1]
            decoded_url = urllib.parse.unquote(coded_url)
            assert coded_url == decoded_url



    finally:
        if os.path.exists(RFC_OLD_FW_FILE + "_bak"):
            rename_file(RFC_OLD_FW_FILE + "_bak", RFC_OLD_FW_FILE)

