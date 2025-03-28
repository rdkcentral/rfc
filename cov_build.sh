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
export RFC_INSTALL_DIR=${RFC_ROOT}
mkdir -p $RFC_INSTALL_DIR

cd $RFC_ROOT
rm -rf common_utilities
git clone https://github.com/rdkcentral/common_utilities.git -b develop
cd common_utilities
sed -i 's/-Werror //g' utils/Makefile.am
autoreconf -i
./configure
make && make install
cp /usr/common_utilities/lib/* /usr/lib/
cp /usr/common_utilities/utils/common_device_api.h $WORKDIR/rfcMgr
cd $WORKDIR

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

