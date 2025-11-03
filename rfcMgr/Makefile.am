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

bin_PROGRAMS = rfcMgr

rfcMgr_SOURCES = rfc_main.cpp rfc_manager.cpp rfc_common.cpp mtlsUtils.cpp rfc_xconf_handler.cpp xconf_handler.cpp rfc_otlp_instrumentation.cpp
rfcMgr_CPPFLAGS = -std=c++17 -Wall -Werror -DRDK_LOGGER -I${top_srcdir}/rfcMgr -I${top_srcdir}/rfcapi/ $(cjson_CFLAGS) $(curl_CFLAGS) $(CFLAGS) -g -O2 -fPIC -Wno-unused-parameter  -I/usr/local/include
AM_LDFLAGS = -L$(PKG_CONFIG_SYSROOT_DIR)/$(libdir)
AM_LDFLAGS += $(cjson_LIBS) $(curl_LIBS)
AM_LDFLAGS += -lrdkloggers -ldwnlutil -lfwutils -lparsejson -lsecure_wrapper -lcurl -L/usr/local/lib -lopentelemetry_exporter_otlp_http -lopentelemetry_exporter_otlp_http_client -lopentelemetry_trace -lopentelemetry_resources -lopentelemetry_common -lopentelemetry_version -lpthread

if IS_IARMBUS_ENABLED
rfcMgr_CPPFLAGS += $(IARMBUS_EVENT_FLAG) -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/rdk/iarmbus
AM_LDFLAGS += -lIARMBus
endif

if ENABLE_RDKB
rfcMgr_CPPFLAGS += $(RDKB_FLAG) $(rbus_CFLAGS) $(RDKBEXTENDER_FLAG)
AM_LDFLAGS += -lrbus
else
AM_LDFLAGS +=  ../rfcapi/.libs/librfcapi.la
endif

if ENABLE_RDKBEXTENDER
rfcMgr_CPPFLAGS += $(RDKBEXTENDER_FLAG)
endif

if IS_LIBRDKCERTSEL_ENABLED
rfcMgr_CPPFLAGS += $(LIBRDKCERTSEL_FLAG)
rfcMgr_CFLAGS = $(LIBRDKCERTSEL_FLAG)
if IS_LIBRDKCONFIG_ENABLED
rfcMgr_CPPFLAGS += $(LIBRDKCONFIG_FLAG)
AM_LDFLAGS += -lRdkCertSelector -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib64 -lrdkconfig
else
AM_LDFLAGS += -lRdkCertSelector
endif
else
if IS_LIBRDKCONFIG_ENABLED
rfcMgr_CPPFLAGS += $(LIBRDKCONFIG_FLAG)
AM_LDFLAGS += -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib64 -lrdkconfig
endif
endif
