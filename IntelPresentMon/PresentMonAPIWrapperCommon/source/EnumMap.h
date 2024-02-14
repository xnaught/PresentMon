#pragma once
#include "../../PresentMonAPI2/source/PresentMonAPI.h"
#include "../../CommonUtilities/source/str/String.h"
#include "Introspection.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <shared_mutex>

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
        static std::shared_ptr<const KeyMap> GetKeyMap(PM_ENUM enumId)
        {
            auto& singleton = Get_();
            std::shared_lock lk{ singleton.mtx_ };
            if (!singleton.initialized_) {
                throw Exception{ "Enum lookup accessed without being initialized" };
            }
            try { return singleton.enumMap_.at(enumId); }
            catch (const std::out_of_range&) {
                throw LookupException{ std::format("Enum lookup failed to find Enum id={}", int(enumId)) };
            }
        }
        static void Refresh(const pmapi::intro::Root& introRoot)
        {
            using ::pmon::util::str::ToWide;
            auto& singleton = Get_();
            std::lock_guard lk{ singleton.mtx_ };
            for (auto e : introRoot.GetEnums()) {
                auto pKeys = std::make_shared<KeyMap>(); auto& keys = *pKeys;
                for (auto k : e.GetKeys()) {
                    keys[k.GetId()] = {
                        .narrowSymbol = k.GetSymbol(),
                        .wideSymbol = ToWide(k.GetSymbol()),
                        .narrowName = k.GetName(),
                        .wideName = ToWide(k.GetName()),
                        .narrowShortName = k.GetShortName(),
                        .wideShortName = ToWide(k.GetShortName()),
                        .narrowDescription = k.GetDescription(),
                        .wideDescription = ToWide(k.GetDescription()),
                    };
                }
                singleton.enumMap_[e.GetId()] = std::move(pKeys);
            }
            singleton.initialized_ = true;
        }
    private:
        static EnumMap& Get_()
        {
            // exception if not initialized
            static EnumMap singleton;
            return singleton;
        }
        std::shared_mutex mtx_;
        std::unordered_map<PM_ENUM, std::shared_ptr<KeyMap>> enumMap_;
        bool initialized_ = false;
    };
}