#!/bin/sh
####################################################################################
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 RDK Management
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

WORKDIR=`pwd`

## Build and install critical dependency
export RFC_ROOT=/usr
export RFC_INSTALL_DIR=${RFC_ROOT}/local
mkdir -p $RFC_INSTALL_DIR
cd $RFC_ROOT

# Build and install common_utilities
cd /home
git clone https://github.com/rdkcentral/common_utilities.git
cd common_utilities
autoreconf -i
./configure --prefix=${RFC_INSTALL_DIR} CFLAGS="-Wno-unused-result -Wno-format-truncation -Wno-error=format-security"
make && make install

# Build and install libSyscallWrapper
cd /home
git clone https://github.com/rdkcentral/libSyscallWrapper.git
cd libSyscallWrapper
autoreconf -i
./configure --prefix=${RFC_INSTALL_DIR}
make && make install

# Build and install WDMP-c
cd /home
git clone https://github.com/xmidt-org/wdmp-c.git
cd wdmp-c
# Modify the wdmp-c.h file
sed -i '/WDMP_ERR_SESSION_IN_PROGRESS/a\    WDMP_ERR_INTERNAL_ERROR,\n    WDMP_ERR_DEFAULT_VALUE,' src/wdmp-c.h
# Build the project
cmake -H. -Bbuild -DBUILD_FOR_DESKTOP=ON -DCMAKE_BUILD_TYPE=Debug
make -C build && make -C build install

# Build and install RFC
cd /home/rfc
autoreconf -i
export cjson_CFLAGS="-I/usr/include/cjson"
export CXXFLAGS="-Wno-format -Wno-unused-variable"
./configure --prefix=${RFC_INSTALL_DIR} --enable-rfctool=yes --enable-tr181set=yes

# rfcapi/
cd rfcapi
cp /usr/include/cjson/cJSON.h  ./
cp /usr/local/include/wdmp-c/wdmp-c.h ./
make && make install

# tr181api/
cd ../tr181api
make && make install

# utils/
cd ../utils
make && make install

# rfcMgr/
cd ../rfcMgr
export curl_LIBS=" -lcurl"
make && make install

