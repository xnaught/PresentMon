#include "ExperimentalInterprocess.h"
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

namespace pmon::ipc::experimental
{
	namespace bip = boost::interprocess;

	using ShmSegment = bip::managed_windows_shared_memory;
	using ShmSegmentManager = ShmSegment::segment_manager;
	using ShmString = bip::basic_string<char, std::char_traits<char>, ShmSegment::allocator<char>::type>;

	class Server : public IServer
	{
	public:
		Server(std::string code)
		{
			// construct string in shm
			auto pMessage = shm.construct<ShmString>(MessageStringName)(shm.get_segment_manager());
			*pMessage = (code + "-served").c_str();
			// construct ptr to string in shm
			shm.construct<bip::offset_ptr<ShmString>>(MessagePtrName)(pMessage);
		}
	private:
		ShmSegment shm{ bip::create_only, SharedMemoryName, 0x10'0000 };
	};

	std::unique_ptr<IServer> IServer::Make(std::string code)
	{
		return std::make_unique<Server>(std::move(code));
	}

	class Client : public IClient
	{
	public:
		Client()
		{
			pMessage = shm.find<ShmString>(IServer::MessageStringName).first;
			ppMessage = shm.find<bip::offset_ptr<ShmString>>(IServer::MessagePtrName).first;
		}
		std::string Read() override
		{
			return pMessage->c_str();
		}
		std::string ReadWithPointer() override
		{
			return ppMessage->get()->c_str();
		}
	private:
		bip::managed_windows_shared_memory shm{ bip::open_only, IServer::SharedMemoryName };
		const ShmString* pMessage = nullptr;
		const bip::offset_ptr<ShmString>* ppMessage = nullptr;
	};

	std::unique_ptr<IClient> IClient::Make()
	{
		return std::make_unique<Client>();
	}


	const char* f()
	{
		return "inter-process-stub";
	}
}