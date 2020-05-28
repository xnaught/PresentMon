/*
Copyright 2020 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include "PresentMonTests.h"

namespace {

struct TestArgs {
    std::string name_;
    std::wstring etl_;
    std::wstring goldCsv_;
    std::wstring testCsv_;
};

class Tests : public ::testing::Test, TestArgs {
public:
    explicit Tests(TestArgs const& args)
    {
        TestArgs::operator=(args);
    }

    void TestBody() override
    {
        // Open the gold CSV
        PresentMonCsv goldCsv;
        if (!goldCsv.Open(goldCsv_, __FILE__, __LINE__)) {
            return;
        }

        // Make sure output directory exists.
        {
            auto i = testCsv_.find_last_of(L"/\\");
            if (i != std::wstring::npos) {
                ASSERT_TRUE(EnsureDirectoryCreated(testCsv_.substr(0, i)));
            }
        }

        // Generate command line, querying gold CSV to try and match expected
        // data.
        PresentMon pm;
        pm.AddEtlPath(etl_);
        pm.AddCsvPath(testCsv_);
        if (goldCsv.simple_) pm.Add(L"-simple");
        if (goldCsv.verbose_) pm.Add(L"-verbose");
        if (goldCsv.GetColumnIndex("QPCTime") != SIZE_MAX) pm.Add(L"-qpc_time"); // TODO: check if %ull or %.9lf to see if -qpc_time_s
        pm.Start();
        pm.ExpectExit(__FILE__, __LINE__);

        // Open test CSV file and check it has the same columns as gold
        PresentMonCsv testCsv;
        if (!testCsv.Open(testCsv_, __FILE__, __LINE__)) {
            goldCsv.Close();
            return;
        }

        // Compare gold/test CSV data rows
        if (!testCsv.CompareColumns(goldCsv)) {
            goldCsv.Close();
            testCsv.Close();
            return;
        }

        UINT errorLineCount = 0;
        for (;;) {
            auto goldDone = !goldCsv.ReadRow();
            auto testDone = !testCsv.ReadRow();
            EXPECT_EQ(goldDone, testDone);
            if (goldDone || testDone) {
                break;
            }

            errorLineCount += goldCsv.CompareRow(testCsv, errorLineCount == 0, "GOLD", "TEST") ? 0 : 1;
        }

        goldCsv.Close();
        testCsv.Close();

        EXPECT_EQ(errorLineCount, 0u);
    }
};

bool CheckGoldEtlCsvPair(
    std::wstring const& dir,
    size_t relIdx,
    wchar_t const* fileName,
    TestArgs* args)
{
    // Confirm fileName is an ETL
    auto len = wcslen(fileName);
    if (len < 4 || _wcsicmp(fileName + len - 4, L".etl") != 0) {
        return false;
    }

    auto path = dir + fileName;
    args->etl_ = path;

    // Check if there is a CSV with the same path/name but different extension
    path = path.substr(0, path.size() - 4);
    args->goldCsv_ = path + L".csv";

    auto attr = GetFileAttributes(args->goldCsv_.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return false;
    }

    // Create name and test CSV path
    std::wstring relName(path.substr(relIdx));
    args->name_ = Convert(relName);
    args->testCsv_ = outDir_ + relName + L".csv";
    return true;
}

}

void AddGoldEtlCsvTests(
    std::wstring const& dir,
    size_t relIdx)
{
    WIN32_FIND_DATA ff = {};
    auto h = FindFirstFile((dir + L'*').c_str(), &ff);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }
    do
    {
        if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (wcscmp(ff.cFileName, L".") == 0) continue;
            if (wcscmp(ff.cFileName, L"..") == 0) continue;
            AddGoldEtlCsvTests(dir + ff.cFileName + L'\\', relIdx);
        } else {
            TestArgs args;
            if (CheckGoldEtlCsvPair(dir, relIdx, ff.cFileName, &args)) {
                ::testing::RegisterTest(
                    "GoldEtlCsvTests", args.name_.c_str(), nullptr, nullptr, __FILE__, __LINE__,
                    [=]() -> ::testing::Test* { return new Tests(args); });
            }
        }
    } while (FindNextFile(h, &ff) != 0);

    FindClose(h);
}

