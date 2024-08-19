#pragma once
#include <memory>

namespace pmon::util
{
	inline constexpr size_t GetPadding(size_t byteIndex, size_t alignment)
	{
		const auto partialBytes = byteIndex % alignment;
		const auto padding = (alignment - partialBytes) % alignment;
		return padding;
	}

	template<typename T>
	constexpr size_t GetPadding(size_t byteIndex)
	{
		return GetPadding(byteIndex, alignof(T));
	}

	template<std::copy_constructible T>
	class CloningUptr : public std::unique_ptr<T>
	{
	public:
		CloningUptr() = default;
		explicit CloningUptr(T* p) noexcept : std::unique_ptr<T>{ p } {}
		CloningUptr(CloningUptr&& other) noexcept : std::unique_ptr<T>{ std::move(other) } {}
		// TODO: consider slicing implications / metaprogramming detection of clone() member etc.
		CloningUptr(const CloningUptr& other)
			:
			std::unique_ptr<T>{ bool(other) ? std::make_unique<T>(*other) : std::unique_ptr<T>{} }
		{}
		CloningUptr(std::unique_ptr<T>&& up) : std::unique_ptr<T>{ std::move(up) } {}
		CloningUptr& operator=(const CloningUptr& rhs)
		{
			std::unique_ptr<T>::reset(CloningUptr{ rhs }.std::unique_ptr<T>::release());
			return *this;
		}
		CloningUptr& operator=(CloningUptr&& rhs) noexcept
		{
			std::unique_ptr<T>::reset(rhs.release());
			return *this;
		}
		~CloningUptr() = default;
	};

	// get the size in bytes of a container's contents
	template<class C>
	size_t SizeInBytes(const C& container)
	{
		if (const auto size = std::size(container)) {
			return size * sizeof(*std::begin(container));
		}
		return 0;
	}
}