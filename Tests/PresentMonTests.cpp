#include "PresentMonCsv.h"

#include <gtest/gtest.h>
#include <strsafe.h>
#include <windows.h>

#define RETURN_ON_FATAL_FAILURE(_P) do { _P; if (::testing::Test::HasFatalFailure()) return; } while (0)

class GoldStandardTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Setup the output directory
        BOOL result = CreateDirectory(L"..\\..\\..\\Tests\\Output", NULL);
        if (!result)
        {
            if (GetLastError() != ERROR_ALREADY_EXISTS && GetLastError() != ERROR_PATH_NOT_FOUND)
            {
                ADD_FAILURE() << "Unable to create test output directory."; // If the output directory already exists, that's fine.
            }
        }
    }

    void RunPresentMon(char* tracename, char* processname)
    {
        char tracenameMsg[256];
        EXPECT_HRESULT_SUCCEEDED(StringCchPrintfA(tracenameMsg, ARRAYSIZE(tracenameMsg), "TraceName: %s", tracename));
        SCOPED_TRACE(tracenameMsg);

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

#ifdef _WIN64 
#define ARCH "64"
#else
#define ARCH "32"
#endif
        char cmdline[256];
        EXPECT_HRESULT_SUCCEEDED(StringCchPrintfA(cmdline, ARRAYSIZE(cmdline),
            "PresentMon%s-dev.exe -etl_file ..\\..\\..\\Tests\\GoldStandard\\Resources\\Traces\\%s.etl -process_name %s -verbose -output_file ..\\..\\..\\Tests\\Output\\%s.csv",
            ARCH, tracename, processname, tracename));
#undef ARCH

        printf(cmdline);

        // Start PresentMon on an etl trace. 
        if (!CreateProcessA(NULL,    // No module name (use command line)
            cmdline,                // Command line
            NULL,                   // Process handle not inheritable
            NULL,                   // Thread handle not inheritable
            FALSE,                  // Set handle inheritance to FALSE
            0,                      // No creation flags
            NULL,                   // Use parent's environment block
            NULL,                   // Use parent's starting directory 
            &si,                    // Pointer to STARTUPINFO structure
            &pi)                    // Pointer to PROCESS_INFORMATION structure
            )
        {
            char path[256];
            GetCurrentDirectoryA(
                256,
                path
            );
            printf("CreateProcess failed (%d). Path %s.\n", GetLastError(), path);
            ADD_FAILURE() << "Failed to start the PresentMon process.";
        }

        // Wait until child process exits.
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Create a CSV reader on the expected output of this trace. Use single-byte string file names for CSV reader.
        char referenceFilePath[256];
        StringCchPrintfA(referenceFilePath, ARRAYSIZE(referenceFilePath), "..\\..\\..\\Tests\\GoldStandard\\Resources\\References\\%s.csv", tracename);
        PresentMonCsv goldCsv;
        RETURN_ON_FATAL_FAILURE(goldCsv.Open(referenceFilePath));

        // Create a CSV reader on the actual output of this test run. Use single-byte string file names for CSV reader.
        char outputFilePath[256];
        StringCchPrintfA(outputFilePath, ARRAYSIZE(outputFilePath), "..\\..\\..\\Tests\\Output\\%s.csv", tracename);
        PresentMonCsv testCsv;
        RETURN_ON_FATAL_FAILURE(testCsv.Open(outputFilePath));
        RETURN_ON_FATAL_FAILURE(testCsv.CompareColumns(goldCsv));

        UINT lineNumber = 0;

        // Compare following data members of all lines
        bool refReadResult = goldCsv.ReadRow();

        // Expect at least one row in the output.
        EXPECT_TRUE(refReadResult);

        while(refReadResult)
        {
            bool outputReadResult;
            lineNumber++;

            TCHAR lineNumberMsg[256];
            EXPECT_HRESULT_SUCCEEDED(StringCchPrintf(lineNumberMsg, ARRAYSIZE(lineNumberMsg), L"TraceName: %s line: %i", tracename, lineNumber));
            SCOPED_TRACE(lineNumberMsg);

            // If fail to read row, it means the two trace files are not the same.
            outputReadResult = testCsv.ReadRow();
            ASSERT_TRUE(outputReadResult);

            ASSERT_TRUE(goldCsv.CompareRow(testCsv, false, nullptr, nullptr));

            refReadResult = goldCsv.ReadRow();
        }

        // Ensure that output does not have more rows than ref.
        refReadResult = testCsv.ReadRow();
        EXPECT_FALSE(refReadResult);
    }
};

TEST_F(GoldStandardTests, TrivialFlipTest)
{
    RunPresentMon("trivflip", "trivflip12.exe");
}
