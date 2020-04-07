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

#pragma once
#include <stdarg.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include <vector>

struct PresentMonCsv
{
    struct ColumnInfo
    {
        char const* header_;
        bool required_;
    };

    static constexpr ColumnInfo const COLUMN[] = {
        { "Application",            true },
        { "ProcessID",              true },
        { "SwapChainAddress",       true },
        { "Runtime",                true },
        { "SyncInterval",           true },
        { "PresentFlags",           true },
        { "Dropped",                true },
        { "TimeInSeconds",          true },
        { "MsBetweenPresents",      true },
        { "MsInPresentAPI",         true },
        // !-simple
        { "AllowsTearing",          false },
        { "PresentMode",            false },
        { "MsBetweenDisplayChange", false },
        { "MsUntilRenderComplete",  false },
        { "MsUntilDisplayed",       false },
        // -verbose
        { "WasBatched",             false },
        { "DwmNotified",            false },
    };

    std::string path_;
    size_t line_;
    FILE* fp_;
    char row_[1024];
    std::vector<char const*> cols_;
    size_t colIndex_[_countof(COLUMN)];

    PresentMonCsv()
        : line_(0)
        , fp_(nullptr)
    {
    }

    void Open(char const* path)
    {
        path_ = path;
        line_ = 0;
        memset(colIndex_, 0xff, sizeof(colIndex_));

        if (fopen_s(&fp_, path, "rb")) {
            AddFailure("Failed to open file");
            return;
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

        // Read the header and identify each column.
        ReadRow();

        for (size_t i = 0, n = cols_.size(); i < n; ++i) {
            for (size_t c = 0; ; ++c) {
                if (c == _countof(COLUMN)) {
                    AddFailure("Unrecognised column: %s", cols_[i]);
                    break;
                }
                if (strcmp(cols_[i], COLUMN[c].header_) == 0) {
                    if (colIndex_[c] != SIZE_MAX) AddFailure("Duplicate column: %s", cols_[i]);
                    colIndex_[c] = i;
                    break;
                }
            }
        }

        // Make sure all required headers are present
        for (size_t c = 0; c < _countof(COLUMN); ++c) {
            if (COLUMN[c].required_ && colIndex_[c] == SIZE_MAX) {
                AddFailure("Required column missing: %s", COLUMN[c].header_);
            }
        }
    }

    void Close()
    {
        fclose(fp_);
        fp_ = nullptr;
    }

    bool ReadRow()
    {
        row_[0] = '\0';
        cols_.clear();

        // Read a line
        if (fgets(row_, _countof(row_), fp_) == nullptr) {
            if (ferror(fp_) != 0) {
                AddFailure("File read error");
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

    // Look up the column index for 'header', returns SIZE_MAX if header not
    // found.
    size_t GetColumnIndex(char const* header) const
    {
        size_t colIndex = SIZE_MAX;
        for (size_t c = 0; c < _countof(COLUMN); ++c) {
            if (_stricmp(COLUMN[c].header_, header) == 0) {
                colIndex = colIndex_[c];
                break;
            }
        }
        return colIndex;
    }

    // Returns a pointer to the current row's text for column 'colIndex', or ""
    // if the row doesn't have that many columns.
    char const* GetColumn(size_t colIndex) const
    {
        if (colIndex < cols_.size()) {
            return cols_[colIndex];
        }

        AddFailure("CSV column %zu missing", colIndex);
        return "";
    }

    void CompareColumns(PresentMonCsv const& cmp) const
    {
        for (size_t c = 0; c < _countof(COLUMN); ++c) {
            if ((colIndex_[c] == SIZE_MAX) != (cmp.colIndex_[c] == SIZE_MAX)) {
                AddFailure("CSVs have different columns: %s", COLUMN[c].header_);
            }
        }
    }

    bool CompareRow(PresentMonCsv const& cmp, bool print, char const* name, char const* cmpName) const
    {
        bool same = true;
        for (size_t c = 0; c < _countof(COLUMN); ++c) {
            if (colIndex_[c] != SIZE_MAX && cmp.colIndex_[c] != SIZE_MAX) {
                char const* a = GetColumn(colIndex_[c]);
                char const* b = cmp.GetColumn(cmp.colIndex_[c]);
                if (_stricmp(a, b) != 0) {
                    if (print) {
                        int r;
                        if (same) {
                            printf("%s = %s(%zu)\n", name, path_.c_str(), line_);
                            printf("%s = %s(%zu)\n", cmpName, cmp.path_.c_str(), cmp.line_);
                            printf("Difference:\n");
                            printf("%29s", "");
                            r = printf(" %s", name);
                            printf("%*s %s\n", r < 38 ? 38 - r : 0, "", cmpName);
                        }
                        r = printf("    %s", COLUMN[c].header_); printf("%*s", r < 29 ? 29 - r : 0, "");
                        r = printf(" %s", a);                    printf("%*s", r < 38 ? 38 - r : 0, "");
                        printf(" %s\n", b);
                    }

                    same = false;
                }
            }
        }
        return same;
    }

    void AddFailure(char const* fmt, ...) const
    {
        char buffer[512];

        va_list val;
        va_start(val, fmt);
        vsnprintf(buffer, _countof(buffer), fmt, val);
        va_end(val);

        GTEST_MESSAGE_AT_(path_.c_str(), (int) line_, buffer, ::testing::TestPartResult::kFatalFailure);
    }
};
