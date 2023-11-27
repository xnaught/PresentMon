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

	class ServiceComms
	{
	public:
		virtual ~ServiceComms() = default;
		virtual intro::IntrospectionRoot& GetIntrospectionRoot() = 0;
	};

	class MiddlewareComms
	{
	public:
		virtual ~MiddlewareComms() = default;
		virtual intro::IntrospectionRoot& GetIntrospectionRoot() = 0;
	};

	std::unique_ptr<ServiceComms> MakeServiceComms(std::optional<std::string> sharedMemoryName = {});
	std::unique_ptr<MiddlewareComms> MakeMiddlewareComms(std::optional<std::string> sharedMemoryName = {});
}