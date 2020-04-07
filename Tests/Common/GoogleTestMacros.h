// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include "RuntimeParameters.h"

#define TEST_GROUP_BEGIN(group_name)
#define TEST_GROUP_END()

#define PRESENTMON_TEST(group_name, test_name) \
  TEST_F(group_name, test_name) {         \
    getapi().test_name();                 \
  }

#define PRESENTMON_TEST_CLASS_BEGIN_NO_SETUP(test_class_name) \
  namespace {                                            \
    class test_class_name : public ::testing::Test {     \
    };

#define PRESENTMON_TEST_CLASS_BEGIN_WITH_SETUP(test_class_name, setup_method) \
  namespace {                                                            \
    class test_class_name : public ::testing::Test {                     \
    protected:                                                           \
      void SetUp() override {                                            \
        getapi().setup_method();                                         \
      }                                                                  \
    };

#define PRESENTMON_TEST_CLASS_END() }

// For old versions of gtest without GTEST_SKIP, stream the message and return success instead
#ifndef GTEST_SKIP
#define GTEST_SKIP_(message) \
  return GTEST_MESSAGE_(message, ::testing::TestPartResult::kSuccess)
#define GTEST_SKIP GTEST_SKIP_("")
#endif

#define EXPECT_THROW_SPECIFIC(statement, exception, condition)  \
    EXPECT_THROW(                                               \
        try {                                                   \
            statement;                                          \
        } catch (const exception& e) {                          \
            EXPECT_TRUE(condition(e));                          \
            throw;                                              \
        }                                                       \
    , exception);

#ifndef INSTANTIATE_TEST_SUITE_P
// Use the old name, removed in newer versions of googletest
#define INSTANTIATE_TEST_SUITE_P INSTANTIATE_TEST_CASE_P
#endif

#define PRESENTMON_SKIP_TEST(message) \
  GTEST_SKIP() << message;

#define PRESENTMON_EXPECT_NO_THROW(statement) EXPECT_NO_THROW(statement)
#define PRESENTMON_EXPECT_TRUE(statement) EXPECT_TRUE(statement)
#define PRESENTMON_EXPECT_FALSE(statement) EXPECT_FALSE(statement)
#define PRESENTMON_EXPECT_EQUAL(val1, val2) EXPECT_EQ(val1, val2)
#define PRESENTMON_EXPECT_NOT_EQUAL(val1, val2) EXPECT_NE(val1, val2)

#define PRESENTMON_SETUP_ASSERT_EQUAL(val1, val2) ASSERT_EQ(val1, val2)

#define IFC_PRESENTMON_EXPECT_NO_THROW(statement) PRESENTMON_EXPECT_NO_THROW(statement); if(::testing::Test::HasFailure()) {goto Cleanup;}
#define IFC_PRESENTMON_EXPECT_TRUE(statement) PRESENTMON_EXPECT_TRUE(statement); if(::testing::Test::HasFailure()) {goto Cleanup;}
#define IFC_PRESENTMON_EXPECT_FALSE(statement) PRESENTMON_EXPECT_FALSE(statement); if(::testing::Test::HasFailure()) {goto Cleanup;}
#define IFC_PRESENTMON_EXPECT_EQUAL(val1, val2) PRESENTMON_EXPECT_EQUAL(val1, val2); if(::testing::Test::HasFailure()) {goto Cleanup;}
#define IFC_PRESENTMON_EXPECT_HRESULT_SUCCEEDED(hresult_expression) PRESENTMON_EXPECT_HRESULT_SUCCEEDED(hresult_expression); if(::testing::Test::HasFailure()) {goto Cleanup;}
#define IFC_PRESENTMON_EXPECT_HRESULT_FAILED(hresult_expression) PRESENTMON_EXPECT_HRESULT_FAILED(hresult_expression); if(::testing::Test::HasFailure()) {goto Cleanup;}

#define PRESENTMON_LOG_ERROR(message) \
  ADD_FAILURE() << message
#define PRESENTMON_LOG_COMMENT(message)\
  SCOPED_TRACE(message)

#define PRESENTMON_EXPECT_HRESULT_SUCCEEDED(hresult_expression) EXPECT_HRESULT_SUCCEEDED(hresult_expression)
#define PRESENTMON_EXPECT_HRESULT_FAILED(hresult_expression) EXPECT_HRESULT_FAILED(hresult_expression)
#define PRESENTMON_EXPECT_THROW_SPECIFIC(statement, exception, condition) EXPECT_THROW_SPECIFIC(statement, exception, condition)