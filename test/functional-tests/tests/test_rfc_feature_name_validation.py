####################################################################################
# If not stated otherwise in this file or this component's Licenses file the
# following copyright and licenses apply:
#
# Copyright 2026 RDK Management
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

import glob
import os

from rfc_test_helper import *


def test_rfcmanager_validates_xconf_feature_names():

    modify_rfc_url(RFC_XCONF_URL)

    try:
        rfc_run_binary()

        INVALID_FEATURE_NAME = "Invalid RFC feature name received from Xconf"
        PARSE_MSG = "JSON Parsing Failed for Feature Name"
        assert grep_log_file(RFC_LOG_FILE, INVALID_FEATURE_NAME), f"Expected '{INVALID_FEATURE_NAME}' in log file."

        parse_failure_count = 0
        for log_path in glob.glob(f"{RFC_LOG_FILE}*"):
            with open(log_path, encoding="utf-8", errors="replace") as log_file:
                parse_failure_count += sum(PARSE_MSG in line for line in log_file)
        assert parse_failure_count == 2, f"Expected two '{PARSE_MSG}' entries, found {parse_failure_count}."

        sentinel_param = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.RDKRemoteDebugger.Enable"
        valid_param = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Bootstrap.OsClass"
        assert grep_log_file(RFC_LOG_FILE, f"Feature Name [{valid_param}] Value[default]"), "Valid configData was not processed."
        assert not grep_log_file(RFC_LOG_FILE, f"Feature Name [{sentinel_param}]"), "Rejected feature's configData reached parameter application."

        with open(TR181_INI_FILE, encoding="utf-8") as param_file:
            applied_params = param_file.read()
        assert f"TR181: {valid_param} default" in applied_params, "Valid parameter is missing from the applied-parameter list."
        assert sentinel_param not in applied_params, "Rejected feature's parameter appears in the applied-parameter list."
    except Exception as e:
        print(f"Exception during feature name validation: {e}")
        assert False, f"Exception during feature name validation: {e}"
