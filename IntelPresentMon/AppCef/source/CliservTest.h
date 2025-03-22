#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "../Interprocess/source/act/SymmetricActionConnector.h"
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <memory>
#include <set>
#include <unordered_map>
#include <string>
#include <optional>
#include <cstdint>
#include <chrono>
#include <random>

using namespace ::pmon::ipc::act;
using namespace as::experimental::awaitable_operators;

namespace p2c::client::kact
{
    struct KernelExecutionContext;

    struct KernelSessionContext
    {
        std::unique_ptr<SymmetricActionConnector<KernelExecutionContext>> pConn;
        uint32_t clientPid = 0;
        std::optional<uint32_t> lastTokenSeen;
        std::chrono::high_resolution_clock::time_point lastReceived;
        uint32_t receiveCount = 0;
        uint32_t errorCount = 0;
        uint32_t nextCommandToken = 0;
    };

    struct KernelExecutionContext
    {
        // types
        using SessionContextType = KernelSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };

    inline std::string yaboi = R"(\\.\pipe\pipe-ya)";

    void LaunchServer()
    {
        // launch and detach a thread to run the action server
        std::thread{ [] {
            SymmetricActionServer<KernelExecutionContext> server{ KernelExecutionContext{}, yaboi, 1, "" };
            std::this_thread::sleep_for(100s);
        } }.detach();
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
            basePipeName_{ std::move(pipeName) },
            asioCloseEvtView_{ ioctx_, winCloseEvt_.Clone().Release() }
        {
            stx_.pConn = SymmetricActionConnector<KernelExecutionContext>::ConnectToServer(basePipeName_, ioctx_);
            as::co_spawn(ioctx_, [this]() -> as::awaitable<void> {
                bool alive = true;
                while (alive) {                    
                    auto res = co_await (stx_.pConn->SyncHandleRequest(ctx_, stx_) ||
                        asioCloseEvtView_.async_wait(as::use_awaitable));
                    alive = res.index() == 0;
                }
            }, as::detached);
            runner_ = std::jthread{ &ActionClient::Run_, this };
            auto res = DispatchSync(OpenSession::Params{
                .clientPid = GetCurrentProcessId(),
            });
            pmlog_info(std::format("Opened session with server, yikes = [{}]", res.yikes));
        }

        ActionClient(const ActionClient&) = delete;
        ActionClient& operator=(const ActionClient&) = delete;
        ActionClient(ActionClient&&) = delete;
        ActionClient& operator=(ActionClient&&) = delete;
        ~ActionClient()
        {
            winCloseEvt_.Set();
        }

        template<class Params>
        auto DispatchSync(Params&& params)
        {
            return stx_.pConn->DispatchSync(std::forward<Params>(params), ioctx_, stx_);
        }

    private:
        // function
        void Run_()
        {
            ioctx_.run();
        }
        // data
        std::string basePipeName_;
        pipe::as::io_context ioctx_;
        SessionContextType stx_;
        KernelExecutionContext ctx_;
        ::pmon::util::win::Event winCloseEvt_;
        as::windows::object_handle asioCloseEvtView_;
        std::jthread runner_;
    };

    void LaunchClientWork()
    {
        std::thread{ [] {
            ActionClient ac{ yaboi };
            std::this_thread::sleep_for(50ms);
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
            std::this_thread::sleep_for(10s);
        } }.detach();
    }
}