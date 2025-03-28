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

bin_PROGRAMS =
if ENABLE_TR181SET_APP
bin_PROGRAMS += tr181
endif

if ENABLE_RFCTOOL_APP
bin_PROGRAMS += rfctool
endif

if IS_IARMBUS_ENABLED
tr181_SOURCES = tr181utils.cpp trsetutils.cpp
tr181_LDADD = ../rfcapi/.libs/librfcapi.la ../tr181api/.libs/libtr181api.la
tr181_CPPFLAGS = -std=c++0x -Wall -I${top_srcdir}/rfcapi/ -I${top_srcdir}/tr181api/ -I=/usr/include/wdmp-c $(IARMBUS_EVENT_FLAG) $(TR69HOSTIF_FLAG)

tr181_LDADD += -lIARMBus
tr181_CPPFLAGS += -I$(PKG_CONFIG_SYSROOT_DIR)${includedir}/rdk/iarmbus

rfctool_SOURCES = jsonhandler.cpp
rfctool_LDADD = ${cjson_LIBS}
rfctool_CPPFLAGS = -std=c++0x -Wall ${cjson_CFLAGS}
endif
