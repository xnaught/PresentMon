#pragma once
#include <stdexcept>
#include "../../PresentMonAPI2/source/PresentMonAPI.h"

namespace pmapi
{
    class Exception : public std::runtime_error { using runtime_error::runtime_error; };
    struct ApiErrorException : public Exception {
        ApiErrorException(PM_STATUS err, std::string message = "")
            : Exception{ std::move(message) }, errorCode{ err } {}
        PM_STATUS errorCode;
    };
    class DatatypeException : public Exception { using Exception::Exception; };
    class LookupException : public Exception { using Exception::Exception; };
}