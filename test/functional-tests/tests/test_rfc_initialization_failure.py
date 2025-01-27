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


def test_rfc_init_fail_no_server_url():
    """
    Test RFC initialization failure when the server URL is missing from the properties file.
    
    The test renames the existing properties file, creates a new properties file with an empty URL,
    runs the RFC binary, and checks for specific error messages in the log file.
    """
    if os.path.exists(RFC_PROPS_FILE):
        rename_file(RFC_PROPS_FILE, RFC_PROPS_FILE + "_bak")  # rename the props file

        with open(RFC_PROPS_FILE, "w") as file:  # create new props file with empty URL
            file.write("RFC_CONFIG_SERVER_URL=\n")

    try:
        rfc_run_binary()

        ERROR_MSG1 = "URL not found in the file."
        ERROR_MSG2 = "Xconf Initialization ...!!"
        
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG1), f"Expected '{ERROR_MSG1}' in log file."
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG2), f"Expected '{ERROR_MSG2}' in log file."

    finally:
        os.remove(RFC_PROPS_FILE)
        rename_file(RFC_PROPS_FILE + "_bak", RFC_PROPS_FILE)


def test_rfc_init_fail_no_props_file():
    """
    Test RFC initialization failure when the properties file is missing.
    
    The test renames the existing properties file, runs the RFC binary, and checks for specific
    error messages in the log file.
    """
    if os.path.exists(RFC_PROPS_FILE):
        rename_file(RFC_PROPS_FILE, RFC_PROPS_FILE + "_bak")

    try:
        rfc_run_binary()
        
        ERROR_MSG1 = "Failed to open file."
        ERROR_MSG2 = "Xconf Initialization ...!!"

        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG1)
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG2)

    finally:
        if os.path.exists(RFC_PROPS_FILE + "_bak"):
            rename_file(RFC_PROPS_FILE + "_bak", RFC_PROPS_FILE)

