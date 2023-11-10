#include "Interprocess.h"
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

namespace pmon::ipc
{
	namespace bip = boost::interprocess;

	using SegmentManager = bip::managed_windows_shared_memory::segment_manager;
	using CharAllocator = bip::allocator<char, SegmentManager>;
	using MyShmString = bip::basic_string<char, std::char_traits<char>, CharAllocator>;
	using StringAllocator = bip::allocator<MyShmString, SegmentManager>;

	class Server : public IServer
	{
	public:
		Server()
		{
			shm.construct<MyShmString>(MessageStringName)(strAlloc);
		}
	private:
		bip::managed_windows_shared_memory shm{ bip::create_only, SharedMemoryName, 0x10'0000 };
		StringAllocator strAlloc{ shm.get_segment_manager() };
	};

	class Client : public IClient
	{
	public:
		Client()
		{
			pMessage = shm.find<MyShmString>(IServer::MessageStringName).first;
		}
		std::string Read() override
		{
			return pMessage->c_str();
		}
	private:
		bip::managed_windows_shared_memory shm{ bip::open_only, IServer::SharedMemoryName };
		const MyShmString* pMessage = nullptr;
	};

	std::unique_ptr<IServer> IServer::Make()
	{
		return std::make_unique<Server>();
	}

	std::unique_ptr<IClient> IClient::Make()
	{
		return std::make_unique<Client>();
	}


	const char* f()
	{
		return "inter-process-stub";
	}
}