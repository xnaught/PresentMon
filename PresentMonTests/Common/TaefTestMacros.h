// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "WexTestClass.h"

using namespace WEX::Logging;
using namespace WEX::Common;
using namespace WEX::TestExecution;

#define PRESENTMON_EXPECT_NO_THROW(statement) VERIFY_NO_THROW(statement)

#define PRESENTMON_TEST_CLASS_BEGIN_WITH_SETUP(test_class_name, setup_method) \
  class test_class_name {                                                \
    TEST_CLASS(test_class_name);                                         \
    TEST_CLASS_SETUP(TestClassSetup) {                                   \
      getapi().setup_method();                                           \
      return true;                                                       \
    }

#define PRESENTMON_TEST_CLASS_END() \
  }                            \
  ;

#define PRESENTMON_TEST(group_name, test_name) \
  TEST_METHOD(test_name) {                \
    getapi().test_name();                 \
  }

#define PRESENTMON_SKIP_TEST(message)                                                                 \
  do {                                                                                           \
    Log::Result(TestResults::Skipped,                                                            \
                std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(message).c_str()); \
    return;                                                                                      \
  } while (0)

#define PRESENTMON_EXPECT_NO_THROW(statement) VERIFY_NO_THROW(statement)
#define PRESENTMON_EXPECT_TRUE(statement) VERIFY_IS_TRUE(statement)
#define PRESENTMON_EXPECT_FALSE(statement) VERIFY_IS_FALSE(statement)
#define PRESENTMON_EXPECT_EQUAL(val1, val2) VERIFY_ARE_EQUAL(val1, val2)
#define PRESENTMON_EXPECT_NOT_EQUAL(val1, val2) VERIFY_ARE_NOT_EQUAL(val1, val2)

#define PRESENTMON_SETUP_ASSERT_EQUAL(val1, val2) ASSERT_EQ(val1, val2)

#define IFC_PRESENTMON_EXPECT_TRUE(expr) {if (!(expr)){goto Cleanup;}}
#define IFC_PRESENTMON_EXPECT_FALSE(expr) {if (expr){goto Cleanup;}}
#define IFC_PRESENTMON_EXPECT_EQUAL(expected, actual,  ...) {if (!VERIFY_IS_EQUAL((expected) == (actual), __VA_ARGS__)) {goto Cleanup;}}
#define IFC_PRESENTMON_EXPECT_HRESULT_SUCCEEDED(expr, ...) {if (FAILED(VERIFY_SUCCEEDED_RETURN((expr), __VA_ARGS__))) goto Cleanup;}
#define IFC_PRESENTMON_EXPECT_HRESULT_FAILED(expr, ...) {if (SUCCEEDED(VERIFY_SUCCEEDED_RETURN((expr), __VA_ARGS__))) goto Cleanup;}

#define PRESENTMON_LOG_ERROR(message) \
  VERIFY_FAIL(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(message).c_str())
#define PRESENTMON_LOG_COMMENT(message)\
  WEX::Logging::Log::Comment(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(message).c_str())

#define PRESENTMON_EXPECT_HRESULT_SUCCEEDED(hresult_expression) VERIFY_SUCCEEDED(hresult_expression)
#define PRESENTMON_EXPECT_HRESULT_FAILED(hresult_expression) VERIFY_FAILED(hresult_expression)
#define PRESENTMON_EXPECT_THROW_SPECIFIC(statement, exception, condition) VERIFY_THROWS_SPECIFIC(statement, exception, condition)