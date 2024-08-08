#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/log/StructDump.h"
#include "../Interprocess/source/act/AsyncActionManager.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonService/AllActions.h"

namespace pmon::mid
{
    using namespace util;
    using namespace svc;
    using namespace acts;

    class ActionClient
    {
    public:
        ActionClient(std::string pipeName)
            :
            thisPid_{ GetCurrentProcessId() },
            pipeName_{ std::move(pipeName) },
            pipe_{ pipe::DuplexPipe::Connect(pipeName_, ioctx_) }
        {
            pmlog_info(ExecuteSynchronous_(OpenSession_()));
        }

        ActionClient(const ActionClient&) = delete;
        ActionClient& operator=(const ActionClient&) = delete;
        ActionClient(ActionClient&&) = delete;
        ActionClient& operator=(ActionClient&&) = delete;
        ~ActionClient() = default;

        template<class Params>
        auto DispatchSync(Params&& params)
        {
            using Action = typename ipc::act::ActionParamsTraits<std::decay_t<Params>>::Action;
            using Response = Action::Response;
            pmlog_dbg(std::format("dispatching action [{}] w/ params: {}", Action::Identifier, log::DumpStruct(params)));
            return ExecuteSynchronous_([this](Params&& params) -> pipe::as::awaitable<Response> {
                co_return co_await ipc::act::SyncRequest<Action>(std::forward<Params>(params),
                    token_++, pipe_, pipe_.writeBuf_, pipe_.readBuf_);
            }(std::forward<Params>(params)));
        }

    private:
        // functions
        template<class C>
        auto ExecuteSynchronous_(C&& coro)
        {
            auto fut = pipe::as::co_spawn(ioctx_, std::forward<C>(coro), pipe::as::use_future);
            ioctx_.run();
            ioctx_.restart();
            return fut.get();
        }
        // TODO: roll this up with Dispatch (when we remove the placeholder logging)
        pipe::as::awaitable<std::string> OpenSession_()
        {
            const OpenSession::Params p{ .clientPid = thisPid_ };
            auto res = co_await ipc::act::SyncRequest<OpenSession>(p, token_++, pipe_, pipe_.writeBuf_, pipe_.readBuf_);
            co_return res.str;
        }
        // data
        uint32_t token_ = 0;
        uint32_t thisPid_;
        std::string pipeName_;
        pipe::as::io_context ioctx_;
        pipe::DuplexPipe pipe_;
    };
}