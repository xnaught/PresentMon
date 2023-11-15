#pragma once
#include <string>
#include <memory>
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

namespace pmon::ipc::experimental
{
	namespace bip = boost::interprocess;
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

	// <A> is any allocator, Branch::Allocator is <A> adapted to allocate this class type
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

	// <A> is any allocator
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

	class IServer
	{
	public:
		virtual ~IServer() = default;
		static constexpr const char* SharedMemoryName = "MySharedMemory-42069";
		static constexpr const char* MessageStringName = "message-string-777";
		static constexpr const char* MessagePtrName = "message-ptr-787";
		static constexpr const char* MessageUptrName = "message-uptr-57";
		static constexpr const char* RootPtrName = "root-ptr-157";
		static constexpr const char* ClientFreeUptrString = "client-free-string-11";
		static constexpr const char* ClientFreeRoot = "client-free-root-22";
		virtual void MakeUptrToMessage(std::string code) = 0;
		virtual void FreeUptrToMessage() = 0;
		virtual void MakeRoot(int x) = 0;
		virtual void FreeRoot() = 0;
		virtual int RoundtripRootInShared() = 0;
		virtual void CreateForClientFree(int x, std::string s) = 0;
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
		virtual std::string ReadForClientFree() = 0;
		virtual void ClientFree() = 0;
		static std::unique_ptr<IClient> Make();
	};

	const char* f();
}