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
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([rfctools], [1.0], [josekutty@tataelxsi.co.in])
AC_CONFIG_SRCDIR([utils/jsonhandler.cpp])
AC_CONFIG_SRCDIR([utils/trsetutils.cpp])
AC_CONFIG_SRCDIR([utils/tr181utils.cpp])
AC_CONFIG_HEADERS([utils/config.h])

AM_INIT_AUTOMAKE([foreign])
LT_INIT
LT_LANG([C++])
AC_PROG_CXX
AC_PROG_CC

GTEST_ENABLE_FLAG = ""
IS_LIBRDKCERTSEL_ENABLED=""

AC_ARG_ENABLE([gtestapp],
             AS_HELP_STRING([--enable-gtestapp],[enable Gtest support (default is no)]),
             [
               case "${enableval}" in
                yes) GTEST_SUPPORT_ENABLED=true
                     GTEST_ENABLE_FLAG="-DGTEST_ENABLE"
                     m4_if(m4_sysval,[0],[AC_CONFIG_FILES([test/Makefile])]);;
                no) GTEST_SUPPORT_ENABLED=false AC_MSG_ERROR([Gtest support is disabled]);;
                 *) AC_MSG_ERROR([bad value ${enableval} for --enable-gtestapp ]);;
               esac
             ],
             [echo "Gtestapp is disabled"])
AM_CONDITIONAL([WITH_GTEST_SUPPORT], [test x$GTEST_SUPPORT_ENABLED = xtrue])

# Checks for libraries.
PKG_CHECK_MODULES([cjson],[libcjson])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])

#check what all tools are enabled. rfctool by default
ENABLE_RFCTOOL_APP=true
#only for yocto builds.
ENABLE_TR181SET_APP=" "

AC_ARG_ENABLE([rfctool],
              AS_HELP_STRING([--enable-rfctool],[enable rfctool app (default is yes)]),
              [
                case "${enableval}" in
                 yes) ENABLE_RFCTOOL_APP=true;;
                  no) ENABLE_RFCTOOL_APP=" ";
                      AC_MSG_ERROR([RFC tool app is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-rfctool]) ;;
                esac
              ],
              [echo "rfctool app is disabled"])
AM_CONDITIONAL([ENABLE_RFCTOOL_APP], [test x$ENABLE_RFCTOOL_APP = xtrue])

AC_ARG_ENABLE([rdkcertselector],
        AS_HELP_STRING([--enable-rdkcertselector],[enables rdkcertselector replacement (default is no)]),
        [
          case "${enableval}" in
           yes) IS_LIBRDKCERTSEL_ENABLED=true
                LIBRDKCERTSEL_FLAG=" -DLIBRDKCERTSELECTOR ";;
           no)  IS_LIBRDKCERTSEL_ENABLED=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-rdkcertselector]) ;;
           esac
           ],
        [echo "rdkcertselector is disabled"])
AM_CONDITIONAL([IS_LIBRDKCERTSEL_ENABLED], [test x$IS_LIBRDKCERTSEL_ENABLED = xtrue])
AC_SUBST(LIBRDKCERTSEL_FLAG)

AC_ARG_ENABLE([tr181set],
              AS_HELP_STRING([--enable-tr181set],[enable tr181 client app (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_TR181SET_APP=true;;
                  no) AC_MSG_ERROR([TR181 client app is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-tr181set]) ;;
                esac
              ],
              [echo "tr181 client app is disabled"])

AM_CONDITIONAL([ENABLE_TR181SET_APP], [test x$ENABLE_TR181SET_APP = xtrue])

AC_ARG_ENABLE([iarmbus],
              AS_HELP_STRING([--enable-iarmbus], [enable IARMBus support (default is no)]),
              [
                case "${enableval}" in
                  yes) IS_IARMBUS_ENABLED=true
                       IARMBUS_EVENT_FLAG=" -DUSE_IARMBUS ";;
                  no)  IS_IARMBUS_ENABLED=false
                       AC_MSG_ERROR([IARMBus support is disabled]);;
                  *) AC_MSG_ERROR([bad value ${enableval} for --enable-iarmbus]);;
                esac
              ],
              [echo "IARMBus support is disabled"])
AM_CONDITIONAL([IS_IARMBUS_ENABLED], [test x$IS_IARMBUS_ENABLED = xtrue])
AC_SUBST(IARMBUS_EVENT_FLAG)

AC_ARG_ENABLE([tr69hostif],
          AS_HELP_STRING([--enable-tr69hostif], [enable TR69 host interface support (default is no)]),
          [
                case "${enableval}" in
                  yes) IS_TR69HOSTIF_ENABLED=true
                       TR69HOSTIF_FLAG=" -DUSE_TR69HOSTIF ";;
                  no)  IS_TR69HOSRIF_ENABLED=false
                       AC_MSG_ERROR([TR69 host interface support is disabled]);;
                  *) AC_MSG_ERROR([bad value ${enableval} for --enable-tr69hostif]);;
                esac
              ],
          [echo "TR69 host interface support is disabled"])
AM_CONDITIONAL([IS_TR69HOSRIF_ENABLED], [test x$IS_TR69HOSRIF_ENABLED = xtrue])
AC_SUBST(TR69HOSTIF_FLAG)

AC_ARG_ENABLE([rdkc],
              AS_HELP_STRING([--enable-rdkc],[enable rdkc (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_RDKC=true;;
                  no) AC_MSG_ERROR([RDKC is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-rdkc]) ;;
                esac
              ],
              [echo "rdkc is disabled"])

AM_CONDITIONAL([ENABLE_RDKC], [test x$ENABLE_RDKC = xtrue])

AC_ARG_ENABLE([rdkb],
              AS_HELP_STRING([--enable-rdkb],[enable rdkb (default is no)]),
              [
                case "${enableval}" in
                 yes) ENABLE_RDKB=true
                       RDKB_FLAG=" -DRDKB_SUPPORT ";;
                  no) ENABLE_RDKB=false
                      AC_MSG_ERROR([RDKB is disabled]) ;;
                   *) AC_MSG_ERROR([bad value ${enableval} for --enable-rdkb]) ;;
                esac
              ],
              [echo "rdkb is disabled"])

AM_CONDITIONAL([ENABLE_RDKB], [test x$ENABLE_RDKB = xtrue])
AC_SUBST(RDKB_FLAG)

# Check for breakpad
BREAKPAD_CFLAGS=" "
BREAKPAD_LFLAGS=" "
AC_ARG_ENABLE([breakpad],
	      AS_HELP_STRING([--enable-breakpad],[enable breakpad support (default is no)]),
	      [
	        case "${enableval}" in
	         yes) BREAKPAD_CFLAGS="-DINCLUDE_BREAKPAD" ;;
		 no) AC_MSG_ERROR([breakpad is disabled]) ;;
		 *) AC_MSG_ERROR([bad value ${enableval} for --enable-breakpad]) ;;
		 esac
		 ],
              [echo "breakpad is disabled"])

AC_CONFIG_FILES([
        Makefile
        rfcMgr/Makefile
        rfcapi/Makefile
        tr181api/Makefile
	utils/Makefile])
AC_SUBST(GTEST_ENABLE_FLAG)
AC_OUTPUT
