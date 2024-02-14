#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include <stdexcept>
#include <string>

namespace pmapi
{
    class Exception : public std::runtime_error { using runtime_error::runtime_error; };
    class ApiErrorException : public Exception {
    public:
        ApiErrorException(PM_STATUS err, const std::string& message = "");
    private:
        // functions
        static std::string BuildWhatString_(PM_STATUS err, const std::string& message);
        // data
        PM_STATUS errorCode_;
    };
    class DatatypeException : public Exception { using Exception::Exception; };
    class LookupException : public Exception { using Exception::Exception; };
}