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
#include <algorithm>
#include "PresentMonTests.h"

namespace {

void TerminateAfterTimedTest(uint32_t timed, DWORD timeoutMilliseconds)
{
    wchar_t timedArg[128];
    _snwprintf_s(timedArg, _TRUNCATE, L"%u", timed);

    PresentMon pm;
    pm.Add(L"-stop_existing_session -terminate_after_timed -timed");
    pm.Add(timedArg);
    pm.Start();
    pm.ExpectExit(__FILE__, __LINE__, timeoutMilliseconds, 0);

    // We don't check the CSV... it's ok if buffers drain adn we get more data
}

}

TEST(CommandLineTests, DISABLED_TerminateAfterTimed_0s)
{
    TerminateAfterTimedTest(0, 2000);
}

TEST(CommandLineTests, TerminateAfterTimed_1s)
{
    TerminateAfterTimedTest(1, 2000);
}
