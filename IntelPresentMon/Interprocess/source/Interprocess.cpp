#include "Interprocess.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionHelpers.h"
#include "SharedMemoryTypes.h"

namespace pmon::ipc
{
	namespace bip = boost::interprocess;

	class CommsBase_
	{
	protected:
		static constexpr const char* defaultSegmentName = "presentmon-2-bip-shm";
		static constexpr const char* introspectionRootName = "in-root";
	};

	class ServiceComms_ : public ServiceComms, CommsBase_
	{
	public:
		ServiceComms_(std::optional<std::string> sharedMemoryName)
			:
			shm_{ bip::create_only, sharedMemoryName.value_or(defaultSegmentName).c_str(), 0x10'0000},
			pRoot_{ ShmMakeNamedUnique<intro::IntrospectionRoot>(introspectionRootName, shm_.get_segment_manager(), shm_.get_segment_manager()) }
		{
			// populate introspection data structures at service-side
			auto pSegmentManager = shm_.get_segment_manager();
			intro::PopulateEnums(pSegmentManager, *pRoot_);
			intro::PopulateDevices(pSegmentManager, *pRoot_);
			intro::PopulateMetrics(pSegmentManager, *pRoot_);
		}
		intro::IntrospectionRoot& GetIntrospectionRoot() override
		{
			return *pRoot_;
		}
	private:
		bip::managed_windows_shared_memory shm_;
		ShmUniquePtr<intro::IntrospectionRoot> pRoot_;
	};

	class MiddlewareComms_ : public MiddlewareComms, CommsBase_
	{
	public:
		MiddlewareComms_(std::optional<std::string> sharedMemoryName)
			:
			shm_{ bip::open_only, sharedMemoryName.value_or(defaultSegmentName).c_str() }
		{}
		intro::IntrospectionRoot& GetIntrospectionRoot() override
		{
			const auto result = shm_.find<intro::IntrospectionRoot>(introspectionRootName);
			if (!result.first) {
				throw std::runtime_error{ "Failed to find introspection root in shared memory" };
			}
			return *result.first;
		}
	private:
		bip::managed_windows_shared_memory shm_;
	};

	std::unique_ptr<MiddlewareComms> MakeMiddlewareComms(std::optional<std::string> sharedMemoryName)
	{
		return std::make_unique<MiddlewareComms_>(std::move(sharedMemoryName));
	}

	std::unique_ptr<ServiceComms> MakeServiceComms(std::optional<std::string> sharedMemoryName)
	{
		return std::make_unique<ServiceComms_>(std::move(sharedMemoryName));
	}
}