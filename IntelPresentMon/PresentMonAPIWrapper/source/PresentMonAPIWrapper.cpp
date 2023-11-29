#include "PresentMonAPIWrapper.h"

// global state
uint32_t mockProcessId_ = 4004;

namespace pmapi
{
    Session::Session()
    {
        if (auto sta = pmOpenSession(mockProcessId_); sta != PM_STATUS_SUCCESS) {
            throw SessionException{ std::format("error opening session id={}", (int)sta) };
        }
    }
}
