####################################################################################
# If not stated otherwise in this file or this component's Licenses file the
# following copyright and licenses apply:
#
# Copyright 2024 RDK Management
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

import fcntl
from rfc_test_helper import *
from typing import TextIO


def create_and_lock_rfc_lock_file() -> TextIO:
    """
    Create and lock the RFC lock file.
    
    :return: The file object for the locked file.
    """
    remove_file(RFC_LOCK_FILE)

    print(f"Creating and locking {RFC_LOCK_FILE}")
    rfc_lock_file = open(RFC_LOCK_FILE, "w")
    fcntl.flock(
        rfc_lock_file, fcntl.LOCK_EX | fcntl.LOCK_NB
    )
    return rfc_lock_file


# Utility to release and remove lock file
def release_and_cleanup_lock_file(lock_file: TextIO) -> None:
    """
    Release the lock and remove the RFC lock file.

    :param lock_file: The file object for the locked file.
    :return: None
    """
    try:
        fcntl.flock(lock_file, fcntl.LOCK_UN)  # Release the lock
        lock_file.close()

    finally:
        print(f"Releasing {RFC_LOCK_FILE}")
        remove_file(RFC_LOCK_FILE)  # Remove the lock file


def test_rfcMgr_with_locked_file() -> None:
    """
    Test running the RFC manager binary when the RFC lock file is locked.

    :return: None
    """
    rfc_lock_file = create_and_lock_rfc_lock_file()  # Create and lock the file

    try:
        rfc_run_binary()

        ERROR_MSG = "RFC: rfcMgr process in progress, New instance not allowed as file /tmp/.rfcServiceLock is locked!"
        assert grep_log_file(RFC_LOG_FILE, ERROR_MSG)
    finally:
        release_and_cleanup_lock_file(rfc_lock_file)
        initial_rfc_setup()



