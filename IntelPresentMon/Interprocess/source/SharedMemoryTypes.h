#pragma once
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

namespace pmon::ipc
{
	namespace bip = boost::interprocess;

	using ShmSegment = bip::managed_windows_shared_memory;
	using ShmSegmentManager = ShmSegment::segment_manager;
	template<typename T>
	using ShmAllocator = ShmSegmentManager::allocator<T>::type;
	using ShmString = bip::basic_string<char, std::char_traits<char>, ShmAllocator<char>>;
	template<typename T>
	using ShmVector = bip::vector<T, ShmAllocator<T>>;
	template<typename T>
	using ShmUniquePtr = typename bip::managed_unique_ptr<T, ShmSegment>::type;

	template<typename T, typename...P>
	ShmUniquePtr<T> ShmMakeUnique(ShmSegmentManager* pSegmentManager, P&&...params)
	{
		// make allocator for T
		auto allocator = pSegmentManager->get_allocator<T>();
		// allocate space in shared memory for object
		auto ptr = allocator.allocate(1);
		// construct object in allocated memory
		std::allocator_traits<ShmAllocator<T>>::construct(allocator, to_raw_pointer(ptr), std::forward<P>(params)...);
		// construct uptr compatible with storage in a shared segment
		return ShmUniquePtr<T>(ptr, bip::deleter<T, ShmSegmentManager>{ pSegmentManager });
	}
}