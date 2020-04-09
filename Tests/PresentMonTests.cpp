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
    struct Paths {
        std::string etl_;
        std::string goldCsv_;
        std::string testCsv_;
    } path_;

    explicit GoldStandardTests(Paths const& paths)
        : path_(paths)
    {
    }

    bool CreateOutputDirectories() const
    {
        auto path = (char*) &path_.testCsv_[0];
        for (auto i = outDir_.size() - 1, n = path_.testCsv_.size(); i < n; ++i) {
            if (path[i] == '\\') {
                path[i] = '\0';

                auto attr = GetFileAttributesA(path);
                if (attr == INVALID_FILE_ATTRIBUTES) {
                    if (!CreateDirectoryA(path, NULL)) {
                        return false;
                    }
                } else if ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                    return false;
                }

                path[i] = '\\';
            }
        }

        return true;
    }

    void TestBody() override
    {
        // Open the gold CSV
        PresentMonCsv goldCsv;
        RETURN_ON_FATAL_FAILURE(goldCsv.Open(path_.goldCsv_.c_str()));

        // Make sure output directory exists.
        ASSERT_TRUE(CreateOutputDirectories());

        // Generate command line, querying gold CSV to try and match expected
        // data.
        std::string cmdline;
        cmdline += '\"';
        cmdline += presentMonPath_;
        cmdline += "\" -stop_existing_session -no_top -etl_file \"";
        cmdline += path_.etl_;
        cmdline += "\" -output_file \"";
        cmdline += path_.testCsv_;
        cmdline += "\"";

        if (goldCsv.GetColumnIndex("AllowsTearing") == SIZE_MAX) {
            cmdline += " -simple";
        } else if (goldCsv.GetColumnIndex("WasBatched") != SIZE_MAX) {
            cmdline += " -verbose";
        }

        SCOPED_TRACE(cmdline);

        // Start PresentMon wait for it to complete.
        STARTUPINFOA si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;

        PROCESS_INFORMATION pi = {};
        if (!CreateProcessA(nullptr, (LPSTR) cmdline.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi)) {
            FAIL() << "Failed to start the PresentMon process.";
        }

        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        EXPECT_TRUE(GetExitCodeProcess(pi.hProcess, &exitCode));
        EXPECT_EQ(exitCode, 0u);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        // Open test CSV file and check it has the same columns as gold
        PresentMonCsv testCsv;
        RETURN_ON_FATAL_FAILURE(testCsv.Open(path_.testCsv_.c_str()));
        RETURN_ON_FATAL_FAILURE(testCsv.CompareColumns(goldCsv));

        // Compare gold/test CSV data rows
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
    char* relName,
    size_t relNameLen,
    GoldStandardTests::Paths* paths)
{
    (void) relNameLen;

    // Confirm fileName is an ETL
    auto len = strlen(relName);
    if (len < 4 || _strnicmp(relName + len - 4, ".etl", 4) != 0) {
        return false;
    }

    paths->etl_ = testDir_;
    paths->etl_ += relName;

    // Check if there is a CSV with the same path/name but different extension
    relName[len - 3] = 'c';
    relName[len - 2] = 's';
    relName[len - 1] = 'v';

    paths->goldCsv_ = testDir_;
    paths->goldCsv_ += relName;

    auto attr = GetFileAttributesA(paths->goldCsv_.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return false;
    }

    // Create test CSV path
    paths->testCsv_ = outDir_;
    paths->testCsv_ += relName;

    // Prune off extension to use as test name
    relName[len - 4] = '\0';
    return true;
}

void AddGoldEtlCsvTests(
    char const* dir)
{
    GoldStandardTests::Paths paths;
    auto fnidx = strlen(dir);
    auto relidx = testDir_.size();

    // Start listing files at dir/*
    char path[MAX_PATH];
    if (fnidx + 1 >= _countof(path)) {
        return;
    }
    memcpy(path, dir, fnidx);
    path[fnidx + 0] = '*';
    path[fnidx + 1] = '\0';

    auto relName = path + relidx;
    auto relNameLen = _countof(path) - relidx;

    WIN32_FIND_DATAA ff = {};
    auto h = FindFirstFileA(path, &ff);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }
    do
    {
        auto len = strlen(ff.cFileName);
        if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (len == 1 && ff.cFileName[0] == '.') continue;
            if (len == 2 && ff.cFileName[0] == '.' && ff.cFileName[1] == '.') continue;
            if (fnidx + len + 2 > _countof(path)) continue;

            memcpy(path + fnidx, ff.cFileName, len);
            memcpy(path + fnidx + len, "\\", 2);
            AddGoldEtlCsvTests(path);
        } else {
            if (fnidx + len + 1 > _countof(path)) continue;
            memcpy(path + fnidx, ff.cFileName, len + 1);

            if (CheckGoldEtlCsvPair(relName, relNameLen, &paths)) {
                ::testing::RegisterTest(
                    "CsvCompareTests", relName, nullptr, nullptr, __FILE__, __LINE__,
                    [=]() -> ::testing::Test* { return new GoldStandardTests(paths); });
            }
        }
    } while (FindNextFileA(h, &ff) != 0);

    FindClose(h);
}

void DeleteDirectory(
    char const* dir)
{
    char path[MAX_PATH];
    auto fnidx = strlen(dir);
    if (fnidx + 1 >= _countof(path)) {
        return;
    }
    memcpy(path, dir, fnidx);
    path[fnidx + 0] = '*';
    path[fnidx + 1] = '\0';

    WIN32_FIND_DATAA ff = {};
    auto h = FindFirstFileA(path, &ff);
    if (h == INVALID_HANDLE_VALUE) {
        return;
    }
    do
    {
        if (ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(ff.cFileName, ".") != 0 &&
                strcmp(ff.cFileName, "..") != 0) {
                strcpy_s(path + fnidx, _countof(path) - fnidx, ff.cFileName);
                DeleteDirectory(path);
            }
        } else {
            strcpy_s(path + fnidx, _countof(path) - fnidx, ff.cFileName);
            DeleteFileA(path);
        }
    } while (FindNextFileA(h, &ff) != 0);

    path[fnidx - 1] = '\0';
    RemoveDirectoryA(path);

    FindClose(h);
}

bool CheckPath(
    char const* commandLineArg,
    std::string* str,
    char const* path,
    bool directory,
    bool* exists)
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

    // Make sure file exists and is the right type (file vs directory).
    auto attr = GetFileAttributesA(fullPath);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (exists == nullptr) { // must exist
            fprintf(stderr, "error: path does not exist: %s\n", fullPath);
            fprintf(stderr, "       Specify a new path using the %s command line argument.\n", commandLineArg);
            return false;
        }
        *exists = false;
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
                "    --delete             Delete output directory after tests, unless the directory already existed.\n"
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
    bool deleteOutDir = false;
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

        if (_stricmp(argv[i], "--delete") == 0) {
            deleteOutDir = true;
            continue;
        }

        fprintf(stderr, "error: unrecognized command line argument: %s.\n", argv[i]);
        fprintf(stderr, "       Use --help command line argument for usage.\n");
        return 1;
    }

    // Check command line arguments...
    bool outDirExisted = true;
    if (!CheckPath("--presentmon", &presentMonPath_, presentMonPath, false, nullptr) ||
        !CheckPath("--testdir", &testDir_, testDir, true, nullptr) ||
        !CheckPath("--outdir", &outDir_, outDir, true, &outDirExisted)) {
        return 1;
    }

    // Search test dir for gold ETL/CSV test pairs.
    AddGoldEtlCsvTests(testDir_.c_str());

    // Run all the tests
    int result = RUN_ALL_TESTS();

    // If we created the output directory, and the user requested it, delete
    // the output directory.
    if (deleteOutDir) {
        if (outDirExisted) {
            fprintf(stderr, "warning: output directory existed before running tests, and won't be deleted.\n");
        } else {
            DeleteDirectory(outDir_.c_str());
        }
    }

    return result;
}
