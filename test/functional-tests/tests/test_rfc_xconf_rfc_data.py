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

def test_RFC_dir_exists():
    """Verify /opt/secure/RFC exists and is a directory"""
    path = "/opt/secure/RFC"
    assert os.path.exists(path), f"{path} does not exist"
    assert os.path.isdir(path), f"{path} is not a directory"

def test_expected_files_present():
    path = "/opt/secure/RFC"
    expected_files = ["tr181store.ini", "tr181localstore.ini", "tr181.list", "rfcVariable.ini", "rfcFeature.list"]
    actual_files = os.listdir(path)

    for f in expected_files:
        assert f in actual_files, f"Missing expected file: {f}"
