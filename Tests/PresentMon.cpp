// Copyright (C) 2020-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include "PresentMonTests.h"

#include <initializer_list>
#include <unordered_set>

void AddTestFailure(char const* file, int line, char const* fmt, ...)
{
    char buffer[512];

    va_list val;
    va_start(val, fmt);
    vsnprintf(buffer, _countof(buffer), fmt, val);
    va_end(val);

    GTEST_MESSAGE_AT_(file, line, buffer, ::testing::TestPartResult::kNonFatalFailure);
}

namespace {

struct HeaderCollection {
    wchar_t const* param_;
    std::unordered_set<PresentMonCsv::Header> required_;
    uint32_t foundCount_;

    HeaderCollection(wchar_t const* param, std::initializer_list<PresentMonCsv::Header> const& i)
        : param_(param)
        , required_(i)
        , foundCount_(0)
    {
    }

    bool Check(PresentMonCsv::Header h)
    {
        if (required_.find(h) == required_.end()) {
            return false;
        }
        foundCount_ += 1;
        return true;
    }

    bool Validate(std::vector<wchar_t const*>* params) const
    {
        if (param_ != nullptr) {
            bool negParam = wcsncmp(param_, L"-no_", 4) == 0;

            if (foundCount_ == 0) {
                if (negParam) {
                    params->push_back(param_);
                }
                return true;
            }

            if (!negParam) {
                params->push_back(param_);
            }
        }
        return foundCount_ == required_.size();
    }
};

PresentMonCsv::Header FindHeader(char const* header)
{
    for (uint32_t i = 0; i < PresentMonCsv::KnownHeaderCount; ++i) {
        auto h = (PresentMonCsv::Header) i;
        if (strcmp(header, PresentMonCsv::GetHeaderString(h)) == 0) {
            return h;
        }
    }
    return PresentMonCsv::UnknownHeader;
}

}

bool PresentMonCsv::Open(char const* file, int line, std::wstring const& path)
{
    // Setup the header groups
    HeaderCollection headerGroups[] = {
        HeaderCollection(nullptr, { Header_Application,
                                    Header_ProcessID,
                                    Header_SwapChainAddress,
                                    Header_Runtime,
                                    Header_SyncInterval,
                                    Header_PresentFlags,
                                    Header_Dropped,
                                    Header_TimeInSeconds,
                                    Header_msBetweenPresents,
                                    Header_msInPresentAPI }),

        HeaderCollection(L"-qpc_time", { Header_QPCTime, }),

        HeaderCollection(L"-no_track_display", { Header_AllowsTearing,
                                                 Header_PresentMode,
                                                 Header_msBetweenDisplayChange,
                                                 Header_msUntilRenderComplete,
                                                 Header_msUntilDisplayed }),

        HeaderCollection(L"-no_track_gpu", { Header_msUntilRenderStart,
                                             Header_msGPUActive }),

        HeaderCollection(L"-track_gpu_video", { Header_msGPUVideoActive }),

        HeaderCollection(L"-no_track_input", { Header_msSinceInput }),
    };

    // Load the CSV
    for (uint32_t i = 0; i < _countof(headerColumnIndex_); ++i) {
        headerColumnIndex_[i] = SIZE_MAX;
    }
    cols_.clear();
    path_ = path;
    line_ = 0;

    if (_wfopen_s(&fp_, path.c_str(), L"r")) {
        AddTestFailure(file, line, "Failed to open file: %ls", path.c_str());
        return false;
    }

    // Remove UTF-8 marker if there is one.
    long int startOfs = 0;
    if (fread(row_, 3, 1, fp_) == 1 &&
        row_[0] == -17 &&
        row_[1] == -69 &&
        row_[2] == -65) {
        startOfs = 3;
    }
    fseek(fp_, startOfs, SEEK_SET);

    // Read the header and ensure required columns are present
    ReadRow();

    for (size_t i = 0, n = cols_.size(); i < n; ++i) {
        auto h = FindHeader(cols_[i]);
        switch (h) {
        case KnownHeaderCount:
        case UnknownHeader:
            AddTestFailure(Convert(path_).c_str(), (int) line_, "Unrecognised column: %s", cols_[i]);
            break;
        default:
            if (headerColumnIndex_[(size_t) h] != SIZE_MAX) {
                AddTestFailure(Convert(path_).c_str(), (int) line_, "Duplicate column: %s", cols_[i]);
            } else {
                headerColumnIndex_[(size_t) h] = i;

                for (auto& hg : headerGroups) {
                    if (hg.Check(h)) {
                        break;
                    }
                }
            }
            break;
        }
    }

    for (auto const& hg : headerGroups) {
        if (!hg.Validate(&params_)) {
            AddTestFailure(Convert(path_).c_str(), (int) line_, "Missing required columns.");
        }
    }

    // Add dependencies to avoid warnings:
    // warning: -track_gpu_video requires GPU tracking; ignoring -track_gpu_video due to -no_track_gpu.
    // warning: GPU tracking requires display tracking; ignoring -no_track_display.
    size_t idx_track_gpu_video = SIZE_MAX;
    size_t idx_no_track_gpu = SIZE_MAX;
    size_t idx_no_track_display = SIZE_MAX;
    for (size_t i = 0, n = params_.size(); i < n; ++i) {
             if (wcscmp(params_[i], L"-track_gpu_video")  == 0) { idx_track_gpu_video  = i; }
        else if (wcscmp(params_[i], L"-no_track_gpu")     == 0) { idx_no_track_gpu     = i; }
        else if (wcscmp(params_[i], L"-no_track_display") == 0) { idx_no_track_display = i; }
    }
    if (idx_track_gpu_video != SIZE_MAX && idx_no_track_gpu != SIZE_MAX) {
        params_.erase(params_.begin() + idx_track_gpu_video);
        idx_track_gpu_video = SIZE_MAX;
    }
    if (idx_no_track_gpu == SIZE_MAX && idx_no_track_display != SIZE_MAX) {
        params_.erase(params_.begin() + idx_no_track_display);
        idx_no_track_display = SIZE_MAX;
    }

    return true;
}

void PresentMonCsv::Close()
{
    fclose(fp_);
    fp_ = nullptr;
}

bool PresentMonCsv::ReadRow()
{
    row_[0] = '\0';
    cols_.clear();

    // Read a line
    if (fgets(row_, _countof(row_), fp_) == nullptr) {
        if (ferror(fp_) != 0) {
            AddTestFailure(Convert(path_).c_str(), (int) line_, "File read error");
        }
        return false;
    }

    line_ += 1;

    // Split line into columns, skipping leading/trailing whitespace
    auto p0 = row_;
    for (; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
    for (auto p = p0; ; ++p) {
        auto ch = *p;
        if (ch == ',' || ch == '\0') {
            *p = '\0';
            cols_.push_back(p0);
            for (p0 = p + 1; *p0 == ' ' || *p0 == '\t'; ++p0) *p0 = '\0';
            for (auto q = p - 1; *q == ' ' || *q == '\t' || *q == '\n' || *q == '\r'; --q) *q = '\0';
            if (ch == '\0') break;
        }
    }

    return true;
}

size_t PresentMonCsv::GetColumnIndex(char const* header) const
{
    auto h = FindHeader(header);
    return h < KnownHeaderCount ? headerColumnIndex_[h] : SIZE_MAX;
}

PresentMon::PresentMon()
    : cmdline_()
    , csvArgSet_(false)
{
    cmdline_ += L'\"';
    cmdline_ += exePath_;
    cmdline_ += L"\" -no_console_stats";
}

PresentMon::~PresentMon()
{
    if (::testing::Test::HasFailure()) {
        printf("%ls\n", cmdline_.c_str());
    }
}

void PresentMon::AddEtlPath(std::wstring const& etlPath)
{
    cmdline_ += L" -etl_file \"";
    cmdline_ += etlPath;
    cmdline_ += L'\"';
}

void PresentMon::AddCsvPath(std::wstring const& csvPath)
{
    EXPECT_FALSE(csvArgSet_);
    cmdline_ += L" -output_file \"";
    cmdline_ += csvPath;
    cmdline_ += L'\"';
    csvArgSet_ = true;

    // Delete the file if it exists.  Otherwise, PresentMon may not output
    // anything and any previous content will remain.
    DeleteFile(csvPath.c_str());
}

void PresentMon::Add(wchar_t const* args)
{
    cmdline_ += L' ';
    cmdline_ += args;
}

void PresentMon::Start(char const* file, int line)
{
    if (!csvArgSet_) {
        cmdline_ += L" -no_csv";
        csvArgSet_ = true;
    }

    STARTUPINFO si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    if (CreateProcess(nullptr, &cmdline_[0], nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, (PROCESS_INFORMATION*) this) == 0) {
        AddTestFailure(file, line, "Failed to start PresentMon");
    }
}

bool PresentMon::IsRunning(DWORD timeoutMilliseconds) const
{
    return WaitForSingleObject(hProcess, timeoutMilliseconds) == WAIT_TIMEOUT;
}

void PresentMon::ExpectExited(char const* file, int line, DWORD timeoutMilliseconds, DWORD expectedExitCode)
{
    auto isRunning = IsRunning(timeoutMilliseconds);
    if (isRunning) {
        AddTestFailure(file, line, "PresentMon still running after %ums", timeoutMilliseconds);

        TerminateProcess(hProcess, 0);
        WaitForSingleObject(hProcess, INFINITE);
    } else {
        DWORD exitCode = 0;
        GetExitCodeProcess(hProcess, &exitCode);

        if (exitCode != expectedExitCode) {
            AddTestFailure(file, line, "Unexpected PresentMon exit code: %d (expecting %d)", exitCode, expectedExitCode);
        }
    }

    CloseHandle(hProcess);
    CloseHandle(hThread);
}
