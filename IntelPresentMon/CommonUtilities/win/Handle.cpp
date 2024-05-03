#include "Handle.h"
#include "WinAPI.h"
#include <stdexcept>


namespace pmon::util::win
{
    Handle::Handle() = default;
    Handle::Handle(HandleType handle) : handle_{ handle } {}
    Handle::~Handle()
    {
        try { Clear(); }
        catch (...) {
            // TODO: consider logging here in some builds / modes, but
            // caution required since this is used by logging components themselves
        }
    }
    Handle::Handle(Handle&& other) noexcept
        :
        handle_(other.handle_)
    {
        other.handle_ = nullptr;
    }
    Handle& Handle::operator=(Handle&& other) noexcept
    {
        if (this != &other) {
            Clear();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    Handle::operator HandleType() const
    {
        return handle_;
    }
    Handle::HandleType Handle::Get() const
    {
        return handle_;
    }
    void Handle::Clear()
    {
        if (*this) {
            if (!CloseHandle(handle_)) {
                // TODO: throw custom exception, perhaps with more context / error code / formatted error
                throw std::runtime_error{ "Failed closing handle in Handle wrapper" };
            }
            handle_ = nullptr;
        }
    }
    Handle::HandleType Handle::Release()
    {
        const auto temp = handle_;
        Clear();
        return temp;
    }
    Handle::operator bool() const noexcept
    {
        return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE;
    }
}