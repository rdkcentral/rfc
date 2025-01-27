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
from typing import NoReturn


def test_rfcMgr_device_offline_dns_file() -> NoReturn:
    """
    Test RFC manager's behavior when the DNS file is not present.

    The test renames the existing DNS file, runs the RFC binary, and checks for specific
    error messages in the log file.
    """
    if os.path.exists(RFC_DNS_FILE):
        rename_file(RFC_DNS_FILE, RFC_DNS_FILE + "_bak")

    try:
        rfc_run_binary()

        ERROR_MSG1 = f"dns resolve file:{RFC_DNS_FILE} not present"
        ERROR_MSG2 = "RFC:Device is Offline"

        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG1), f"Expected '{ERROR_MSG1}' in log file."
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG2), f"Expected '{ERROR_MSG2}' in log file."

    finally:
        if os.path.exists(RFC_DNS_FILE + "_bak"):
            rename_file(RFC_DNS_FILE + "_bak", RFC_DNS_FILE)

