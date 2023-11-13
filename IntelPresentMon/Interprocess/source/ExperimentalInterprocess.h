#pragma once
#include <string>
#include <memory>

namespace pmon::ipc::experimental
{
	class IServer
	{
	public:
		virtual ~IServer() = default;
		static constexpr const char* SharedMemoryName = "MySharedMemory-42069";
		static constexpr const char* MessageStringName = "message-string-777";
		static std::unique_ptr<IServer> Make(std::string code);
	};

	class IClient
	{
	public:
		virtual ~IClient() = default;
		virtual std::string Read() = 0;
		static std::unique_ptr<IClient> Make();
	};


	const char* f();
}