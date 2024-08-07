#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../Interprocess/source/act/AsyncActionManager.h"
#include "../CommonUtilities/pipe/Pipe.h"
#include "../PresentMonService/acts/OpenSession.h"
#include "../PresentMonService/acts/StartStream.h"
#include "../PresentMonService/acts/GetStaticCpuMetrics.h"
#include "../PresentMonService/acts/EnumerateAdapters.h"
#include "../PresentMonService/acts/SetTelemetryPeriod.h"

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

        auto StartStream(uint32_t targetPid)
        {
            return ExecuteSynchronous_([=, this]() -> pipe::as::awaitable<StartStream::Response> {
                const StartStream::Params p{ .clientPid = thisPid_, .targetPid = targetPid };
                co_return co_await ipc::act::SyncRequest<acts::StartStream>(p, token_++, pipe_, pipe_.writeBuf_, pipe_.readBuf_);
            }());
        }
        auto GetStaticCpuMetrics()
        {
            return ExecuteSynchronous_([=, this]() -> pipe::as::awaitable<GetStaticCpuMetrics::Response> {
                co_return co_await ipc::act::SyncRequest<acts::GetStaticCpuMetrics>(
                    {}, token_++, pipe_, pipe_.writeBuf_, pipe_.readBuf_);
            }());
        }
        auto EnumerateAdapters()
        {
            return ExecuteSynchronous_([=, this]() -> pipe::as::awaitable<EnumerateAdapters::Response> {
                co_return co_await ipc::act::SyncRequest<acts::EnumerateAdapters>(
                    {}, token_++, pipe_, pipe_.writeBuf_, pipe_.readBuf_);
            }());
        }
        auto SetTelemetryPeriod(uint32_t telemetryPeriodMs)
        {
            return ExecuteSynchronous_([=, this]() -> pipe::as::awaitable<SetTelemetryPeriod::Response> {
                co_return co_await ipc::act::SyncRequest<acts::SetTelemetryPeriod>(
                    { telemetryPeriodMs }, token_++, pipe_, pipe_.writeBuf_, pipe_.readBuf_);
            }());
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