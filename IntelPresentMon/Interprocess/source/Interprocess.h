#pragma once
#include <optional>
#include <string>
#include <memory>

namespace pmon::ipc
{
	namespace intro
	{
		struct IntrospectionRoot;
	}

	class Base
	{
	public:
		virtual ~Base();
	};

	class ServiceView : public Base
	{
	public:
	};

	class MiddlewareView : public Base
	{
	public:
		virtual intro::IntrospectionRoot& GetIntrospectionRoot() = 0;
	};

	std::unique_ptr<ServiceView> MakeServiceView(std::optional<std::string> sharedMemoryName = {});
	std::unique_ptr<MiddlewareView> MakeMiddlewareView(std::optional<std::string> sharedMemoryName = {});
}