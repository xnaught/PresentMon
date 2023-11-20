#pragma once
#include <memory>
#include <string>
#include <optional>

namespace pmon::ipc
{
	class Base
	{
	public:
		virtual ~Base();
	};

	class ServiceView : public Base
	{

	};

	class MiddlewareView : public Base
	{

	};

	std::unique_ptr<ServiceView> MakeServiceView(std::optional<std::string> sharedMemoryName);
	std::unique_ptr<ServiceView> MakeMiddlewareView(std::optional<std::string> sharedMemoryName);
}