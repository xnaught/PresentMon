#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/log/StructDump.h"
#include "../Interprocess/source/act/AsyncActionManager.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonService/AllActions.h"
#include "../Versioning/BuildId.h"

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
            auto res = DispatchSync(OpenSession::Params{
                .clientPid = thisPid_, .clientBuildId = bid::BuildIdShortHash()
            });
            pmlog_info(std::format("Opened session with server, build id = [{}]", res.serviceBuildId));
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
                co_return co_await ipc::act::SyncRequest<Action>(std::forward<Params>(params), token_++, pipe_, timeoutMs_);
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
        // data
        uint32_t timeoutMs_ = 1000;
        uint32_t token_ = 0;
        uint32_t thisPid_;
        std::string pipeName_;
        pipe::as::io_context ioctx_;
        pipe::DuplexPipe pipe_;
    };
}