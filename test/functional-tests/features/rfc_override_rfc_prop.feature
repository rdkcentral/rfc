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

Feature: Validate override functionality for rfc.properties file

  Scenario: Override function correctly overrides /etc/rfc.properties using /opt/rfc.properties
    Given the following /opt/rfc.properties file exist:
      | path                 | key                      | value                                                           |
      | /etc/rfc.properties  | RFC_CONFIG_SERVER_URL    | https://mockxconf/featureControl/getSettings                    |
      | /opt/rfc.properties  | RFC_CONFIG_SERVER_URL    | https://mockxconf_opt_rfc_properties/featureControl/getSettings |
      | /opt/rfc.properties  | RFC_CONFIG_SERVER_URL_EU | https://mockxconf_opt_rfc_properties/featureControl/getSettings | 
    When the RFC binary is run
    Then the rfc.properties should take from the /opt/rfc.properties path
    And a RFC file path "Found Persistent file /opt/rfc.properties" should be logged
    And a XCONF_SERVER_URL value "_xconf_server_url: [https://mockxconf_opt_rfc_properties/featureControl/getSettings]" should be logged
    And an override message "Setting URL from local override to https://mockxconf_opt_rfc_properties/featureControl/getSettings" should be logged

