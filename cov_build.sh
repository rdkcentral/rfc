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

if [ "${RFC_L2_COVERAGE:-0}" = "1" ]; then
    export CFLAGS="-g -O0 --coverage ${CFLAGS:-}"
    export CXXFLAGS="-g -O0 --coverage -Wno-format -Wno-unused-variable ${CXXFLAGS:-}"
    export LDFLAGS="-lgcov --coverage ${LDFLAGS:-}"
fi

WORKDIR=`pwd`

## Build and install critical dependency
export RFC_ROOT=/usr
export RFC_INSTALL_DIR=${RFC_ROOT}
mkdir -p $RFC_INSTALL_DIR

autoreconf -i
export cjson_CFLAGS="-I/usr/include/cjson"
if [ "${RFC_L2_COVERAGE:-0}" != "1" ]; then
    export CXXFLAGS="-Wno-format -Wno-unused-variable"
fi
./configure --prefix=${RFC_INSTALL_DIR} --enable-rfctool=yes --enable-tr181set=yes --enable-rdkcertselector=yes --enable-mountutils=yes

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
cd $RFC_ROOT
git clone https://github.com/rdkcentral/rdk-cert-config.git
cp rdk-cert-config/RdkConfigApi/include/* /usr/local/include

cd $WORKDIR 

# rfcapi/
cd rfcapi
cp /usr/include/cjson/cJSON.h  ./
cp /usr/local/include/wdmp-c/wdmp-c.h ./
make clean >/dev/null 2>&1 || true
make CXXFLAGS="$CXXFLAGS" && make install

# tr181api/
cd ../tr181api
make clean >/dev/null 2>&1 || true
make CXXFLAGS="$CXXFLAGS" && make install

# utils/
cd ../utils
make clean >/dev/null 2>&1 || true
make CXXFLAGS="$CXXFLAGS" && make install

# rfcMgr/
cd ../rfcMgr
make clean >/dev/null 2>&1 || true
export curl_LIBS=" -lcurl"
export LDFLAGS="${LDFLAGS:-} -L/usr/local/lib"
export LIBS="${LIBS:-} -lrdkconfig"
make CXXFLAGS="$CXXFLAGS" LDFLAGS="$LDFLAGS" && make install
