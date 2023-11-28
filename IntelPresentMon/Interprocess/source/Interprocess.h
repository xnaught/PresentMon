#pragma once
#include <optional>
#include <string>
#include <memory>

struct PM_INTROSPECTION_ROOT;

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
		virtual const PM_INTROSPECTION_ROOT* GetIntrospectionRoot() = 0;
	};

	std::unique_ptr<ServiceComms> MakeServiceComms(std::optional<std::string> sharedMemoryName = {});
	std::unique_ptr<MiddlewareComms> MakeMiddlewareComms(std::optional<std::string> sharedMemoryName = {});
}