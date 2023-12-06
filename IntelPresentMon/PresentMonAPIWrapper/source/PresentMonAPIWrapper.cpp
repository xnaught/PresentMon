#include "PresentMonAPIWrapper.h"
#include "../../PresentMonAPI2/source/Internal.h"

namespace pmapi
{
    Session::Session()
    {
        if (auto sta = pmOpenSession(); sta != PM_STATUS_SUCCESS) {
            throw SessionException{ std::format("error opening session errid={}", (int)sta) };
        }
    }

    Session::Session(std::string controlPipe, std::string introspectionNsm)
    {
        if (auto sta = pmOpenSession_(controlPipe.c_str(), introspectionNsm.c_str());
            sta != PM_STATUS_SUCCESS) {
            throw SessionException{ std::format("error opening session errid={} ctrl={} intro={}",
                (int)sta, controlPipe, introspectionNsm) };
        }
    }
}
