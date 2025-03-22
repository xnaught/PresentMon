#include "../CommonUtilities/pipe/Pipe.h"
#include "../Interprocess/source/act/ActionServer.h"
#include "../CommonUtilities/pipe/CoroMutex.h"
#include <memory>
#include <set>
#include <unordered_map>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include <random>


namespace p2c::client::kact
{
    struct ActionSession
    {
        std::shared_ptr<::pmon::util::pipe::DuplexPipe> pPipe;
        uint32_t clientPid = 0;
        std::optional<uint32_t> lastTokenSeen;
        std::chrono::high_resolution_clock::time_point lastReceived;
        uint32_t receiveCount = 0;
        uint32_t errorCount = 0;
    };

    struct KernelExecutionContext
    {
        // types
        using SessionContextType = ActionSession;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };

    inline std::string yaboi = R"(\\.\pipe\pipe-ya)";

    void LaunchServer()
    {
        // launch and detach a thread to run the action server
        ::pmon::ipc::act::ActionServerImpl_<KernelExecutionContext>::LaunchThread(
            KernelExecutionContext{}, yaboi, 1, ""
        ).detach();
    }



    // helper.h
#include "../Interprocess/source/act/AsyncAction.h"
#include "../CommonUtilities/Macro.h"
#include "../CommonUtilities/log/Log.h"
//#include "ServiceExecutionContext.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/optional.hpp>
#include <format>

#define P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
#ifdef P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
#include "../Interprocess/source/act/AsyncActionCollection.h"
#define ACTION_REG(name) ::pmon::ipc::act::AsyncActionRegistrator<::p2c::client::kact::name, ::p2c::client::kact::KernelExecutionContext> CONCATENATE(regSvcAct_, name)##_;
#endif

#define ACTION_TRAITS_DEF(name) namespace pmon::ipc::act { template<> struct ActionParamsTraits<::p2c::client::kact::name::Params> { using Action = ::p2c::client::kact::name; }; }



    // actions

#define ACTNAME OpenSession

        using namespace ::pmon::ipc::act;

        class ACTNAME : public AsyncActionBase_<ACTNAME, KernelExecutionContext>
        {
        public:
            static constexpr const char* Identifier = STRINGIFY(ACTNAME);
            struct Params
            {
                uint32_t clientPid;

                template<class A> void serialize(A& ar) {
                    ar(clientPid);
                }
            };
            struct Response {
                std::string yikes;

                template<class A> void serialize(A& ar) {
                    ar(yikes);
                }
            };
        private:
            friend class AsyncActionBase_<ACTNAME, KernelExecutionContext>;
            static Response Execute_(const KernelExecutionContext& ctx, SessionContext& stx, Params&& in)
            {
                stx.clientPid = in.clientPid;
                pmlog_info("listen here you little");
                return Response{ .yikes = "blah" };
            }
        };

#ifdef P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
        ACTION_REG(ACTNAME);
#endif

}

ACTION_TRAITS_DEF(ACTNAME);
#undef ACTNAME


namespace p2c::client::kact
{

#define ACTNAME TestAct
    class ACTNAME : public AsyncActionBase_<ACTNAME, KernelExecutionContext>
    {
    public:
        static constexpr const char* Identifier = STRINGIFY(ACTNAME);
        struct Params
        {
            uint32_t in;

            template<class A> void serialize(A& ar) {
                ar(in);
            }
        };
        struct Response {
            uint32_t out;

            template<class A> void serialize(A& ar) {
                ar(out);
            }
        };
    private:
        friend class AsyncActionBase_<ACTNAME, KernelExecutionContext>;
        static Response Execute_(const KernelExecutionContext& ctx, SessionContext& stx, Params&& in)
        {
            pmlog_info("got a thing").pmwatch(in.in);
            return Response{ .out = in.in * 2 };
        }
    };

#ifdef P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
    ACTION_REG(ACTNAME);
#endif
}

ACTION_TRAITS_DEF(ACTNAME);
#undef ACTNAME



namespace p2c::client::kact
{
    class ActionClient
    {
        using ExecCtx = KernelExecutionContext;
        using SessionContextType = typename ExecCtx::SessionContextType;
    public:
        ActionClient(std::string pipeName)
            :
            thisPid_{ GetCurrentProcessId() },
            pipeName_{ std::move(pipeName) },
            pipe_{ pipe::DuplexPipe::Connect(pipeName_, ioctx_) },
            workGuard_{ as::make_work_guard(ioctx_) }
        {
            std::thread{ &ActionClient::Run, this }.detach();
            auto res = DispatchSync(OpenSession::Params{
                .clientPid = thisPid_,
            });
            pmlog_info(std::format("Opened session with server, yikes = [{}]", res.yikes));
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
            return as::co_spawn(ioctx_,
                ipc::act::SyncRequest<Action>(std::forward<Params>(params), token_++, pipe_),
                as::use_future).get();
        }
        void Run()
        {
            ioctx_.run();
        }

    private:
        // function
        void DisposeSession_(uint32_t sessionId)
        {
            pmlog_dbg(std::format("Disposing kact session id:{}", sessionId));
            if (auto i = sessions_.find(sessionId); i != sessions_.end()) {
                sessions_.erase(i);
            }
            else {
                pmlog_warn("kact Session to be removed not found");
            }
        }
        // data
        uint32_t timeoutMs_ = 1000;
        uint32_t token_ = 0;
        uint32_t thisPid_;
        std::string pipeName_;
        pipe::as::io_context ioctx_;
        pipe::DuplexPipe pipe_;
        as::executor_work_guard<as::io_context::executor_type> workGuard_;
        // maps session uid => session (uid is same as session pipe id)
        std::unordered_map<uint32_t, SessionContextType> sessions_;
    };

    void LaunchClientWork()
    {
        std::thread{ [] {
            ActionClient ac{ yaboi };
            std::vector<std::jthread> threads;
            for (int i = 0; i < 32; i++) {
                threads.push_back(std::jthread{ [&, tid = i] {
                    std::minstd_rand0 rne{ std::random_device{}() };
                    std::uniform_int_distribution<uint32_t> dist{ 1, 1000 };
                    for (int i = 0; i < 250; i++) {
                        const auto in = dist(rne);
                        const auto [out] = ac.DispatchSync(TestAct::Params{ in });
                        pmlog_info("action run").pmwatch(tid).pmwatch(i).pmwatch(in).pmwatch(out);
                        if (out != in * 2) {
                            DebugBreak();
                        }
                        std::this_thread::sleep_for(1ms);
                    }
                } });
            }
        } }.detach();
    }
}