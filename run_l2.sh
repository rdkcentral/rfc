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

cp ./rfc.properties /opt/rfc.properties
cp /opt/certs/client.pem /etc/ssl/certs/client.pem
cp ./rfcMgr/gtest/mocks/tr181store.ini /opt/secure/RFC/tr181store.ini

rbuscli set Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Control.ConfigSetTime uint32 1763118860

# Run L2 Test cases
pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_single_instance_run.json test/functional-tests/tests/test_rfc_single_instance_run.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_device_offline.json test/functional-tests/tests/test_rfc_device_offline_status.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_init_failure.json test/functional-tests/tests/test_rfc_initialization_failure.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_xconf_communication_success.json test/functional-tests/tests/test_rfc_xconf_communication.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_setget_param.json test/functional-tests/tests/test_rfc_setget_param.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_tr181_setget_local_param.json test/functional-tests/tests/test_rfc_tr181_setget_local_param.py

# The cert selector test cases  are commented for now. Once the code changes are moved to open source, it will be enabled.
#pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_dynamic_static_cert_selector.json test/functional-tests/tests/test_rfc_dynamic_static_cert_selector.py

#pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_static_cert_selector.json test/functional-tests/tests/test_rfc_static_cert_selector.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_xconf_rfc_data.json test/functional-tests/tests/test_rfc_xconf_rfc_data.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_xconf_request_params.json test/functional-tests/tests/test_rfc_xconf_request_params.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_valid_accountid.json test/functional-tests/tests/test_rfc_valid_accountid.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_feature_enable.json test/functional-tests/tests/test_rfc_feature_enable.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_configsethash_time.json test/functional-tests/tests/test_rfc_xconf_configsethash_time.py

echo "ENABLE_MAINTENANCE=true" >> /etc/device.properties

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_xconf_reboot.json test/functional-tests/tests/test_rfc_xconf_reboot.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_override_rfc_prop.json test/functional-tests/tests/test_rfc_override_rfc_prop.py

pytest --json-report --json-report-summary --json-report-file $RESULT_DIR/rfc_rfc_webpa.json test/functional-tests/tests/test_rfc_webpa.py

