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

def test_static_cert_selector():
    """
    Test the communication between RFC Manager and XCONF.

    This function checks the creation of the TR181 INI file,
    verifies the firmware version update, and checks the key-value pair in the TR181 INI file.
    """
    if os.path.exists(TR181_INI_FILE):
        os.remove(TR181_INI_FILE)

    if os.path.exists(RFC_OLD_FW_FILE):
        rename_file(RFC_OLD_FW_FILE, RFC_OLD_FW_FILE + "_bak")

    if os.path.exists("/opt/certs/client.p12"):
        os.remove("/opt/certs/client.p12")

    try:
        rfc_run_binary()
        CERT_INIT_MSG = f"Initializing cert selector"
        CERT_INIT_MSG_STATUS = f"Cert selector initialization successful"
        CERT_STATUS_MSG = f"MTLS dynamic/static cert success. cert=/etc/ssl/certs/client.pem, type=STATIC"
        MTLS_STATUS_MSG = f"MTLS is enable"
        HTTP_CODE_MSG = f"RFC Xconf Connection Response cURL Return : 0 HTTP Code : 200"
        CURL_ERR_MSG = f"curl_easy_strerror =No error"

        assert grep_log_file(RFC_LOG_FILE, CERT_INIT_MSG), f"Expected '{CERT_INIT_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CERT_INIT_MSG_STATUS), f"Expected '{CERT_INIT_MSG_STATUS}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CERT_STATUS_MSG), f"Expected '{CERT_STATUS_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, MTLS_STATUS_MSG), f"Expected '{MTLS_STATUS_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, HTTP_CODE_MSG), f"Expected '{HTTP_CODE_MSG}' in log file."
        assert grep_log_file(RFC_LOG_FILE, CURL_ERR_MSG), f"Expected '{CURL_ERR_MSG}' in log file."
    except Exception as e:
        print(f"Exception during Dynamic/Static Certificate method: {e}")
        assert False, f"Exception during Dynamic/Static Certificate method: {e}"


