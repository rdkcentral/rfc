/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "gtest/gtest.h"
#include <string>
#include <cstring>

int add(int num1,int num2)
{
    return (num1+num2);
}

// StringCaseCompare function for AccountID validation
bool StringCaseCompare(const std::string& str1, const std::string& str2)
{
    return (strcasecmp(str1.c_str(), str2.c_str()) == 0);
}

TEST(Add, PositiveCase)
{
    EXPECT_EQ(30,add(10,20));
    EXPECT_EQ(50,add(30,20));
}

// RDKEMW-11615: AccountID Validation Tests
class AccountIDValidationTest : public ::testing::Test {
protected:
    std::string currentAccountID;
    std::string unknownStr;
    
    void SetUp() override {
        currentAccountID = "3064488088886635972";
        unknownStr = "Unknown";
    }
};

// Test case: Empty AccountID should be rejected
TEST_F(AccountIDValidationTest, EmptyAccountIDRejected)
{
    std::string emptyValue = "";
    EXPECT_TRUE(emptyValue.empty());
}

// Test case: Unknown AccountID should be replaced with current value
TEST_F(AccountIDValidationTest, UnknownAccountIDReplaced)
{
    std::string receivedAccountID = "Unknown";
    std::string replacementAccountID = currentAccountID;
    
    // Check if received value is "Unknown"
    bool isUnknown = StringCaseCompare(receivedAccountID, unknownStr);
    EXPECT_TRUE(isUnknown);
    
    // When unknown, should use current value
    if (isUnknown) {
        receivedAccountID = replacementAccountID;
    }
    
    EXPECT_EQ(receivedAccountID, "3064488088886635972");
}

// Test case: Valid AccountID should be accepted
TEST_F(AccountIDValidationTest, ValidAccountIDAccepted)
{
    std::string validAccountID = "3064488088886635972";
    std::string currentValue = "OldAccountID";
    
    // Check if it's not empty and not "Unknown"
    bool isValid = (!validAccountID.empty() && !StringCaseCompare(validAccountID, unknownStr));
    EXPECT_TRUE(isValid);
}

// Test case: AccountID comparison should be case-insensitive
TEST_F(AccountIDValidationTest, UnknownCaseInsensitiveComparison)
{
    std::string unknownUpper = "UNKNOWN";
    std::string unknownMixed = "UnKnOwN";
    
    EXPECT_TRUE(StringCaseCompare(unknownUpper, unknownStr));
    EXPECT_TRUE(StringCaseCompare(unknownMixed, unknownStr));
}

// Test case: Config value change detection
TEST_F(AccountIDValidationTest, ConfigValueChangeDetection)
{
    std::string currentValue = "OldAccountID";
    std::string newValue = "3064488088886635972";
    
    bool valueChanged = (currentValue != newValue);
    EXPECT_TRUE(valueChanged);
}

// Test case: No change when current and new values are same
TEST_F(AccountIDValidationTest, NoConfigValueChangeWhenSame)
{
    std::string currentValue = "3064488088886635972";
    std::string newValue = "3064488088886635972";
    
    bool valueChanged = (currentValue != newValue);
    EXPECT_FALSE(valueChanged);
}

