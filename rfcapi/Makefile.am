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
lib_LTLIBRARIES = librfcapi.la
librfcapi_la_SOURCES = rfcapi.cpp ../rfcMgr/rfc_otlp_instrumentation.cpp
librfcapi_la_includedir = $(includedir)
librfcapi_la_include_HEADERS = rfcapi.h rfc_otlp_instrumentation.h
if ENABLE_RDKC
librfcapi_la_CPPFLAGS = -std=c++17 -DLINUX -fPIC -g -O2 -Wall -DRDKC -I${top_srcdir}/../rdklogger/include/ -I${top_srcdir}/rfcMgr -DUSE_IARMBUS -DUSE_TR69HOSTIF
librfcapi_la_LIBADD = -L${top_srcdir}/../rdklogger/src/.libs -lrdkloggers -lopentelemetry_exporter_otlp_http -lopentelemetry_exporter_otlp_http_client -lopentelemetry_trace -lopentelemetry_resources -lopentelemetry_common -lopentelemetry_version -lpthread
else
librfcapi_la_CPPFLAGS = -std=c++17 -DLINUX -fPIC -g -O2 -Wall -I=/usr/include/cjson -I=/usr/include/wdmp-c $(IARMBUS_EVENT_FLAG) -I${top_srcdir}/rfcMgr -DUSE_IARMBUS -DUSE_TR69HOSTIF
librfcapi_la_LIBADD = -lcurl -lcjson -lrdkloggers -L/usr/local/lib -lopentelemetry_exporter_otlp_http -lopentelemetry_exporter_otlp_http_client -lopentelemetry_trace -lopentelemetry_resources -lopentelemetry_common -lopentelemetry_version -lpthread
endif
endif
