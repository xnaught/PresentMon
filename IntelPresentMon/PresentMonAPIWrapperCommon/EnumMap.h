#pragma once
#include "../PresentMonAPI2/PresentMonAPI.h"
#include "../CommonUtilities/str/String.h"
#include "Introspection.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <shared_mutex>
#include <atomic>

namespace pmapi
{
    struct EnumKeyStrings
    {
        std::string narrowSymbol;
        std::wstring wideSymbol;
        std::string narrowName;
        std::wstring wideName;
        std::string narrowShortName;
        std::wstring wideShortName;
        std::string narrowDescription;
        std::wstring wideDescription;
    };

    class EnumMap
    {
    public:
        using KeyMap = std::unordered_map<int, EnumKeyStrings>;
        static std::shared_ptr<const KeyMap> GetKeyMap(PM_ENUM enumId);
        static void Refresh(const pmapi::intro::Root& introRoot);
        static bool Initialized();
    private:
        // functions
        static EnumMap& Get_();
        // data
        std::shared_mutex mtx_;
        std::unordered_map<PM_ENUM, std::shared_ptr<KeyMap>> enumMap_;
        std::atomic<bool> initialized_ = false;
    };
}