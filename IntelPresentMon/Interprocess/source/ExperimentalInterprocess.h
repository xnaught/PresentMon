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
		static constexpr const char* RootPtrName = "root-ptr-157";
		virtual void MakeUptrToMessage(std::string code) = 0;
		virtual void FreeUptrToMessage() = 0;
		virtual void MakeRoot(int x) = 0;
		virtual void FreeRoot() = 0;
		virtual int RoundtripRootInShared() = 0;
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
		virtual int ReadRoot() = 0;
		virtual int RoundtripRootInHeap() const = 0;
		static std::unique_ptr<IClient> Make();
	};


	const char* f();
}