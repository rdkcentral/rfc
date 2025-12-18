#!/bin/sh
####################################################################################
# If not stated otherwise in this file or this component's Licenses.txt file the
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

RESULT_DIR="/tmp/rfc_test_report"
mkdir -p "$RESULT_DIR"

echo "ENABLE_MAINTENANCE=false" >> /etc/device.properties

rm -rf /opt/rfc.properties
# Run L2 Test cases
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_unknown_accountid.json test/functional-tests/tests/test_rfc_unknown_accountid.py

cat /opt/logs/rfcscript.txt.1

cat /opt/logs/rfcscript.txt.0


