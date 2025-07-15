#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/str/String.h"
#include "../CommonUtilities/win/Utilities.h"
#include "CliOptions.h"

#include <set>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <thread>
#include <format>

#include "LibraryInject.h"
#include "Logging.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"

namespace stdfs = std::filesystem;
using namespace pmon::util;
using namespace std::literals;

// null logger factory to satisfy linking requirements for CommonUtilities
namespace pmon::util::log
{
    std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
    {
        return {};
    }
}

int main(int argc, char** argv)
{
    // Initialize arguments
    if (auto res = clio::Options::Init(argc, argv)) {
        return *res;
    }
    auto& opts = clio::Options::Get();

    stdfs::path injectorPath;
    {
        std::vector<char> buffer(MAX_PATH);
        auto size = GetModuleFileNameA(NULL, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (size == 0) {
            LOGE << "Failed to get this executable path.";
        }
        injectorPath = std::string(buffer.begin(), buffer.begin() + size);
        injectorPath = injectorPath.parent_path();
    }

    // DLL to inject
    const stdfs::path libraryPath = injectorPath / std::format("FlashInjectorLibrary-{}.dll", PM_BUILD_PLATFORM);

    const bool weAre32Bit = PM_BUILD_PLATFORM == "Win32"s;

    if (!stdfs::exists(libraryPath)) {
        LOGE << "Cannot find library: " << libraryPath;
        exit(1);
    }

    LOGI << "Waiting for processes that match executable name...";

    std::mutex nameMutex;
    std::string targetModuleName;

    std::thread{ [&] {
        std::string line;
        while (true) {
            std::getline(std::cin, line);
            std::lock_guard lk{ nameMutex };
            targetModuleName = str::ToLower(line);
        }
    } }.detach();

    std::unordered_set<DWORD> processesAttached;
    while (true) {
        const auto tgt = [&] {
            std::lock_guard lk{ nameMutex };
            return targetModuleName;
        }();
        if (!tgt.empty()) {
            auto processes = LibraryInject::GetProcessNames();
            for (auto&& [processId, processName] : processes) {
                const auto processNameLower = str::ToLower(processName);
                if (processNameLower == tgt && !processesAttached.contains(processId)) {
                    auto hProcTarget = win::OpenProcess(processId, PROCESS_QUERY_LIMITED_INFORMATION);
                    if (win::ProcessIs32Bit(hProcTarget) == weAre32Bit) {
                        LOGI << "    Injecting DLL to process with PID: " << processId;
                        LibraryInject::Attach(processId, libraryPath);
                        processesAttached.insert(processId);
                        // inform kernel of attachment so it can connect the action client
                        std::cout << processId << std::endl;
                    }
                }
            }
        }
        // check for new matching processes every n milliseconds
        std::this_thread::sleep_for(50ms);
    }

    return 0;
}