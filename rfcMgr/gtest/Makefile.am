##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2018 RDK Management
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
##########################################################################

AUTOMAKE_OPTIONS = subdir-objects
# Define the program name and the source files
bin_PROGRAMS = rfcMgr_gtest

rfcMgr_gtest_SOURCES = $(TOP_DIR)/tr181api/tr181api.cpp $(TOP_DIR)/rfcMgr/rfc_manager.cpp $(TOP_DIR)/rfcMgr/rfc_common.cpp $(TOP_DIR)/rfcMgr/mtlsUtils.cpp $(TOP_DIR)/rfcMgr/rfc_xconf_handler.cpp $(TOP_DIR)/rfcMgr/xconf_handler.cpp $(TOP_DIR)/rfcapi/rfcapi.cpp $(TOP_DIR)/rfcMgr/gtest/gtest_main.cpp $(TOP_DIR)/rfcMgr/gtest/mocks/secure_wrapper.c $(TOP_DIR)/rfcMgr/gtest/mocks/common_device_api.c $(TOP_DIR)/rfcMgr/gtest/mocks/curl_debug.c $(TOP_DIR)/rfcMgr/gtest/mocks/downloadUtil.c $(TOP_DIR)/rfcMgr/gtest/mocks/json_parse.c $(TOP_DIR)/rfcMgr/gtest/mocks/rdk_fwdl_utils.c $(TOP_DIR)/rfcMgr/gtest/mocks/system_utils.c $(TOP_DIR)/rfcMgr/gtest/mocks/urlHelper.c $(TOP_DIR)/rfcMgr/gtest/mocks/rfcMgr_stubs.cpp $(TOP_DIR)/rfcMgr/gtest/mocks/mock_curl.cpp


AM_CPPFLAGS = -std=c++17 -DGTEST_ENABLE -DRDK_LOGGER -DUSE_IARMBUS -I${TOP_DIR}/rfcMgr/ -I${TOP_DIR}/rfcapi/ -I/usr/include -I/usr/include/cjson -I${TOP_DIR}/rfcMgr/gtest/mocks/ -I${TOP_DIR}/rfcMgr/gtest/mocks/wdmp-c/ -I$(TOP_DIR)/tr181api/
AM_CXXFLAGS = -frtti -fprofile-arcs -ftest-coverage

AM_LDFLAGS =  -lgtest -lgtest_main -lgmock_main -lgmock -lgcov -lcjson -lcurl
