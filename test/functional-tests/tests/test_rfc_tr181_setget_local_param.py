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

from rfc_test_helper import *

def test_setLocalParam():
    command_to_check = "tr181 -d -s -n localOnly -v false Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.collectd.Enable"
    result = run_shell_command(command_to_check)
    assert "Set Local Param success!" in result, '"Set Local Param success!" not found in the output'

def test_getLocalParam():
    command_to_check = "tr181 -d -g -n localOnly Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.collectd.Enable"
    result = run_shell_command(command_to_check)
    assert "Param Value :: false" in result, '"Param Value :: false" not found in the output'

