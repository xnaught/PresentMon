#include "PresentMonCsv.h"

#include <../PresentMon/generated/version.h>
#include <gtest/gtest.h>
#include <strsafe.h>
#include <windows.h>

#define RETURN_ON_FATAL_FAILURE(_P) do { _P; if (::testing::Test::HasFatalFailure()) return; } while (0)

std::string presentMonPath_;
std::string testDir_;
std::string outDir_;

class GoldStandardTests : public ::testing::Test {
public:
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

        char cmdline[256];
        EXPECT_HRESULT_SUCCEEDED(StringCchPrintfA(cmdline, ARRAYSIZE(cmdline),
            "\"%s\" -etl_file \"%s%s.etl\" -stop_existing_session -process_name %s -verbose -output_file \"%s%s.csv\"",
            presentMonPath_.c_str(),
            testDir_.c_str(), tracename,
            processname,
            outDir_.c_str(), tracename));

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
        StringCchPrintfA(referenceFilePath, ARRAYSIZE(referenceFilePath), "%s%s.csv", testDir_.c_str(), tracename);
        PresentMonCsv goldCsv;
        RETURN_ON_FATAL_FAILURE(goldCsv.Open(referenceFilePath));

        // Create a CSV reader on the actual output of this test run. Use single-byte string file names for CSV reader.
        char outputFilePath[256];
        StringCchPrintfA(outputFilePath, ARRAYSIZE(outputFilePath), "%s%s.csv", outDir_.c_str(), tracename);
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

bool CheckPath(
    char const* commandLineArg,
    std::string* str,
    char const* path,
    bool directory,
    bool createDirectory)
{
    // If path not specified in command line, use default value
    if (path == nullptr) {
        path = str->c_str();
    }

    // Get full path
    char fullPath[MAX_PATH];
    auto r = GetFullPathNameA(path, _countof(fullPath), fullPath, nullptr);
    if (r == 0 || r > _countof(fullPath)) {
        fprintf(stderr, "error: could not get full path for: %s\n", path);
        fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
        return false;
    }

    // Make sure file exists and is the right type (file vs directory).  If
    // createDirectory==true, then create the directory if it doesn't already
    // exist.
    auto attr = GetFileAttributesA(fullPath);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (!directory || !createDirectory) {
            fprintf(stderr, "error: path is not a %s: %s\n", directory ? "directory" : "file", fullPath);
            fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
            return false;
        }
        if (!CreateDirectoryA(fullPath, NULL)) {
            fprintf(stderr, "error: failed to create directory: %s\n", fullPath);
            fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
            return false;
        }
    } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) != (directory ? FILE_ATTRIBUTE_DIRECTORY : 0u)) {
        fprintf(stderr, "error: path is not a %s: %s\n", directory ? "directory" : "file", fullPath);
        fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
        return false;
    }

    // Update the string with the full path and end directories with separator.
    *str = fullPath;
    if (directory) {
        *str += '\\';
    }

    return true;
}

void SetDefaults()
{
    // PresentMon path
    presentMonPath_ = "PresentMon64-";
    if (strncmp(PRESENT_MON_VERSION, "dev", 3) == 0) {
        presentMonPath_ += "dev";
    } else {
        presentMonPath_ += PRESENT_MON_VERSION;
    }
    presentMonPath_ += ".exe";

    // Test dir
    testDir_ = "../../../Tests/Gold";

    // Output dir
    char path[MAX_PATH];
    GetTempPathA(_countof(path), path);
    strcat_s(path, "PresentMonTestOutput");
    outDir_ = path;
}

int main(
    int argc,
    char** argv)
{
    // Put out usage before googletest
    auto help = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "-?") == 0 ||
            strcmp(argv[i], "/?") == 0) {
            printf(
                "PresentMonTests.exe [options]\n"
                "options:\n"
                "    --presentmon=path    Path to the PresentMon exe path to test (default=%s).\n"
                "    --testdir=path       Path to directory of test ETLs and gold CSVs (default=%s).\n"
                "    --outdir=path        Path to directory for test outputs (default=%%temp%%/PresentMonTestOutput).\n"
                "\n",
                presentMonPath_.c_str(),
                testDir_.c_str());
            help = true;
            break;
        }
    }

    // InitGoogleTest will remove the arguments it recognizes
    testing::InitGoogleTest(&argc, argv);
    if (help) {
        return 0;
    }

    // Set option defaults
    SetDefaults();

    // Parse remaining command line arguments for custom commands.
    char* presentMonPath = nullptr;
    char* testDir = nullptr;
    char* outDir = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (_strnicmp(argv[i], "--presentmon=", 13) == 0) {
            presentMonPath = argv[i] + 13;
            continue;
        }

        if (_strnicmp(argv[i], "--testdir=", 10) == 0) {
            testDir = argv[i] + 10;
            continue;
        }

        if (_strnicmp(argv[i], "--outdir=", 9) == 0) {
            outDir = argv[i] + 9;
            continue;
        }

        fprintf(stderr, "error: unrecognized command line argument: %s.\n", argv[i]);
        fprintf(stderr, "       Use --help command line argument for usage.\n");
        return 1;
    }

    // Check command line arguments...
    if (!CheckPath("--presentmon", &presentMonPath_, presentMonPath, false, false) ||
        !CheckPath("--testdir", &testDir_, testDir, true, false) ||
        !CheckPath("--outdir", &outDir_, outDir, true, true)) {
        return 1;
    }

    // Run all the tests
    return RUN_ALL_TESTS();
}
