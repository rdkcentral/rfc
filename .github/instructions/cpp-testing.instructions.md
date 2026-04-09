---
applyTo: "rfcMgr/gtest/**/*.cpp,rfcMgr/gtest/**/*.h,test/**/*.cpp,test/**/*.h"
---

# C++ Testing Standards (Google Test)

## Test Framework

Use Google Test (gtest) and Google Mock (gmock) for all C++ test code.

## Test Organization

### File Structure
- One test file per source file: `foo.c` → `test/FooTest.cpp`
- Test fixtures for complex setups
- Mocks in separate files when reusable

```cpp
// GOOD: Test file structure
// filepath: rfcMgr/gtest/gtest_rfcapi.cpp

extern "C" {
#include "rfcapi.h"
#include "rfc_common.h"
}

#include <gtest/gtest.h>
#include <gmock/gmock.h>

class RfcApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test resources
    }
    
    void TearDown() override {
        // Clean up test resources
    }
};

TEST_F(RfcApiTest, GetRFCValueReturnsStoredParam) {
    // Test RFC parameter read
    const char* param = "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.AccountInfo.AccountID";
    // verify returned value matches the stored RFC parameter
    ASSERT_EQ(getRFCParameter(NULL, param, NULL), WDMP_SUCCESS);
}
```

## Testing Patterns

### Test C Code from C++
- Wrap C headers in `extern "C"` blocks
- Use RAII in tests for automatic cleanup
- Mock C functions using gmock when needed

```cpp
extern "C" {
#include "rfc_common.h"
#include "rfcapi.h"
}

#include <gtest/gtest.h>

class XconfHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize handler stubs
    }
    
    void TearDown() override {
        // Clean up
    }
};

TEST_F(XconfHandlerTest, InitializeHandlerReturnsSuccess) {
    xconf::XconfHandler handler;
    // Test handler initialization
    int result = handler.initializeXconfHandler();
    // verify handler returns success and populates device parameters
    ASSERT_EQ(result, 0);
}
```

### Memory Leak Testing
- All tests must pass valgrind
- Use RAII wrappers for C resources
- Verify cleanup in TearDown

```cpp
// GOOD: RAII wrapper for C resource
class FileHandle {
    FILE* file_;
public:
    explicit FileHandle(const char* path, const char* mode)
        : file_(fopen(path, mode)) {}
    
    ~FileHandle() {
        if (file_) fclose(file_);
    }
    
    FILE* get() const { return file_; }
    bool valid() const { return file_ != nullptr; }
};

TEST(FileTest, ReadConfig) {
    FileHandle file("/tmp/config.json", "r");
    ASSERT_TRUE(file.valid());
    // file automatically closed when test exits
}
```

### Mocking External Dependencies

```cpp
// GOOD: Mock for handler dependencies
class MockIniFile {
public:
    MOCK_METHOD(std::string, get, (const std::string& key));
    MOCK_METHOD(bool, set, (const std::string& key, const std::string& value));
};

TEST(HandlerTest, GetParamUsesIniFile) {
    MockIniFile mock;
    
    EXPECT_CALL(mock, get("Device.DeviceInfo.Manufacturer"))
        .WillOnce(testing::Return("TestVendor"));
    
    std::string result = mock.get("Device.DeviceInfo.Manufacturer");
    EXPECT_EQ("TestVendor", result);
}
```

## Test Quality Standards

### Coverage Requirements
- All public functions must have tests
- Test both success and failure paths
- Test boundary conditions
- Test error handling

### Test Naming
```cpp
// Pattern: TEST(ComponentName, BehaviorBeingTested)

TEST(Vector, CreateReturnsNonNull) { ... }
TEST(Vector, DestroyHandlesNull) { ... }
TEST(Vector, PushIncrementsSize) { ... }
TEST(Utils, ParseConfigInvalidJson) { ... }
```

### Assertions
- Use `ASSERT_*` when test can't continue after failure
- Use `EXPECT_*` when subsequent checks are still valuable
- Provide helpful failure messages

```cpp
// GOOD: Informative assertions
ASSERT_NE(nullptr, ptr) << "Failed to allocate " << size << " bytes";
EXPECT_EQ(expected, actual) << "Mismatch at index " << i;
EXPECT_TRUE(condition) << "Context: " << debug_info;
```

## Running Tests

### Build Tests
```bash
./configure --enable-gtest
make check
```

### Memory Checking
```bash
valgrind --leak-check=full --show-leak-kinds=all \
         ./rfcMgr/gtest/rfcMgr_gtest

valgrind --leak-check=full --show-leak-kinds=all \
         ./rfcMgr/gtest/rfcapi_gtest
```

### Test Output
- Use `GTEST_OUTPUT=xml:results.xml` for CI integration
- Check return code: 0 = all passed
