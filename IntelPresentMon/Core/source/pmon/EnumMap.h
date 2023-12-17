#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <PresentMonAPI/PresentMonAPI.h>
#include <PresentMonAPI2/source/PresentMonAPI.h>
#include <PresentMonAPIWrapper/source/PresentMonAPIWrapper.h>
#include <CommonUtilities/source/str/String.h>

namespace p2c::pmon
{
	class EnumMap
	{
	public:
		using KeyMap = std::unordered_map<int, std::wstring>;
        static const KeyMap* GetMapPtr(PM_ENUM enumId)
        {
            return Get_().enumMap_[enumId].get();
        }
        static void Init(const pmapi::intro::Root& introRoot)
        {
            auto& singleton = Get_();
            for (auto e : introRoot.GetEnums()) {
                auto pKeys = std::make_unique<KeyMap>(); auto& keys = *pKeys;
                for (auto k : e.GetKeys()) {
                    keys[k.GetId()] = ::pmon::util::str::ToWide(k.GetName());
                }
                singleton.enumMap_[e.GetId()] = std::move(pKeys);
            }
        }
	private:
        static EnumMap& Get_()
        {
            static EnumMap singleton;
            return singleton;
        }
		std::unordered_map<PM_ENUM, std::unique_ptr<KeyMap>> enumMap_;
	};
}