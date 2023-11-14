#include "ExperimentalInterprocess.h"
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <memory>

namespace pmon::ipc::experimental
{
	namespace bip = boost::interprocess;

	using ShmSegment = bip::managed_windows_shared_memory;
	using ShmSegmentManager = ShmSegment::segment_manager;
	template<typename T>
	using ShmAllocator = ShmSegment::allocator<T>::type;
	using ShmString = bip::basic_string<char, std::char_traits<char>, ShmAllocator<char>>;
	template<typename T>
	using UptrDeleter = bip::deleter<T, ShmSegmentManager>;
	template<typename T>
	using Uptr = bip::unique_ptr<T, UptrDeleter<T>>;

	template<class A>
	struct AllocatorDeleter
	{
		using T = typename A::value_type;
		A allocator;
		AllocatorDeleter(A allocator_) : allocator{ std::move(allocator_) } {}
		void operator()(T* ptr) { allocator.deallocate(ptr, 1u); }
	};

	template<template <class> typename T, class A>
	using UptrT = bip::unique_ptr<T<A>, AllocatorDeleter<typename T<A>::Allocator>>;


	//template<class A>
	//class Leaf
	//{
	//	using Allocator = std::allocator_traits<A>::template rebind_alloc<Leaf>;
	//private:

	//};

	template<template <class> typename T, class A>
	auto MakeUnique(A allocator_in)
	{
		// allocator type for creating the object to be managed by the uptr
		using Allocator = typename T<A>::Allocator;
		// deleter type for the uptr (derived from the allocator of the managed object)
		using Deleter = typename UptrT<T, A>::deleter_type;
		// create allocator for managed object
		Allocator allocator(std::move(allocator_in));
		// allocate memory for managed object (uninitialized)
		auto ptr = allocator.allocate(1);
		// construct object in allocated memory
		std::allocator_traits<Allocator>::construct(allocator, ptr, 420);
		// construct uptr and deleter
		return UptrT<T, A>{ ptr, Deleter{ std::move(allocator) } };
	}

	template<class A>
	class Branch
	{
	public:
		using Allocator = std::allocator_traits<A>::template rebind_alloc<Branch>;
		Branch(int x_) : x{ x_ } {}
		int Get() const { return x; }
	private:
		int x;
	};

	template<class A>
	class Root
	{
	public:
		Root(int x, A allocator)
			:
			pBranch{ MakeUnique<Branch, A>(allocator) }
		{}
		int Get() const { return pBranch->Get(); }
	private:
		UptrT<Branch, A> pBranch;
	};


	class Server : public IServer
	{
	public:
		Server(std::string code)
		{
			// construct string in shm
			auto allocator = shm.get_allocator<char>();
			auto pMessage = shm.construct<ShmString>(MessageStringName)(allocator);
			*pMessage = (code + "-served").c_str();
			// construct ptr to string in shm
			shm.construct<bip::offset_ptr<ShmString>>(MessagePtrName)(pMessage);
		}
		void FreeUptrToMessage() override
		{
			pupMessage.reset();
		}
		void MakeUptrToMessage(std::string code) override
		{
			// construct named uptr in shm to anonymous string in shm, and own shm uptr via uptr in heap
			pupMessage = Uptr<Uptr<ShmString>>{ shm.construct<Uptr<ShmString>>(MessageUptrName)(
				shm.construct<ShmString>(bip::anonymous_instance)(shm.get_segment_manager()),
				UptrDeleter<ShmString>{ shm.get_segment_manager() }
			), UptrDeleter<Uptr<ShmString>>{ shm.get_segment_manager() } };
			**pupMessage = (code + "-u-served").c_str();
		}
	private:
		ShmSegment shm{ bip::create_only, SharedMemoryName, 0x10'0000 };
		Uptr<Uptr<ShmString>> pupMessage{ nullptr, UptrDeleter<Uptr<ShmString>>{ nullptr } };
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
		std::string Read() const override
		{
			return pMessage->c_str();
		}
		std::string ReadWithPointer() const override
		{
			return ppMessage->get()->c_str();
		}
		std::string ReadWithUptr() override
		{
			return shm.find<Uptr<ShmString>>(IServer::MessageUptrName).first->get()->c_str();
		}
		size_t GetFreeMemory() const override
		{
			return shm.get_free_memory();
		}
		int RoundtripRootInHeap() const override
		{
			const Root r{ 69, std::allocator<char>{} };
			return r.Get();
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