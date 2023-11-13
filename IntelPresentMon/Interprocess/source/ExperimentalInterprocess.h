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
		static constexpr const char* MessagePtrName = "message-ptr-787";
		static constexpr const char* MessageUptrName = "message-uptr-57";
		virtual void MakeUptrToMessage(std::string code) = 0;
		virtual void FreeUptrToMessage() = 0;
		static std::unique_ptr<IServer> Make(std::string code);
	};

	class IClient
	{
	public:
		virtual ~IClient() = default;
		virtual size_t GetFreeMemory() const = 0;
		virtual std::string Read() const = 0;
		virtual std::string ReadWithPointer() const = 0;
		virtual std::string ReadWithUptr() = 0;
		static std::unique_ptr<IClient> Make();
	};


	const char* f();
}