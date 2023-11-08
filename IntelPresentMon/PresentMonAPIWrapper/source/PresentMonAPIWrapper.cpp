#include "PresentMonAPIWrapper.h"

namespace pmapi
{
    Session::Session()
    {
        if (auto sta = pmOpenSession(); sta != PM_STATUS_SUCCESS) {
            throw SessionException{ std::format("error opening session id={}", (int)sta) };
        }
    }
}
