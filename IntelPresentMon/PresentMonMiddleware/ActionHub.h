#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../Interprocess/source/act/AsyncActionManager.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonService/acts/OpenSession.h"

namespace pmon::mid
{
    using namespace util;

    class ActionHub
    {
    public:
        ActionHub(std::string pipeName)
            :
            pipeName_{ std::move(pipeName) },
            pipe_{ pipe::DuplexPipe::Connect(pipeName_, ioctx_) }
        {
            auto fut = pipe::as::co_spawn(ioctx_, OpenSession(), pipe::as::use_future);
            ioctx_.run();
            pmlog_info(fut.get());
        }

        ActionHub(const ActionHub&) = delete;
        ActionHub& operator=(const ActionHub&) = delete;
        ActionHub(ActionHub&&) = delete;
        ActionHub& operator=(ActionHub&&) = delete;
        ~ActionHub() = default;

        pipe::as::awaitable<std::string> OpenSession()
        {
            using Action = pmon::svc::acts::OpenSession;
            const Action::Params p{ .clientPid = 442200 };
            auto res = co_await ipc::act::SyncRequest<Action>(p, 9669, pipe_, pipe_.writeBuf_, pipe_.readBuf_);
            co_return res.str;
        }
    private:
        std::string pipeName_;
        pipe::as::io_context ioctx_;
        pipe::DuplexPipe pipe_;
    };
}