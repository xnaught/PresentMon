#include "../CommonUtilities/win/WinAPI.h"
#include <boost/process.hpp>
#include "../Core/source/kernel/Kernel.h"
#include "../Core/source/infra/util/FolderResolver.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "kact/KernelExecutionContext.h"
#include "../AppCef/source/util/cact/TargetLostAction.h"
#include "../AppCef/source/util/cact/OverlayDiedAction.h"
#include "../AppCef/source/util/cact/PresentmonInitFailedAction.h"
#include "../AppCef/source/util/cact/StalePidAction.h"
#include <Core/source/cli/CliOptions.h>
#include <PresentMonAPI2Loader/Loader.h>
#include <Core/source/infra/LogSetup.h>
#include <CommonUtilities/win/Utilities.h>
#include <Shobjidl.h>
#include <array>
#include <ranges>


using namespace pmon;
namespace vi = std::views;
namespace rn = std::ranges;
using namespace std::literals;

namespace kproc
{
	using KernelServer = ipc::act::SymmetricActionServer<kact::KernelExecutionContext>;

	class KernelHandler : public p2c::kern::KernelHandler
	{
    public:
        KernelHandler(KernelServer& server) : server_{ server } {}
        void OnTargetLost(uint32_t pid) override
        {
            server_.DispatchAsync(p2c::client::util::cact::TargetLostAction::Params{ pid });
        }
        void OnOverlayDied() override
        {
            server_.DispatchAsync(p2c::client::util::cact::OverlayDiedAction::Params{});
        }
        void OnPresentmonInitFailed() override
        {
            server_.DispatchAsync(p2c::client::util::cact::PresentmonInitFailedAction::Params{});
        }
        void OnStalePidSelected() override
        {
            server_.DispatchAsync(p2c::client::util::cact::StalePidAction::Params{});
        }
    private:
        // data
        KernelServer& server_;
	};
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    using namespace kproc;
    using namespace p2c;

#ifdef NDEBUG
    constexpr bool is_debug = false;
#else
    constexpr bool is_debug = true;
#endif
    // parse the command line arguments and make them globally available
    if (auto err = cli::Options::Init(__argc, __argv, true)) {
        if (*err == 0) {
            MessageBoxA(nullptr, cli::Options::GetDiagnostics().c_str(), "Command Line Help",
                MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND);
        }
        else {
            MessageBoxA(nullptr, cli::Options::GetDiagnostics().c_str(), "Command Line Parse Error",
                MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);
        }
        return *err;
    }
    const auto& opt = cli::Options::Get();
    if (opt.filesWorking) {
        infra::util::FolderResolver::SetDevMode();
    }
    // optionally override the middleware dll path (typically when running from IDE in dev cycle)
    if (auto path = opt.middlewareDllPath.AsOptional()) {
        pmLoaderSetPathToMiddlewareDll_(path->c_str());
    }

    // create logging system and ensure cleanup before main ext
    LogChannelManager zLogMan_;

    // configure the logging system (partially based on command line options)
    ConfigureLogging();

    // set the app id so that windows get grouped
    SetCurrentProcessExplicitAppUserModelID(L"Intel.PresentMon");

    // launch the service as a child process if desired (typically during development)
    boost::process::child childSvc;
    if (opt.svcAsChild) {
        using namespace std::literals;
        namespace bp = boost::process;

        //logSvcPipe = opt.logSvcPipe.AsOptional().value_or(
        //    std::format("pm2-child-svc-log-{}", GetCurrentProcessId()));
        childSvc = boost::process::child{
            "PresentMonService.exe"s,
            "--control-pipe"s, *opt.controlPipe,
            "--nsm-prefix"s, "pm-frame-nsm"s,
            "--intro-nsm"s, *opt.shmName,
            "--etw-session-name"s, *opt.etwSessionName,
            "--log-level"s, std::to_string((int)util::log::GlobalPolicy::Get().GetLogLevel()),
            // "--log-pipe-name"s, *logSvcPipe,
            "--enable-stdio-log",
        };

        if (!::pmon::util::win::WaitForNamedPipe(*opt.controlPipe + "-in", 1500)) {
            pmlog_error("timeout waiting for child service control pipe to go online");
            return -1;
        }
    }

    // this pointer serves as a way to set the kernel on the server exec context after the server is created
    p2c::kern::Kernel* pKernel = nullptr;
    // this server receives a connection from the CEF render process
    const auto actName = std::format(R"(\\.\pipe\ipm-cef-channel-{})", GetCurrentProcessId());
    KernelServer server{ kact::KernelExecutionContext{ .ppKernel = &pKernel }, actName, 1, "" };
    // this handler receives events from the kernel and transmits them to the render process via the server
    KernelHandler kernHandler{ server };
    // the kernel manages the PresentMon data collection and the overlay rendering
    p2c::kern::Kernel kernel{ &kernHandler };
    // new we set this pointer, giving the server access to the Kernel
    pKernel = &kernel;

    // compose optional cli args for cef process tree
    auto args = std::vector<std::string>{
        opt.filesWorking ? "--p2c-files-working"s : ""s,
        opt.traceExceptions ? "--p2c-trace-exceptions"s : ""s,
        opt.logFolder ? "--p2c-log-folder"s : "", *opt.logFolder,
    } | vi::filter(std::not_fn(&std::string::empty)) | rn::to<std::vector>();
    for (auto& f : *opt.uiFlags) {
        args.push_back("--p2c-" + f);
    }
    for (auto& o : *opt.uiOptions) {
        args.push_back("--p2c-" + o.first);
        args.push_back(o.second);
    }
    // launch the CEF browser process, which in turn launches all the other processes in the CEF process constellation
	boost::process::child childCef{
		"PresentMonUI.exe",
        "--p2c-log-level"s, util::log::GetLevelName(*opt.logLevel),
        "--p2c-log-trace-level"s, util::log::GetLevelName(*opt.logTraceLevel),
        "--p2c-act-name", actName,
        boost::process::args(args),
	};
	childCef.wait();

	return 0;
}