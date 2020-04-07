#include "../Common/csv/csv.h"
#include "../Common/test.h"
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include "GoldStandardTests.h"


static void RunPresentMon(char* tracename, char* processname);

static void TrivialFlipTest()
{
    RunPresentMon("trivflip", "trivflip12.exe");
}

static void RunPresentMon(char* tracename, char* processname)
{
    char tracenameMsg[256];
	PRESENTMON_EXPECT_HRESULT_SUCCEEDED(StringCchPrintfA(tracenameMsg, ARRAYSIZE(tracenameMsg), "TraceName: %s", tracename));
    PRESENTMON_LOG_COMMENT(tracenameMsg);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

#ifdef _DEBUG
#define BINDIR "debug"
#else
#define BINDIR "release"
#endif

#ifdef _WIN64 
#define ARCH "64"
#else
#define ARCH "32"
#endif
    char cmdline[256];
    PRESENTMON_EXPECT_HRESULT_SUCCEEDED(StringCchPrintfA(cmdline, ARRAYSIZE(cmdline),
        "..\\..\\build\\%s\\bin\\PresentMon%s-dev.exe -etl_file ..\\..\\Tests\\GoldStandard\\Resources\\Traces\\%s.etl -process_name %s -verbose -output_file ..\\..\\Tests\\Output\\%s.csv",
        BINDIR, ARCH, tracename, processname, tracename));
#undef BINDIR
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
        PRESENTMON_LOG_ERROR("Failed to start the PresentMon process.");
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Create a CSV reader on the expected output of this trace. Use single-byte string file names for CSV reader.
    char referenceFilePath[256];
    StringCchPrintfA(referenceFilePath, ARRAYSIZE(referenceFilePath), "..\\..\\Tests\\GoldStandard\\Resources\\References\\%s.csv", tracename);
    io::CSVReader<17> ref(referenceFilePath);

    // Define receivers of csv column data
    std::string refApplication;
    UINT refProcessId;
    std::string refSwapChainAddress;
    std::string refRuntime;
    int refSyncInterval;
    UINT refPresentFlags;
    UINT refAllowTearing;
    std::string refPresentMode;
    UINT refWasBatched;
    UINT refDwmNotified;
    UINT refDropped;
    double refTimeInSeconds;
    double refMsBetweenPresents;
    double refMsBetweenDisplayChange;
    double refMsInPresentAPI;
    double refMsUntilRenderComplete;
    double refMsUntilDisplayed;
    PRESENTMON_EXPECT_NO_THROW(ref.read_header(io::ignore_extra_column,
        "Application", 
        "ProcessID", 
        "SwapChainAddress", 
        "Runtime", 
        "SyncInterval", 
        "PresentFlags", 
        "AllowsTearing", 
        "PresentMode", 
        "WasBatched", 
        "DwmNotified", 
        "Dropped", 
        "TimeInSeconds", 
        "MsBetweenPresents", 
        "MsBetweenDisplayChange", 
        "MsInPresentAPI", 
        "MsUntilRenderComplete", 
        "MsUntilDisplayed"
    ));

    // Create a CSV reader on the actual output of this test run. Use single-byte string file names for CSV reader.
    char outputFilePath[256];
    StringCchPrintfA(outputFilePath, ARRAYSIZE(outputFilePath), "..\\..\\Tests\\Output\\%s.csv", tracename);
    io::CSVReader<17> output(outputFilePath);

    // Define receivers of csv column data
    std::string outputApplication;
    UINT outputProcessId;
    std::string outputSwapChainAddress;
    std::string outputRuntime;
    int outputSyncInterval;
    UINT outputPresentFlags;
    UINT outputAllowTearing;
    std::string outputPresentMode;
    UINT outputWasBatched;
    UINT outputDwmNotified;
    UINT outputDropped;
    double outputTimeInSeconds;
    double outputMsBetweenPresents;
    double outputMsBetweenDisplayChange;
    double outputMsInPresentAPI;
    double outputMsUntilRenderComplete;
    double outputMsUntilDisplayed;
    PRESENTMON_EXPECT_NO_THROW(output.read_header(io::ignore_extra_column,
        "Application",
        "ProcessID",
        "SwapChainAddress",
        "Runtime",
        "SyncInterval",
        "PresentFlags",
        "AllowsTearing",
        "PresentMode",
        "WasBatched",
        "DwmNotified",
        "Dropped",
        "TimeInSeconds",
        "MsBetweenPresents",
        "MsBetweenDisplayChange",
        "MsInPresentAPI",
        "MsUntilRenderComplete",
        "MsUntilDisplayed"
    ));

    UINT lineNumber = 0;

    // Compare following data members of all lines
    bool refReadResult;
    PRESENTMON_EXPECT_NO_THROW(refReadResult = ref.read_row(refApplication, refProcessId, refSwapChainAddress, refRuntime, refSyncInterval, refPresentFlags, refAllowTearing, refPresentMode,
        refWasBatched, refDwmNotified, refDropped, refTimeInSeconds, refMsBetweenPresents, refMsBetweenDisplayChange, refMsInPresentAPI, refMsUntilRenderComplete,
        refMsUntilDisplayed));

    // Expect at least one row in the output.
    PRESENTMON_EXPECT_TRUE(refReadResult);

    while(refReadResult)
    {
        bool outputReadResult;
        lineNumber++;

        TCHAR lineNumberMsg[256];
        IFC_PRESENTMON_EXPECT_HRESULT_SUCCEEDED(StringCchPrintf(lineNumberMsg, ARRAYSIZE(lineNumberMsg), _T("TraceName: %s line: %i"), tracename, lineNumber));
        PRESENTMON_LOG_COMMENT(lineNumberMsg);

        // If fail to read row, it means the two trace files are not the same.
        IFC_PRESENTMON_EXPECT_NO_THROW(outputReadResult = output.read_row(outputApplication, outputProcessId, outputSwapChainAddress, outputRuntime, outputSyncInterval, outputPresentFlags, outputAllowTearing,
            outputPresentMode, outputWasBatched, outputDwmNotified, outputDropped, outputTimeInSeconds, outputMsBetweenPresents, outputMsBetweenDisplayChange, 
            outputMsInPresentAPI, outputMsUntilRenderComplete, outputMsUntilDisplayed));
        IFC_PRESENTMON_EXPECT_TRUE(outputReadResult);

        IFC_PRESENTMON_EXPECT_EQUAL(outputApplication, refApplication);
        IFC_PRESENTMON_EXPECT_EQUAL(outputProcessId, refProcessId);
        IFC_PRESENTMON_EXPECT_EQUAL(outputSwapChainAddress, refSwapChainAddress);
        IFC_PRESENTMON_EXPECT_EQUAL(outputRuntime, refRuntime);
        IFC_PRESENTMON_EXPECT_EQUAL(outputSyncInterval, refSyncInterval);
        IFC_PRESENTMON_EXPECT_EQUAL(outputPresentFlags, refPresentFlags);
        IFC_PRESENTMON_EXPECT_EQUAL(outputAllowTearing, refAllowTearing);
        IFC_PRESENTMON_EXPECT_EQUAL(outputPresentMode, refPresentMode);
        IFC_PRESENTMON_EXPECT_EQUAL(outputWasBatched, refWasBatched);
        IFC_PRESENTMON_EXPECT_EQUAL(outputDwmNotified, refDwmNotified);
        IFC_PRESENTMON_EXPECT_EQUAL(outputDropped, refDropped);
        IFC_PRESENTMON_EXPECT_EQUAL(outputTimeInSeconds, refTimeInSeconds);
        IFC_PRESENTMON_EXPECT_EQUAL(outputMsBetweenPresents, refMsBetweenPresents);
        IFC_PRESENTMON_EXPECT_EQUAL(outputMsBetweenDisplayChange, refMsBetweenDisplayChange);
        IFC_PRESENTMON_EXPECT_EQUAL(outputMsInPresentAPI, refMsInPresentAPI);
        IFC_PRESENTMON_EXPECT_EQUAL(outputMsUntilRenderComplete, refMsUntilRenderComplete);
        IFC_PRESENTMON_EXPECT_EQUAL(outputMsUntilDisplayed, refMsUntilDisplayed);

        IFC_PRESENTMON_EXPECT_NO_THROW(refReadResult = ref.read_row(refApplication, refProcessId, refSwapChainAddress, refRuntime, refSyncInterval, refPresentFlags, refAllowTearing, refPresentMode,
            refWasBatched, refDwmNotified, refDropped, refTimeInSeconds, refMsBetweenPresents, refMsBetweenDisplayChange, refMsInPresentAPI, refMsUntilRenderComplete,
            refMsUntilDisplayed));
    }

    // Ensure that output does not have more rows than ref.
    PRESENTMON_EXPECT_FALSE(refReadResult = output.read_row(outputApplication, outputProcessId, outputSwapChainAddress, outputRuntime, outputSyncInterval, outputPresentFlags,
        outputAllowTearing, outputPresentMode, outputWasBatched, outputDwmNotified, outputDropped, outputTimeInSeconds, outputMsBetweenPresents, 
        outputMsBetweenDisplayChange, outputMsInPresentAPI, outputMsUntilRenderComplete, outputMsUntilDisplayed));
    PRESENTMON_EXPECT_FALSE(refReadResult);

Cleanup:
    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void GoldStandardTestsSetup()
{
    // Setup the output directory
    BOOL result = CreateDirectory(_T("..\\..\\Tests\\Output"), NULL);
    if (!result)
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS && GetLastError() != ERROR_PATH_NOT_FOUND)
        {
            PRESENTMON_LOG_ERROR("Unable to create test output directory."); // If the output directory already exists, that's fine.
        }
    }
}

const GoldStandardTestsApi& getapi() {
    static constexpr GoldStandardTestsApi api =
    {
        GoldStandardTestsSetup,
        TrivialFlipTest
    };
    return api;
}
