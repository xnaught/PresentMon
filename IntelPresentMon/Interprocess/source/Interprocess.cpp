#include "Interprocess.h"
#include "IntrospectionTransfer.h"
#include "IntrospectionHelpers.h"
#include "SharedMemoryTypes.h"

namespace pmon::ipc
{
	namespace bip = boost::interprocess;

	Base::~Base() = default;

	class CommunicationBridge : public ServiceView, public MiddlewareView
	{
	public:
		CommunicationBridge()
			:
			shm{ bip::create_only, "my-shm-1111", 0x10'0000 },
			pRoot{ ShmMakeUnique<intro::IntrospectionRoot>(shm.get_segment_manager(), shm.get_segment_manager()) }
		{
			// populate introspection data structures
			auto pSegmentManager = shm.get_segment_manager();
			intro::PopulateEnums(pSegmentManager, *pRoot);
			intro::PopulateDevices(pSegmentManager, *pRoot);
			intro::PopulateMetrics(pSegmentManager, *pRoot);
		}
		intro::IntrospectionRoot& GetIntrospectionRoot() override
		{
			return *pRoot;
		}
	private:
		bip::managed_windows_shared_memory shm;
		ShmUniquePtr<intro::IntrospectionRoot> pRoot;
	};

	std::unique_ptr<MiddlewareView> MakeMiddlewareView(std::optional<std::string> sharedMemoryName)
	{
		return std::make_unique<CommunicationBridge>();
	}
}