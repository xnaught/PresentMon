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

	using bip::ipcdetail::to_raw_pointer;

	// <A> is allocator already adapted to handling the type T its meant to handle
	template<class A>
	struct AllocatorDeleter
	{
		using T = typename A::value_type;
		using pointer = std::allocator_traits<A>::pointer;
		AllocatorDeleter(A allocator_) : allocator{ std::move(allocator_) } {}
		void operator()(pointer ptr) { allocator.deallocate(to_raw_pointer(ptr), 1u); }
	private:
		A allocator;
	};

	template<template <class> typename T, class A>
	using UptrT = bip::unique_ptr<T<A>, AllocatorDeleter<typename T<A>::Allocator>>;

	template<template <class> typename T, class A, typename...P>
	auto MakeUnique(A allocator_in, P&&...args)
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
		std::allocator_traits<Allocator>::construct(allocator, to_raw_pointer(ptr), std::forward<P>(args)...);
		// construct uptr and deleter
		return UptrT<T, A>(to_raw_pointer(ptr), Deleter(allocator));
	}

	template<class A>
	class Branch
	{
	public:
		// allocator type for this instance, based on the allocator type used to template
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
			pBranch{ MakeUnique<Branch, A>(allocator, x) }
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
		int RoundtripRootInShared() override
		{
			
			auto allocator = shm.get_allocator<void>();
			const auto pRoot = shm.construct<Root<decltype(allocator)>>(bip::anonymous_instance)(69, std::move(allocator));
			const auto ret = pRoot->Get();
			shm.destroy_ptr(pRoot);
			return ret;
		}
		void MakeRoot(int x) override
		{
			pRoot = Uptr<Root<ShmAllocator<void>>>{ 
				shm.construct<Root<ShmAllocator<void>>>(RootPtrName)(x, shm.get_allocator<void>()),
				UptrDeleter<Root<ShmAllocator<void>>>{ shm.get_segment_manager() }
			};
		}
		void FreeRoot() override
		{
			pRoot.reset();
		}
		void CreateForClientFree(int x, std::string s) override
		{
			auto allo = shm.get_allocator<void>();
			shm.construct<Root<ShmAllocator<void>>>(ClientFreeRoot)(x, allo);
			**shm.construct<Uptr<ShmString>>(ClientFreeUptrString)(
				shm.construct<ShmString>(bip::anonymous_instance)(allo),
				UptrDeleter<ShmString>{ shm.get_segment_manager() }
			) = s.c_str();
		}
	private:
		ShmSegment shm{ bip::create_only, SharedMemoryName, 0x10'0000 };
		Uptr<Uptr<ShmString>> pupMessage{ nullptr, UptrDeleter<Uptr<ShmString>>{ nullptr } };
		Uptr<Root<ShmAllocator<void>>> pRoot{ nullptr, UptrDeleter<Root<ShmAllocator<void>>>{ nullptr } };
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
			auto pup = shm.find<Uptr<ShmString>>(IServer::MessageUptrName).first;
			return pup->get()->c_str();
		}
		size_t GetFreeMemory() const override
		{
			return shm.get_free_memory();
		}
		int RoundtripRootInHeap() const override
		{
			const Root r{ 420, std::allocator<char>{} };
			return r.Get();
		}
		int ReadRoot() override
		{
			auto pair = shm.find<Root<ShmAllocator<void>>>(IServer::RootPtrName);
			auto ptr = pair.first;
			return ptr->Get();
		}
		std::string ReadForClientFree() override
		{
			std::string s = shm.find<Uptr<ShmString>>(IServer::ClientFreeUptrString).first->get()->c_str();
			return s + "#" + std::to_string(shm.find<Root<ShmAllocator<void>>>(IServer::ClientFreeRoot).first->Get());
		}
		void ClientFree() override
		{
			shm.destroy<Root<ShmAllocator<void>>>(IServer::ClientFreeRoot);
			shm.destroy<Uptr<ShmString>>(IServer::ClientFreeUptrString);
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