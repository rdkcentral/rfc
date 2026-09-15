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


def test_direct_block_is_active_within_24_hours():
    remove_file(RFC_DIRECT_BLOCK_FILE)
    write_direct_block_marker()

    try:
        # RFC_LOG_FILE can rotate to LOG_FILE mid-run once a size threshold is hit,
        # so clear both first to keep grep_log_file scoped to just this run.
        remove_file(RFC_LOG_FILE)
        remove_file(LOG_FILE)
        rfc_run_binary()
        assert grep_log_file(RFC_LOG_FILE, "Last direct failed blocking is still valid")
        assert not grep_log_file(RFC_LOG_FILE, "Xconf Request :")
    finally:
        remove_file(RFC_DIRECT_BLOCK_FILE)


def test_direct_block_expires_after_24_hours():
    remove_file(RFC_DIRECT_BLOCK_FILE)
    write_direct_block_marker(age_seconds=86401)

    try:
        remove_file(RFC_LOG_FILE)
        remove_file(LOG_FILE)
        rfc_run_binary()
        assert not os.path.exists(RFC_DIRECT_BLOCK_FILE)
        assert grep_log_file(RFC_LOG_FILE, "Last direct failed blocking has expired")
        assert grep_log_file(RFC_LOG_FILE, "Xconf Request :")
    finally:
        remove_file(RFC_DIRECT_BLOCK_FILE)
