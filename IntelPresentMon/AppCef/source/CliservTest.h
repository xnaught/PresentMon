#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "../Interprocess/source/act/SymmetricActionClient.h"
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
    struct CefExecutionContext;

    struct CefSessionContext
    {
        std::unique_ptr<SymmetricActionConnector<CefExecutionContext>> pConn;
        pipe::ManualAsyncEvent stopEvt;
        uint32_t remotePid = 0;
        std::optional<uint32_t> lastTokenSeen;
        std::chrono::high_resolution_clock::time_point lastReceived;
        uint32_t receiveCount = 0;
        uint32_t errorCount = 0;
        uint32_t nextCommandToken = 0;
    };

    struct CefExecutionContext
    {
        // types
        using SessionContextType = CefSessionContext;

        // data
        std::optional<uint32_t> responseWriteTimeoutMs;
    };


    struct KernelExecutionContext;

    struct KernelSessionContext
    {
        std::unique_ptr<SymmetricActionConnector<KernelExecutionContext>> pConn;
        uint32_t remotePid = 0;
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



    // helper.h
#include "../Interprocess/source/act/AsyncAction.h"
#include "../CommonUtilities/Macro.h"
#include "../CommonUtilities/log/Log.h"
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/optional.hpp>
#include <format>

#define P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
#ifdef P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
#include "../Interprocess/source/act/AsyncActionCollection.h"
#define ACTION_REG(name) ::pmon::ipc::act::AsyncActionRegistrator<ACT_NS::name, ACT_EXEC_CTX> CONCATENATE(regSvcAct_, name)##_;
#endif

#define ACTION_TRAITS_DEF(name) namespace pmon::ipc::act { template<> struct ActionParamsTraits<ACT_NS::name::Params> { using Action = ACT_NS::name; }; }


    // actions



#define ACT_NAME OpenSession
#define ACT_EXEC_CTX CefExecutionContext
#define ACT_NS ::p2c::client::kact

        using namespace ::pmon::ipc::act;

        class ACT_NAME : public AsyncActionBase_<ACT_NAME, ACT_EXEC_CTX>
        {
        public:
            static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
            struct Params
            {
                uint32_t clientPid;

                template<class A> void serialize(A& ar) {
                    ar(clientPid);
                }
            };
            struct Response {
                std::string yikes;
                uint32_t serverPid;

                template<class A> void serialize(A& ar) {
                    ar(yikes, serverPid);
                }
            };
        private:
            friend class AsyncActionBase_<ACT_NAME, ACT_EXEC_CTX>;
            static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
            {
                stx.remotePid = in.clientPid;
                pmlog_info("listen here you little");
                return Response{ .yikes = "blah", .serverPid = GetCurrentProcessId() };
            }
        };

#ifdef P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
        ACTION_REG(ACT_NAME);
#endif

}

ACTION_TRAITS_DEF(ACT_NAME);
#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS


namespace p2c::client::kact
{

#define ACT_NAME TestAct
#define ACT_EXEC_CTX CefExecutionContext
#define ACT_NS ::p2c::client::kact

    class ACT_NAME : public AsyncActionBase_<ACT_NAME, ACT_EXEC_CTX>
    {
    public:
        static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
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
        friend class AsyncActionBase_<ACT_NAME, ACT_EXEC_CTX>;
        static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
        {
            pmlog_info("got a thing").pmwatch(in.in);
            return Response{ .out = in.in * 2 };
        }
    };

#ifdef P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
    ACTION_REG(ACT_NAME);
#endif
}

ACTION_TRAITS_DEF(ACT_NAME);
#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS


namespace p2c::client::kact
{

#define ACT_NAME TestFromServer
#define ACT_EXEC_CTX KernelExecutionContext
#define ACT_NS ::p2c::client::kact

    class ACT_NAME : public AsyncActionBase_<ACT_NAME, ACT_EXEC_CTX>
    {
    public:
        static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
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
        friend class AsyncActionBase_<ACT_NAME, ACT_EXEC_CTX>;
        static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
        {
            pmlog_info("got a thing qwerty").pmwatch(in.in);
            return Response{ .out = in.in * 2 };
        }
    };

#ifdef P2C_KERNEL_ASYNC_ACTION_REGISTRATION_
    ACTION_REG(ACT_NAME);
#endif
}

ACTION_TRAITS_DEF(ACT_NAME);
#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS



namespace p2c::client::kact
{
    void LaunchServer()
    {
        // launch and detach a thread to run the action server
        std::thread{ [] {
            log::IdentificationTable::AddThisThread("srv-act-test-master");
            SymmetricActionServer<CefExecutionContext> server{ {}, yaboi, 1, "" };
            std::this_thread::sleep_for(2s);
            auto out = server.DispatchSync(TestFromServer::Params{ .in = 420 });
            pmlog_info("henlo").pmwatch(out.out);
            std::this_thread::sleep_for(15s);
        } }.detach();
    }

    void LaunchClientWork()
    {
        std::thread{ [] {
            log::IdentificationTable::AddThisThread("cli-act-test-master");
            SymmetricActionClient<KernelExecutionContext, OpenSession> ac{ yaboi };
            std::this_thread::sleep_for(50ms);
            std::vector<std::jthread> threads;
            for (int i = 0; i < 32; i++) {
                threads.push_back(std::jthread{ [&, tid = i] {
                log::IdentificationTable::AddThisThread(std::format("cli-act-test-{}", tid));
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
            std::this_thread::sleep_for(20s);
        } }.detach();
    }
}