# Test Suite Documentation

This directory contains the test suite for the MarketMakerSimulator project using Google Test (GTest).

## Directory Structure

```
tests/
├── README.md           # This file
├── test.cc             # Main test entry point with sanity tests
├── test_utils.hh       # Reusable test utilities and helper classes
├── algo_test.cc        # Algo class tests with fixtures
└── *_test.cc           # Additional test files following naming convention
```

## Test Organization

### File Naming Convention

- **`test.cc`**: Main test file, contains GTest entry point and basic sanity tests
- **`*_test.cc`**: Test files for specific classes/modules (e.g., `algo_test.cc`, `config_test.cc`)
- **`test_utils.hh`**: Shared test utilities, builders, and helper functions

### Test Fixture Naming Convention

Test fixtures should follow this pattern: `<ClassName><Feature>Test`

Examples:
- `AlgoServerTest` - Tests for Algo server functionality
- `AlgoClientTest` - Tests for Algo client functionality
- `ConfigParserTest` - Tests for Config parsing

### Test Naming Convention

Individual tests should have descriptive names that explain what is being tested:

```cpp
TEST_F(AlgoServerTest, InitializeServerSetsCorrectState)
TEST_F(AlgoClientTest, StopClientTwiceIsIdempotent)
TEST_F(ConfigTest, ParseValidConfigReturnsSuccess)
```

## Using the Test Harness

### Basic Test Structure

```cpp
#include <gtest/gtest.h>
#include "test_utils.hh"
#include "mms/YourClass.hh"

// Test fixture with custom setup/teardown
class YourClassTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize test data
        test_obj_ = std::make_unique<YourClass>();
    }

    void TearDown() override
    {
        // Clean up
        test_obj_.reset();
    }

    std::unique_ptr<YourClass> test_obj_;
};

TEST_F(YourClassTest, DescriptiveTestName)
{
    // Arrange
    // ... setup test conditions

    // Act
    // ... perform the operation

    // Assert
    EXPECT_TRUE(/* condition */);
}
```

### Using Test Utilities

#### TestConfigBuilder

The `TestConfigBuilder` class helps create test configurations:

```cpp
#include "test_utils.hh"

// Use default config from config.toml
auto config = mms::create_default_test_config();

// Create custom config with specific port
auto config = mms::create_test_config_with_port(8080);

// Build fully custom config
auto config = mms::TestConfigBuilder()
    .with_market_ip("192.168.1.100")
    .with_market_port(9000)
    .with_temp_config()
    .build();
```

#### TempFileGuard

RAII wrapper for temporary files that cleans up automatically:

```cpp
#include "test_utils.hh"

TEST_F(MyTest, WorksWithTempFile)
{
    auto temp_path = std::filesystem::temp_directory_path() / "test.dat";
    mms::TempFileGuard guard(temp_path);

    // Use temp_path...
    // File is automatically deleted when guard goes out of scope
}
```

## Running Tests

### Build Tests

```bash
# From project root
make clean
make -j$(nproc)
```

### Run All Tests

```bash
# From build directory
./bin/unit_tests
```

### Run Specific Tests

```bash
# Run tests from a specific fixture
./bin/unit_tests --gtest_filter=AlgoServerTest.*

# Run a single test
./bin/unit_tests --gtest_filter=AlgoServerTest.InitializeServerSetsState

# Run tests matching a pattern
./bin/unit_tests --gtest_filter=*Client*
```

### Run Tests with CTest

```bash
# From build directory
ctest
ctest --verbose
ctest --output-on-failure
```

## Best Practices

### 1. Test Independence

Each test should be independent and not rely on the state from other tests:

```cpp
// Good - each test is independent
TEST_F(AlgoTest, TestA) { /* ... */ }
TEST_F(AlgoTest, TestB) { /* ... */ }

// Bad - TestB depends on TestA running first
static int shared_state;
TEST_F(AlgoTest, TestA) { shared_state = 5; }
TEST_F(AlgoTest, TestB) { EXPECT_EQ(shared_state, 5); } // Fragile!
```

### 2. Use Fixtures for Shared Setup

When multiple tests need the same setup, use a test fixture:

```cpp
class DatabaseTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        db_ = std::make_unique<Database>("test.db");
        db_->initialize();
    }

    void TearDown() override
    {
        db_->cleanup();
        db_.reset();
    }

    std::unique_ptr<Database> db_;
};

TEST_F(DatabaseTest, InsertWorks) { /* ... */ }
TEST_F(DatabaseTest, QueryWorks) { /* ... */ }
```

### 3. Test One Thing Per Test

Keep tests focused on a single behavior:

```cpp
// Good - tests one thing
TEST_F(AlgoTest, InitializeServerSetsState) { /* ... */ }
TEST_F(AlgoTest, InitializeServerBindsPort) { /* ... */ }

// Bad - tests multiple things
TEST_F(AlgoTest, InitializeServerDoesEverything) { /* ... */ }
```

### 4. Use Descriptive Assertion Messages

```cpp
// Good - clear what failed
EXPECT_EQ(result.size(), 5) << "Expected 5 market data entries";

// OK but less informative
EXPECT_EQ(result.size(), 5);
```

### 5. Handle Expected Failures

```cpp
// Test both success and expected failure modes
auto result = algo->initialize_server();

if (result.has_value())
{
    EXPECT_TRUE(algo->is_server_initialized());
}
else
{
    // Port might be in use - this is an expected failure
    EXPECT_EQ(result.error(), AlgoError::INIT_SERVER_FAIL);
}
```

## Adding New Tests

### Step 1: Create Test File

Create a new file following the naming convention: `<module>_test.cc`

```bash
touch tests/my_module_test.cc
```

### Step 2: Define Test Fixture

```cpp
#include <gtest/gtest.h>
#include "test_utils.hh"
#include "mms/MyModule.hh"

class MyModuleTest : public ::testing::Test
{
protected:
    void SetUp() override { /* setup */ }
    void TearDown() override { /* cleanup */ }
};
```

### Step 3: Write Tests

```cpp
TEST_F(MyModuleTest, BasicFunctionality)
{
    // Test implementation
}
```

### Step 4: Build and Run

The CMakeLists.txt automatically picks up `*_test.cc` files, so just rebuild:

```bash
make
./bin/unit_tests --gtest_filter=MyModuleTest.*
```

## Common GTest Assertions

```cpp
// Boolean conditions
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// Comparison
EXPECT_EQ(val1, val2);    // Equal
EXPECT_NE(val1, val2);    // Not equal
EXPECT_LT(val1, val2);    // Less than
EXPECT_LE(val1, val2);    // Less than or equal
EXPECT_GT(val1, val2);    // Greater than
EXPECT_GE(val1, val2);    // Greater than or equal

// String comparison
EXPECT_STREQ(str1, str2);
EXPECT_STRNE(str1, str2);

// Floating point
EXPECT_FLOAT_EQ(val1, val2);
EXPECT_DOUBLE_EQ(val1, val2);
EXPECT_NEAR(val1, val2, abs_error);

// Exceptions
EXPECT_THROW(statement, exception_type);
EXPECT_NO_THROW(statement);
EXPECT_ANY_THROW(statement);

// Use ASSERT_* for fatal failures (stops test execution)
ASSERT_NE(ptr, nullptr);  // Don't continue if nullptr
EXPECT_EQ(ptr->value, 5); // This won't execute if above failed
```

## Debugging Tests

### Run Tests in Verbose Mode

```bash
./bin/unit_tests --gtest_print_time=1
```

### Show Test Output

```bash
# GTest normally suppresses stdout/stderr on success
./bin/unit_tests --gtest_print_utf8=0
```

### Use GDB

```bash
gdb --args ./bin/unit_tests --gtest_filter=AlgoTest.MyTest
```

## Resources

- [Google Test Primer](https://google.github.io/googletest/primer.html)
- [Google Test Advanced Guide](https://google.github.io/googletest/advanced.html)
- [Google Test FAQ](https://google.github.io/googletest/faq.html)
