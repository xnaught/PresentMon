#include "Session.h"
#include "../PresentMonAPI2/Internal.h"

namespace pmapi
{
    Session::Session()
    {
        if (auto sta = pmOpenSession(&handle_); sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, "error opening session" };
        }
    }

    Session::Session(std::string controlPipe, std::string introspectionNsm)
    {
        if (auto sta = pmOpenSession_(&handle_, controlPipe.c_str(), introspectionNsm.c_str());
            sta != PM_STATUS_SUCCESS) {
            throw ApiErrorException{ sta, std::format("error opening session ctrl={} intro={}",
                controlPipe, introspectionNsm) };
        }
    }
}