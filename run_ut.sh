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

cp ./rfcMgr/gtest/mocks/rfc.properties /etc/rfc.properties
cp ./rfcMgr/gtest/mocks/rfcdefaults.ini /tmp/rfcdefaults.ini

mkdir /opt/secure
mkdir /opt/secure/RFC
cp ./rfcMgr/gtest/mocks/tr181store.ini /opt/secure/RFC/tr181store.ini


export TOP_DIR=`pwd`
cd ./rfcMgr/

rm ./gtest/rfcMgr_gtest

automake --add-missing
autoreconf --install


./configure

make clean

echo "TOP_DIR = $TOP_DIR"

echo "**** Compiling rfcMgr gtest ****"
cd $TOP_DIR/rfcMgr/gtest
make
./rfcMgr_gtest
echo "********************"

lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.filtered.info
genhtml coverage.filtered.info --output-directory out
