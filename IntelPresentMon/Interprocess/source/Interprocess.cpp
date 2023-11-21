#include "Interprocess.h"
#include "Introspection.h"
#include <boost/interprocess/managed_windows_shared_memory.hpp>

namespace pmon::ipc
{
	Base::~Base() = default;

	class CommunicationBridge : public ServiceView, public MiddlewareView
	{
	public:
		CommunicationBridge()
		{
			pRoot = std::make_unique<intro::IntrospectionRoot>();
		}
		intro::IntrospectionRoot& GetIntrospectionRoot() override
		{
			return *pRoot;
		}
	private:
		std::unique_ptr<intro::IntrospectionRoot> pRoot;
	};

	std::unique_ptr<MiddlewareView> MakeMiddlewareView(std::optional<std::string> sharedMemoryName)
	{
		return std::make_unique<CommunicationBridge>();
	}
}