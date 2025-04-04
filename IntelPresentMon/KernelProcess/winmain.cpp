#include "../CommonUtilities/win/WinAPI.h"
#include <boost/process.hpp>
#include "../Core/source/kernel/Kernel.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "kact/KernelExecutionContext.h"
#include "../AppCef/source/util/cact/TargetLostAction.h"
#include "../AppCef/source/util/cact/OverlayDiedAction.h"
#include "../AppCef/source/util/cact/PresentmonInitFailedAction.h"
#include "../AppCef/source/util/cact/StalePidAction.h"


using namespace pmon;

namespace kproc
{
	using KernelServer = ipc::act::SymmetricActionServer<kact::KernelExecutionContext>;

	class KernelHandler : public p2c::kern::KernelHandler
	{
    public:
        KernelHandler(KernelServer* pServer) : pServer_{ pServer } {}
        void OnTargetLost(uint32_t pid) override
        {
            pServer_->DispatchAsync(p2c::client::util::cact::TargetLostAction::Params{ pid });
        }
        void OnOverlayDied() override
        {
            pServer_->DispatchAsync(p2c::client::util::cact::OverlayDiedAction::Params{});
        }
        void OnPresentmonInitFailed() override
        {
            pServer_->DispatchAsync(p2c::client::util::cact::PresentmonInitFailedAction::Params{});
        }
        void OnStalePidSelected() override
        {
            pServer_->DispatchAsync(p2c::client::util::cact::StalePidAction::Params{});
        }
    private:
        // data
        KernelServer* pServer_;
	};
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    using namespace kproc;

    p2c::kern::Kernel* pKernel = nullptr;
    KernelServer server{ kact::KernelExecutionContext{ .ppKernel = &pKernel },
        std::format(R"(\\.\pipe\ipm-cef-channel-{})", GetCurrentProcessId()), 1, "" };
    KernelHandler kernHandler{ &server };
    p2c::kern::Kernel kernel{ &kernHandler };

	boost::process::child childCef{
		"PresentMon.exe",
		"--p2c-svc-as-child",
		"--p2c-files-working",
		"--p2c-log-level", "debug",
		"--p2c-svc-pipe-enable",
		"--p2c-middleware-dll-path", "PresentMonAPI2.dll",
		"--p2c-log-middleware-copy",
		"--p2c-enable-ui-dev-options"
	};
	childCef.wait();

	return 0;
}