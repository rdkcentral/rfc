##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2016 RDK Management
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
SUBDIRS = 

if ENABLE_TR181SET_APP
lib_LTLIBRARIES = libtr181api.la
libtr181api_la_SOURCES = tr181api.cpp ../rfcMgr/rfc_otlp_instrumentation.cpp
if ENABLE_RDKB
libtr181api_la_CPPFLAGS = "-std=c++17" -DLINUX -fPIC -g -O2 -Wall -I${top_srcdir}/rfcapi/ $(RDKB_FLAG) -DUSE_IARMBUS -DUSE_TR69HOSTIF
else
libtr181api_la_CPPFLAGS = "-std=c++17" -DLINUX -fPIC -g -O2 -Wall -I=/usr/include/wdmp-c -I${top_srcdir}/rfcapi/ $(IARMBUS_EVENT_FLAG) -DUSE_IARMBUS -DUSE_TR69HOSTIF
endif
libtr181api_la_LIBADD = -lrdkloggers ../rfcapi/.libs/librfcapi.la -lpthread

libtr181api_la_includedir = $(includedir)
libtr181api_la_include_HEADERS = tr181api.h
endif
