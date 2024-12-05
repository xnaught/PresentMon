#pragma once
#include <unordered_map>
#include <functional>
#include <string>
#include <typeindex>
#include <sstream>
#include "../GeneratedReflection.h"

// target includes

#include "../../../../Reflector/Test1.h"

#include "../../../../IntelPresentMon/ControlLib/igcl_api.h"


namespace pmon::util::ref::gen
{
	void RegisterDumpers_(std::unordered_map<std::type_index, std::function<std::string(const void*)>>& dumpers)
	{

		dumpers[typeid(B)] = [](const void* pStruct) {
			const auto& s = *static_cast<const B*>(pStruct);
			std::ostringstream oss;
			oss << "struct B {"

				<< " .fff = " << s.fff

				<< " }";
			return oss.str();
		};

		dumpers[typeid(AAA)] = [](const void* pStruct) {
			const auto& s = *static_cast<const AAA*>(pStruct);
			std::ostringstream oss;
			oss << "struct AAA {"

				<< " .x = " << s.x

				<< " .foo = " << s.foo

				<< " .barff = " << DumpGenerated(s.barff)

				<< " }";
			return oss.str();
		};

		dumpers[typeid(ctl_application_id_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ctl_application_id_t*>(pStruct);
			std::ostringstream oss;
			oss << "struct ctl_application_id_t {"

				<< " .Data1 = " << s.Data1

				<< " .Data2 = " << s.Data2

				<< " .Data3 = " << s.Data3

				<< " .Data4 = " << "{ unsupported }"

				<< " }";
			return oss.str();
		};

		dumpers[typeid(ctl_init_args_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ctl_init_args_t*>(pStruct);
			std::ostringstream oss;
			oss << "struct ctl_init_args_t {"

				<< " .Size = " << s.Size

				<< " .Version = " << s.Version

				<< " .AppVersion = " << s.AppVersion

				<< " .flags = " << s.flags

				<< " .SupportedVersion = " << s.SupportedVersion

				<< " .ApplicationUID = " << DumpGenerated(s.ApplicationUID)

				<< " }";
			return oss.str();
		};

		dumpers[typeid(ctl_firmware_version_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ctl_firmware_version_t*>(pStruct);
			std::ostringstream oss;
			oss << "struct ctl_firmware_version_t {"

				<< " .major_version = " << s.major_version

				<< " .minor_version = " << s.minor_version

				<< " .build_number = " << s.build_number

				<< " }";
			return oss.str();
		};

		dumpers[typeid(ctl_adapter_bdf_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ctl_adapter_bdf_t*>(pStruct);
			std::ostringstream oss;
			oss << "struct ctl_adapter_bdf_t {"

				<< " .bus = " << s.bus

				<< " .device = " << s.device

				<< " .function = " << s.function

				<< " }";
			return oss.str();
		};

		dumpers[typeid(ctl_device_adapter_properties_t)] = [](const void* pStruct) {
			const auto& s = *static_cast<const ctl_device_adapter_properties_t*>(pStruct);
			std::ostringstream oss;
			oss << "struct ctl_device_adapter_properties_t {"

				<< " .Size = " << s.Size

				<< " .Version = " << s.Version

				<< " .pDeviceID = " << "{ unsupported }"

				<< " .device_id_size = " << s.device_id_size

				<< " .device_type = " << "{ unsupported }"

				<< " .supported_subfunction_flags = " << s.supported_subfunction_flags

				<< " .driver_version = " << s.driver_version

				<< " .firmware_version = " << DumpGenerated(s.firmware_version)

				<< " .pci_vendor_id = " << s.pci_vendor_id

				<< " .pci_device_id = " << s.pci_device_id

				<< " .rev_id = " << s.rev_id

				<< " .num_eus_per_sub_slice = " << s.num_eus_per_sub_slice

				<< " .num_sub_slices_per_slice = " << s.num_sub_slices_per_slice

				<< " .num_slices = " << s.num_slices

				<< " .name = " << s.name

				<< " .graphics_adapter_properties = " << s.graphics_adapter_properties

				<< " .Frequency = " << s.Frequency

				<< " .pci_subsys_id = " << s.pci_subsys_id

				<< " .pci_subsys_vendor_id = " << s.pci_subsys_vendor_id

				<< " .adapter_bdf = " << DumpGenerated(s.adapter_bdf)

				<< " .reserved = " << s.reserved

				<< " }";
			return oss.str();
		};

	}
}
